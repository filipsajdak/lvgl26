#!/usr/bin/env python3
"""Unit tests of the editor stub tools: python3 tools/editor/test_editor.py"""

import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import check  # noqa: E402
import generate  # noqa: E402
import inventory  # noqa: E402
import reference_stubs  # noqa: E402
import token_stubs  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
ENUM = {"write": "one of the enum's names, in quotes", "enum": "lv_align_t"}
WHOLE = {"write": "a whole number (not bool, not floating point)"}


class ValueTypes(unittest.TestCase):
    def test_units(self):
        self.assertEqual(reference_stubs.value_type("true or false"), "lv::strict<bool>")
        self.assertEqual(reference_stubs.value_type("an angle in whole degrees (90 * lv::deg)"), "lv::angle")
        self.assertEqual(reference_stubs.value_type("an angle (90 * lv::deg, or 22.5 * lv::deg ...)"), "lv::decidegrees")
        self.assertEqual(reference_stubs.value_type("a width or x (120 * lv::px)"), "lv::horizontal")

    def test_design_values(self):
        token = "a design token (for example tok.space.md) or lv::raw(...), holding "
        self.assertEqual(reference_stubs.value_type(token + "lv_color_t"), "lv::styled<lv_color_t>")
        self.assertEqual(reference_stubs.value_type(token + "a time (300 * lv::ms)"), "lv::styled<lv::duration>")
        self.assertEqual(reference_stubs.value_type("a font from a typography token (...)"),
                         "lv::styled<const lv_font_t*>")

    def test_c_types(self):
        self.assertEqual(reference_stubs.value_type("const char*"), "const char*")
        self.assertEqual(reference_stubs.value_type("_lv_obj_t*"), "struct _lv_obj_t*")
        self.assertEqual(reference_stubs.value_type("void (*)(_lv_anim_t*)"), "void (*)(_lv_anim_t*)")
        self.assertEqual(reference_stubs.value_type("something new, in words"), "stub::anything")

    def test_member_types(self):
        self.assertEqual(reference_stubs.member_type({"takes": [ENUM, ENUM]}),
                         "stub::enum_arg<stub::lv_align_names, stub::lv_align_names>")
        self.assertEqual(reference_stubs.member_type({"takes": [ENUM, WHOLE]}),
                         "lv::arg<stub::name_of<stub::lv_align_names>, lv::strict<std::int32_t>>")


class Members(unittest.TestCase):
    reference = {
        "lvgl": "0", "enums": {"lv_align_t": ["center", "top_left"]},
        "base": [{"name": "id", "kind": "id"}, {"name": "style", "kind": "style"},
                 {"name": "style_pressed", "kind": "style"}, {"name": "on_delete", "kind": "event"},
                 {"name": "width", "kind": "property", "sets": "lv_obj_set_width",
                  "takes": [{"write": "a width or x (120 * lv::px)"}]},
                 {"name": "value", "kind": "property", "sets": "lv_obj_set_value", "takes": [WHOLE]}],
        "widgets": {"arc": [{"name": "value", "kind": "property", "sets": "lv_arc_set_value", "takes": [WHOLE]},
                            {"name": "rotation", "kind": "property", "sets": "lv_arc_set_rotation",
                             "takes": [{"write": "an angle in whole degrees (90 * lv::deg)"}]},
                            {"name": "range", "kind": "property", "sets": "lv_arc_set_range", "takes": [WHOLE, WHOLE]}],
                    "switch": []},
        "style": [{"name": "align", "takes": [ENUM]}],
    }

    def test_own_member_wins_and_order_is_by_group(self):
        members = reference_stubs.members_of(self.reference, "arc")
        self.assertEqual([m["name"] for m in members],
                         ["id", "range", "rotation", "value", "width", "style", "style_pressed", "on_delete"])
        self.assertEqual([m["sets"] for m in members if m["name"] == "value"], ["lv_arc_set_value"])

    def test_comment_names_the_setter_and_the_enum_names(self):
        text = reference_stubs.property_comment({"sets": "lv_obj_set_align", "takes": [ENUM]}, self.reference["enums"])
        self.assertIn("Sets lv_obj_set_align.", text)
        self.assertIn("center, top_left", text)

    def test_animatable_is_one_number(self):
        arc = {m["name"]: reference_stubs.animatable_type(m) for m in self.reference["widgets"]["arc"]}
        self.assertEqual(arc, {"value": "lv::strict<std::int32_t>", "rotation": "lv::angle", "range": None})
        self.assertIsNone(reference_stubs.animatable_type({"name": "id", "kind": "id"}))
        self.assertIsNone(reference_stubs.animatable_type({"takes": [ENUM]}))

    def test_getter_of_an_angle_has_its_unit(self):
        rotation = reference_stubs.getter("arc", self.reference["widgets"]["arc"][1])
        self.assertIn('getter<"arc", "rotation">', rotation)
        self.assertIn("unit_of<lv::angle, decltype(lv_arc_get_rotation(o, a...))>", rotation)
        self.assertIsNone(reference_stubs.getter("obj", {"name": "id", "kind": "id"}))

    def test_header_has_every_part(self):
        text = reference_stubs.header(self.reference, "test.json")
        for part in ["struct lv_align_names", "struct style_t", "struct props_arc", 'props_for<"switch">',
                     "struct animatable_obj", "lv::event_ref<LV_EVENT_DELETE> gone;", "look toggle{};",
                     "look screen{};", "style_t style_pressed{};", "id_name id{};"]:
            self.assertIn(part, text)
        self.assertLess(text.index("look arc{}"), text.index("look obj{}"))
        self.assertLess(text.index("look screen{}"), text.index("look toggle{}"))

    def test_the_committed_reference_files_are_read(self):
        for file in sorted((ROOT / "docs" / "reference").glob("lvgl-*.json")):
            text = reference_stubs.header(json.loads(file.read_text()), file.name)
            self.assertNotIn("stub::anything", text, f"{file.name} has a value that the stubs do not know")


