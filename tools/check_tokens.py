#!/usr/bin/env python3
"""Checks every token file of the repository against the official Design Tokens Format
schemas, version 2025.10.

    python3 tools/check_tokens.py            the gate: exit 1 on a new or a stale exception
    python3 tools/check_tokens.py --update   rewrite tools/token_exceptions.json from what is found

A token file is a JSON file that git tracks and that holds a "$value" key at any depth,
unless a prefix in "excluded" of tools/token_exceptions.json names it (fuzzer inputs and
files that are invalid on purpose). A JSON file with a "resolutionOrder" key is a
resolver document and is checked against resolver.json instead.

The official format schema checks a value only when the token names its own "$type", so a
type that comes from a group is pushed down to its tokens first (issue 439 of the
Design Tokens Community Group). Each token is then validated alone, so a message names
the token and the member at fault. The groups are validated with their tokens left out.

"known" lists, for each file, how many violations of each kind it has today. A violation
that is not listed fails, and so does a listed one that no longer occurs: the list shrinks
as the files convert. A kind is the token's type, the member at fault and the schema keyword.

The schemas in tools/dtcg/ are copies; see tools/dtcg/README.md. They need no network.
Needs the jsonschema package.
"""

import argparse
import collections
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXCEPTIONS = ROOT / "tools" / "token_exceptions.json"
SCHEMAS = ROOT / "tools" / "dtcg" / "2025.10"
RESOLVER_KEY = "resolutionOrder"
STAND_IN = {"$type": "number", "$value": 0}
INDEX = re.compile(r"\[\d+\]")


class Violation(collections.namedtuple("Violation", "path kind message")):
    """Where it is (a dotted JSON path), what kind it is, and the schema's message."""


def is_token(node):
    return isinstance(node, dict) and "$value" in node


def is_member(key):
    """A child of a group that is a token or a group: "$root" is a token, other "$" keys are properties."""
    return key == "$root" or not key.startswith("$")


def members(group):
    return [(key, child) for key, child in group.items() if is_member(key) and isinstance(child, dict)]


def push_types(node, inherited=None):
    """Gives each token the "$type" of its nearest group, in place."""
    inherited = node.get("$type", inherited)
    for _, child in members(node):
        if is_token(child):
            if inherited and "$type" not in child:
                child["$type"] = inherited
        else:
            push_types(child, inherited)


def tokens(node, path=()):
    for key, child in members(node):
        if is_token(child):
            yield path + (key,), child
        else:
            yield from tokens(child, path + (key,))


def without_tokens(node):
    """The document with every token replaced by one that is valid, so that only the groups are judged."""
    return {key: (dict(STAND_IN) if is_token(child) and is_member(key) else
                  without_tokens(child) if is_member(key) and isinstance(child, dict) else child)
            for key, child in node.items()}


def leaves(error):
    """The innermost errors of a oneOf or anyOf: the ones that say what is wrong."""
    return [leaf for inner in error.context for leaf in leaves(inner)] if error.context else [error]


def deepest(error):
    """The leaves that sit deepest in the document, one for each place: the first alternative says why."""
    found = leaves(error)
    depth = max(len(leaf.absolute_path) for leaf in found)
    out = {}
    for leaf in found:
        if len(leaf.absolute_path) == depth:
            out.setdefault(tuple(leaf.absolute_path), leaf)
    return list(out.values())


def dotted(parts):
    return ".".join(str(part) for part in parts)


def kind_of(token_type, relative, keyword):
    where = INDEX.sub("[]", "".join(f"[{part}]" if isinstance(part, int) else f".{part}" for part in relative)).lstrip(".")
    return f"{token_type or 'untyped'} {where or 'token'}: {keyword}"


def is_reference(value):
    return (isinstance(value, str) and value.startswith("{")) or (isinstance(value, dict) and "$ref" in value)


def untyped(path, token):
    """The schema lets a token without a type pass; the format (5.2.2) does not, unless an alias gives it one."""
    if "$type" in token or is_reference(token["$value"]):
        return []
    return [Violation(dotted(path), "untyped token", "the token has no $type, and none comes from a group or an alias")]


def token_violations(validator, path, token):
    """The violations of one token. It is wrapped in a group, as the schema wants a group at the root."""
    out = untyped(path, token)
    for error in validator.iter_errors({"g": {"x": token}}):
        for leaf in deepest(error):
            relative = list(leaf.absolute_path)[2:]
            out.append(Violation(dotted(path + tuple(relative)), kind_of(token.get("$type"), relative, leaf.validator),
                                 leaf.message))
    return out


