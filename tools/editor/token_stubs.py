"""The part of the editor stubs that comes from design token files.

In a real build `lv::tokens<doc>` reads an embedded DTCG file and reflection makes a
struct of it. Here the struct is written out, one for each token file of the project.

The stub of `lv::tokens<doc>` chooses the struct by a key. The key is a number worked
out from the bytes of the file (`key_of`). So that an edited file still finds its
struct, the editor does not embed the token file itself: a Clang file overlay hands it
a marker file instead, which holds the key the file had when the stubs were written
(`marker`). A file that the overlay does not cover is found by its bytes.

Uses the standard library only.
"""

import json
import pathlib
import re

MARK = "@lvgl26-editor-tokens:"
SAMPLES = 1024
FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK = 0xFFFFFFFFFFFFFFFF

# The value type of a token of each DTCG "$type": see include/lv/tokens.hpp.
VALUE_TYPES = {
    "color": "lv_color_t",
    "dimension": "length",
    "duration": "duration",
    "number": "double",
    "fontWeight": "std::int32_t",
    "fontFamily": "std::string_view",
    "string": "std::string_view",
    "strokeStyle": "std::string_view",
    "cubicBezier": "bezier",
    "shadow": "shadow_token",
    "border": "border_token",
    "typography": "typography_token",
    "transition": "transition_token",
    "gradient": "gradient_token",
}
# The names include/lv/values.hpp does not use for a member: is_keyword there.
KEYWORDS = {
    "auto", "bool", "break", "case", "char", "class", "const", "continue", "default", "delete", "do",
    "double", "else", "enum", "export", "false", "float", "for", "if", "inline", "int", "long",
    "namespace", "new", "operator", "private", "public", "return", "short", "signed", "static",
    "struct", "switch", "template", "this", "true", "union", "unsigned", "using", "virtual", "void",
    "while",
}
LONGEST_ALIAS_CHAIN = 8
# The bytes a C++ string literal holds as they are: printable ASCII, space to tilde.
PRINTABLE_FIRST = ord(" ")
PRINTABLE_LAST = ord("~")
LONGEST_VALUE_IN_A_COMMENT = 100