class Tokens(unittest.TestCase):
    document = {
        "$description": "said",
        "color": {"$type": "color", "ink": {"$value": "#000", "$description": "Text."}, "accent": {"$value": "{color.ink}"}},
        "space": {"2xl": {"$type": "dimension", "$value": "32px"}, "primary-500": {"$type": "dimension", "$value": "1px"}},
        "alias": {"far": {"$value": "{color.accent}"}, "nowhere": {"$value": "{no.such}"}, "odd": {"$type": "new", "$value": 1}},
        "class": {"int": {"$type": "number", "$value": 1}},
        "nest": {"nest_tokens": {"$type": "number", "$value": 1}, "nest": {"deep": {"$type": "duration", "$value": "1ms"}}},
        "forms": {"$type": "color", "paper": {"$value": {"hex": "#f4efe6", "colorSpace": "srgb", "components": [0.9569, 0.9373, 0.902]}},
                  "glass": {"$value": {"hex": "#000000", "colorSpace": "srgb", "components": [0, 0, 0], "alpha": 0.5}},
                  "gap": {"$type": "dimension", "$value": {"value": 8, "unit": "px"}}},
    }

    def test_member_names(self):
        self.assertEqual(token_stubs.member_name("primary-500"), "primary_500")
        self.assertEqual(token_stubs.member_name("2xl"), "_2xl")
        self.assertEqual(token_stubs.member_name("class"), "class_")
        self.assertEqual(token_stubs.member_name(""), "_")

    def test_types_come_from_the_token_the_group_or_the_alias(self):
        text = token_stubs.file_struct(json.dumps(self.document).encode(), "a test")
        for part in ["token<lv_color_t> ink{};", "token<lv_color_t> accent{};", "token<length> _2xl{};",
                     "token<length> primary_500{};", "token<lv_color_t> far{};", "token<double> int_{};",
                     "/// Text. Value: \"#000\"", "token<duration> deep{};"]:
            self.assertIn(part, text)
        self.assertNotIn("nowhere", text)
        self.assertNotIn("odd", text)

    def test_the_object_forms_are_typed_and_shown_as_a_designer_writes_them(self):
        text = token_stubs.file_struct(json.dumps(self.document).encode(), "a test")
        for part in ["token<lv_color_t> paper{};", "/// Value: #f4efe6", "/// Value: #000000 at alpha 0.5",
                     "token<length> gap{};", "/// Value: 8px"]:
            self.assertIn(part, text)

    def test_a_group_struct_never_has_the_name_of_a_member(self):
        text = token_stubs.file_struct(json.dumps(self.document).encode(), "a test")
        self.assertIn("struct nest_tokens_ {", text)
        self.assertIn("token<double> nest_tokens{};", text)

    def test_the_groups_a_look_can_name(self):
        root = {"tile": {"radius": {"$type": "dimension", "$value": "1px"},
                         "ink-level": {"bg_color": {"$type": "color", "$value": "#000"}, "deep": {"x": {"$value": 1}}}},
                "color": {"ink": {"$type": "color", "$value": "#000"}}, "$description": "no group"}
        self.assertEqual(token_stubs.group_paths(root),
                         ["tile", "tile.ink-level", "tile.ink-level.deep", "tile.ink_level", "tile.ink_level.deep", "color"])
        paths = token_stubs.group_paths(root)
        self.assertIn("tile.ink-level", paths)
        self.assertIn("tile.ink_level.deep", paths)
        self.assertNotIn("tile.radius", paths)  # a token is no look
        self.assertNotIn("$description", paths)
        self.assertIn('look_groups<tokens_', token_stubs.file_struct(json.dumps(root).encode(), "a test"))

    def test_a_group_name_cannot_end_the_string_of_groups(self):
        root = {'say "hi"': {"radius": {"$type": "dimension", "$value": "1px"}},
                "back\\slash": {"radius": {"$type": "dimension", "$value": "1px"}},
                "caf\u00e9": {"radius": {"$type": "dimension", "$value": "1px"}}}
        text = token_stubs.file_struct(json.dumps(root).encode(), "a test")
        line = next(l for l in text.splitlines() if "look_groups<" in l)
        self.assertIn('say \\"hi\\"', line)
        self.assertIn("back\\\\slash", line)
        self.assertIn("caf\\303\\251", line)
        self.assertTrue(line.endswith('";'))

    def test_a_string_literal_keeps_printable_ascii(self):
        self.assertEqual(token_stubs.cpp_string_literal("tile tile.ink-level"), '"tile tile.ink-level"')
        self.assertEqual(token_stubs.cpp_string_literal("a\nb"), '"a\\012b"')

    def test_alias_in_a_circle_ends(self):
        root = {"a": {"$value": "{b}"}, "b": {"$value": "{a}"}}
        self.assertIsNone(token_stubs.token_type(root, root["a"], None))

    def test_key(self):
        self.assertEqual(token_stubs.key_of(b"{}"), token_stubs.key_of(b"{}\0"))
        self.assertNotEqual(token_stubs.key_of(b"{}"), token_stubs.key_of(b"{ }"))
        self.assertEqual(token_stubs.key_of(b""), 0xA8C7F832281A39C5)
        large = bytes(range(256)) * 20
        self.assertNotEqual(token_stubs.key_of(large), token_stubs.key_of(large + b"x"))
        # Two files that differ in one byte far from the start and the end have two keys.
        long_file = b" " * 100000
        self.assertNotEqual(token_stubs.key_of(long_file + b"a" + long_file), token_stubs.key_of(long_file + b"b" + long_file))
        self.assertEqual(token_stubs.marker(7), "@lvgl26-editor-tokens:7\n")

    def test_two_files_with_the_same_bytes_are_one_struct(self):
        data = json.dumps(self.document).encode()
        self.assertEqual(token_stubs.header([("one", data), ("two", data)]).count("struct token_file<"), 1)

    def test_what_a_token_file_is(self):
        self.assertTrue(token_stubs.is_token_file(self.document))
        self.assertFalse(token_stubs.is_token_file({"name": "package", "version": {"major": 1}}))
        self.assertFalse(token_stubs.is_token_file([{"$value": 1}]))