def group_violations(validator, document):
    out = []
    for error in validator.iter_errors(without_tokens(document)):
        for leaf in deepest(error):
            out.append(Violation(dotted(leaf.absolute_path) or "(root)", f"group: {leaf.validator}", leaf.message))
    return out


def violations(validator, document):
    """All violations of a token document, after the types are pushed down."""
    push_types(document)
    out = group_violations(validator, document)
    for path, token in tokens(document):
        out += token_violations(validator, path, token)
    return out


def resolver_violations(validator, document):
    return [Violation(dotted(error.absolute_path) or "(root)", f"resolver: {error.validator}", error.message)
            for error in validator.iter_errors(document)]


def validators():
    import jsonschema
    import referencing

    def no_network(uri):
        raise referencing.exceptions.NoSuchResource(uri)

    registry = referencing.Registry(retrieve=no_network)
    return {name: jsonschema.Draft7Validator(json.loads((SCHEMAS / f"{name}.json").read_text()), registry=registry)
            for name in ("format", "resolver")}


def file_violations(checkers, document):
    if isinstance(document, dict) and RESOLVER_KEY in document:
        return resolver_violations(checkers["resolver"], document)
    return violations(checkers["format"], document)


def has_value_key(node):
    if isinstance(node, dict):
        return "$value" in node or any(has_value_key(child) for child in node.values())
    if isinstance(node, list):
        return any(has_value_key(child) for child in node)
    return False


def tracked_json(root):
    # safe.directory: a CI container runs as another user than the one that owns the checkout.
    command = ["git", "-c", f"safe.directory={root}", "-C", str(root), "ls-files", "*.json"]
    out = subprocess.run(command, check=True, capture_output=True, text=True)
    return out.stdout.split()


def load(path):
    try:
        return json.loads(path.read_text())
    except ValueError:
        return None


def token_files(root, excluded):
    """The tracked JSON files with a "$value" key or a "resolutionOrder" key, minus the excluded prefixes."""
    found = {}
    for name in tracked_json(root):
        if any(name.startswith(prefix) for prefix in excluded):
            continue
        document = load(root / name)
        if has_value_key(document) or (isinstance(document, dict) and RESOLVER_KEY in document):
            found[name] = document
    return found


def counts(found):
    """{file: {kind: count}} of the violations, and {file: [Violation]} for the messages."""
    table = {}
    for name, items in found.items():
        for item in items:
            table.setdefault(name, collections.Counter())[item.kind] += 1
    return {name: dict(sorted(kinds.items())) for name, kinds in sorted(table.items())}


def differences(found, known):
    """What differs between the violations found and the list: a list of text lines."""
    actual = counts(found)
    out = []
    for name in sorted(set(actual) | set(known)):
        for kind in sorted(set(actual.get(name, {})) | set(known.get(name, {}))):
            have, listed = actual.get(name, {}).get(kind, 0), known.get(name, {}).get(kind, 0)
            if have > listed:
                out.append(f"{name}: {have - listed} new violation(s) of '{kind}' (listed {listed}, found {have})")
                out += [f"    {name}  {item.path}  {item.message[:160]}" for item in found[name] if item.kind == kind]
            elif have < listed:
                out.append(f"{name}: stale exception '{kind}' (listed {listed}, found {have}): lower or remove it in "
                           f"tools/token_exceptions.json (python3 tools/check_tokens.py --update)")
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--update", action="store_true", help="write tools/token_exceptions.json from what is found")
    args = parser.parse_args(argv)
    config = json.loads(EXCEPTIONS.read_text())
    checkers = validators()
    files = token_files(ROOT, config["excluded"])
    found = {name: file_violations(checkers, document) for name, document in files.items()}
    found = {name: items for name, items in found.items() if items}
    if args.update:
        config["known"] = counts(found)
        EXCEPTIONS.write_text(json.dumps(config, indent=2) + "\n")
        print(f"wrote {EXCEPTIONS.relative_to(ROOT)}: {len(config['known'])} file(s) with exceptions")
        return 0
    problems = differences(found, config["known"])
    print("\n".join(problems) if problems else f"{len(files)} token files checked, {len(found)} with listed exceptions")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