def key_of(data: bytes) -> int:
    """The key of a token file: FNV-1a over its size and about a thousand of its bytes.

    lv::stub::token_key in tools/editor/stubs/lv/tokens.hpp works out the same number.
    Zeros at the end do not count: the #embed idiom adds one.
    """
    data = data.rstrip(b"\0")
    step = max(1, len(data) // SAMPLES)
    key = FNV_OFFSET
    for byte in len(data).to_bytes(8, "little") + data[::step]:
        key = ((key ^ byte) * FNV_PRIME) & MASK
    return key


def marker(key: int) -> str:
    """What the editor embeds in place of a token file."""
    return f"{MARK}{key}\n"


def member_name(key: str) -> str:
    """A JSON key as a C++ member name: "primary-500" -> primary_500, "2xl" -> _2xl."""
    name = re.sub(r"[^A-Za-z0-9_]", "_", key)
    if not name or name[0].isdigit():
        name = "_" + name
    return name + "_" if name in KEYWORDS else name


def is_alias(value: object) -> bool:
    return isinstance(value, str) and value.startswith("{") and value.endswith("}")


def alias_target(root: dict, alias: str) -> tuple[dict, str | None] | None:
    """The token an alias names, and the type it inherits on the way there."""
    at, inherited = root, None
    for part in alias[1:-1].split("."):
        at = at.get(part) if isinstance(at, dict) else None
        if not isinstance(at, dict):
            return None
        inherited = at.get("$type", inherited)
    return (at, inherited) if "$value" in at else None


def token_type(root: dict, token: dict, inherited: str | None, depth: int = 0) -> str | None:
    """The "$type" of a token: its own, its group's, or the one of the token it names."""
    kind = token.get("$type", inherited)
    if kind is not None or not is_alias(token.get("$value")) or depth > LONGEST_ALIAS_CHAIN:
        return kind
    target = alias_target(root, token["$value"])
    return token_type(root, target[0], target[1], depth + 1) if target else None


def is_token_file(document: object) -> bool:
    """Whether `document` is a DTCG file: an object with a "$value" somewhere in it."""
    if not isinstance(document, dict):
        return False
    return "$value" in document or any(is_token_file(child) for child in document.values())


def comment_of(token: dict) -> str:
    value = json.dumps(token["$value"], ensure_ascii=True)
    if len(value) > LONGEST_VALUE_IN_A_COMMENT:
        value = value[:LONGEST_VALUE_IN_A_COMMENT] + " ..."
    said = str(token.get("$description", "")).replace("\n", " ").strip()
    return f"{said} Value: {value}" if said else f"Value: {value}"


def group_type_name(name: str, taken: set[str]) -> str:
    """A name for the struct of a group that no member of its parent has."""
    candidate = f"{name}_tokens"
    while candidate in taken:
        candidate += "_"
    return candidate


def children_of(group: dict) -> list[tuple[str, dict]]:
    return [(key, child) for key, child in group.items() if not key.startswith("$") and isinstance(child, dict)]


def group_struct(root: dict, group: dict, name: str, inherited: str | None, indent: str) -> list[str]:
    """The lines of the struct `name` for one group of tokens, its sub-groups first."""
    inherited = group.get("$type", inherited)
    children = children_of(group)
    taken = {member_name(key) for key, _ in children}
    lines = [f"{indent}struct {name} {{"]
    inner = indent + "  "
    for key, child in children:
        member = member_name(key)
        if "$value" not in child:
            child_type = group_type_name(member, taken)
            lines += group_struct(root, child, child_type, inherited, inner)
            lines.append(f"{inner}{child_type} {member}{{}};")
            continue
        value_type = VALUE_TYPES.get(token_type(root, child, inherited) or "")
        if value_type is not None:
            lines.append(f"{inner}/// {comment_of(child)}")
            lines.append(f"{inner}token<{value_type}> {member}{{}};")
    lines.append(f"{indent}}};")
    return lines


def group_paths(group: dict, prefix: str = "") -> list[str]:
    """The dotted path of every group below `group`, as lv::look writes it, in file order.

    The compiler also takes a path made of C++ member names ("ink_tubes" for "ink-tubes"), so
    a group whose name changes has both.
    """
    paths = []
    for key, child in children_of(group):
        if "$value" in child:
            continue
        for spelling in dict.fromkeys([key, member_name(key)]):
            here = f"{prefix}{spelling}"
            paths.append(here)
            paths += group_paths(child, here + ".")
    return paths


def cpp_string_literal(text: str) -> str:
    """`text` as a C++ string literal: a quote or a backslash is escaped, any other byte outside
    printable ASCII is written in octal, so a group name of the token file cannot end the literal."""
    out = []
    for byte in text.encode():
        char = chr(byte)
        if char in '"\\':
            out.append("\\" + char)
        elif PRINTABLE_FIRST <= byte <= PRINTABLE_LAST:
            out.append(char)
        else:
            out.append(f"\\{byte:03o}")
    return '"' + "".join(out) + '"'


def struct_name(key: int) -> str:
    return f"tokens_{key:016x}"


def file_struct(data: bytes, said: str) -> str:
    """The struct of one token file and the line that gives it to lv::tokens."""
    key = key_of(data)
    root = json.loads(data.rstrip(b"\0"))
    lines = [f"/// The design tokens of {said}."] + group_struct(root, root, struct_name(key), None, "")
    lines.append(f"template <>\nstruct token_file<{key}ULL> {{\n  using type = {struct_name(key)};\n}};")
    groups = " ".join(group_paths(root))
    lines.append(f'/// The groups of this file that lv::look can name.\n'
                 f'template <>\ninline constexpr const char* look_groups<{struct_name(key)}> = {cpp_string_literal(groups)};')
    return "\n".join(lines)


def header(files: list[tuple[str, bytes]]) -> str:
    """The text of lv/token_files.hpp for token files given as (what to call it, bytes)."""
    seen: set[int] = set()
    structs = []
    for said, data in files:
        if key_of(data) not in seen:
            seen.add(key_of(data))
            structs.append(file_struct(data, said))
    parts = [
        "#pragma once",
        "// Written by tools/editor/generate.py from the design token files of the project.\n"
        "// For editors only: it says what lv::tokens<...> holds in a real build. Do not edit.",
        "#include <lv/stub.hpp>\n#include <lv/values.hpp>",
        "namespace lv::stub {",
        *structs,
        "}  // namespace lv::stub",
    ]
    return "\n\n".join(parts) + "\n"