class Generator(unittest.TestCase):
    def test_token_files_are_found_outside_build_trees(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for path in ["ui/tokens.json", "build/CMakeCache.txt", "build/tokens.json", ".git/tokens.json", "other.json",
                         "broken.json", "out/tokens.json"]:
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text('{"a": {"$value": 1}}' if path.endswith("tokens.json") else '{"a": 1}')
            (root / "broken.json").write_text("{")
            found = generate.find_token_files([root], [], [root / "out"])
            self.assertEqual([path.relative_to(root.resolve()).as_posix() for path in map(pathlib.Path.resolve, found)],
                             ["ui/tokens.json"])
            with self.assertRaises(SystemExit):
                generate.find_token_files([], [root / "other.json"])

    def test_two_token_files_that_differ_in_one_late_byte_are_two_markers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            padding = "x" * 100000
            for name, last in (("a", "1"), ("b", "2")):
                document = {"$description": padding + last, "color": {"$type": "color", "ink": {"$value": "#000000"}}}
                (root / name).mkdir()
                (root / name / "tokens.json").write_text(json.dumps(document))
            out = root / "out"
            tokens = generate.find_token_files([root], [])
            self.assertEqual(len(tokens), 2)
            generate.generate(ROOT / "docs" / "reference" / "lvgl-v9.6.0.json", out, tokens, ROOT / "include")
            self.assertEqual(len(list((out / "embed").iterdir())), 2)

    def test_everything_is_written_and_a_second_run_changes_nothing(self):
        with tempfile.TemporaryDirectory() as directory:
            out = pathlib.Path(directory) / "out"
            tokens = generate.find_token_files([ROOT / "examples"], [])
            reference = ROOT / "docs" / "reference" / "lvgl-v9.6.0.json"
            generate.generate(reference, out, tokens, ROOT / "include")
            for file in ["lv/lv.hpp", "lv/generated.hpp", "lv/token_files.hpp", "lv/signal_tokens.hpp", "overlay.yaml",
                         "fallback/contracts", "token_files.txt"]:
                self.assertTrue((out / file).exists(), file)
            overlay = json.loads((out / "overlay.yaml").read_text())
            self.assertEqual(overlay["roots"][0]["type"], "directory-remap")
            self.assertEqual(len(list((out / "embed").iterdir())), len(tokens))
            before = (out / "lv" / "generated.hpp").stat().st_mtime_ns
            generate.generate(reference, out, tokens, ROOT / "include")
            self.assertEqual((out / "lv" / "generated.hpp").stat().st_mtime_ns, before)


class Check(unittest.TestCase):
    def test_an_editor_keeps_what_it_can_use(self):
        command = ["g++-16", "-DA=1", "-I/inc", "-isystem", "/sys", "-std=gnu++26", "--embed-dir=/e", "-freflection",
                   "-fconstexpr-ops-limit=9", "-Wall", "-g", "-o", "x.o", "-c", "x.cpp"]
        self.assertEqual(check.editor_flags(command),
                         ["-DA=1", "-I/inc", "-isystem", "/sys", "-std=gnu++26", "--embed-dir=/e"])

    def test_a_file_may_ask_not_to_be_read(self):
        with tempfile.TemporaryDirectory() as folder:
            asks = pathlib.Path(folder) / "asks.cpp"
            asks.write_text("// a window\n// editor-check: skip, it needs SDL\nint main() {}\n")
            plain = pathlib.Path(folder) / "plain.cpp"
            plain.write_text("int main() {}\n")
            late = pathlib.Path(folder) / "late.cpp"
            late.write_text("\n" * check.SKIP_LINES + "// editor-check: skip, too late\n")
            self.assertEqual(check.skip_reason(asks), "it needs SDL")
            self.assertIsNone(check.skip_reason(plain))
            self.assertIsNone(check.skip_reason(late))

    def test_a_file_outside_the_build_borrows_a_command(self):
        commands = {pathlib.Path("/p/a.cpp"): ["g++", "-DA", "-c", "/p/a.cpp"]}
        self.assertEqual(check.flags_for(pathlib.Path("/q/b.cpp"), commands, pathlib.Path("/p/a.cpp")),
                         ["-DA", "--embed-dir=/q"])
        with self.assertRaises(SystemExit):
            check.flags_for(pathlib.Path("/q/b.cpp"), commands, None)

    def test_the_pages_name_the_files_they_show(self):
        sources = check.snippet_sources(ROOT / "docs")
        self.assertIn(ROOT / "examples" / "quick_start" / "main.cpp", sources)
        self.assertTrue(all(source.suffix == ".cpp" for source in sources))

    def test_a_mistake_a_page_shows_is_not_read(self):
        with tempfile.TemporaryDirectory() as folder:
            docs = pathlib.Path(folder) / "docs"
            docs.mkdir()
            (docs / "page.md").write_text("<!-- snippet: tests/fail/wrong.cpp -->\n<!-- snippet: examples/a/main.cpp x -->\n")
            self.assertEqual(check.snippet_sources(docs), [pathlib.Path(folder).resolve() / "examples" / "a" / "main.cpp"])


class Inventory(unittest.TestCase):
    stubs = {"a.hpp": "namespace lv {\nstruct state;\n}", "b.hpp": "namespace lv::ease {\ninline constexpr easing in{};\n}"}

    def test_names(self):
        self.assertEqual(inventory.public_names("lv::state\nlv::__dguide_x\nnoise\nlv::state\n"), ["lv::state"])

    def test_declared_means_in_its_namespace(self):
        self.assertTrue(inventory.is_declared("lv::state", self.stubs))
        self.assertTrue(inventory.is_declared("lv::ease::in", self.stubs))
        self.assertFalse(inventory.is_declared("lv::ease::out", self.stubs))
        self.assertFalse(inventory.is_declared("lv::mcp::state", self.stubs))

    def test_used(self):
        self.assertTrue(inventory.is_used("lv::units::px", "static_assert(lv::px);"))
        self.assertFalse(inventory.is_used("lv::state", "lv::states x;"))
        self.assertEqual(inventory.problems(["lv::state"], self.stubs, ""),
                         ["lv::state: not used in tests/editor/api_uses.cpp"])
        self.assertIn("using lv::state;", inventory.using_file(["lv::state"]))


if __name__ == "__main__":
    unittest.main()
