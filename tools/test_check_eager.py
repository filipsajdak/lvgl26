#!/usr/bin/env python3
"""Tests of tools/check_eager.py. Run: python3 tools/test_check_eager.py

Each eager case is one of the seven that lv/theme.hpp, lv/scaled_font.hpp and
lv/encoded_image.hpp held until 2026-11-19, cut down; each fixed form is the same line as it
is now."""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_eager as tool  # noqa: E402

SCANNING = {"theme_items", "theme_widgets", "look_items", "look_targets", "find", "find_later", "scan_api"}

EAGER = {
    "look_selectors": """template <class D = void>
inline constexpr std::span<const std::meta::info> look_selectors = std::define_static_array(look_targets());
""",
    "theme_classes": """template <class D = void>
inline constexpr std::span<const std::meta::info> theme_classes = std::define_static_array(theme_widgets());
""",
    "tiny_ttf_create": """template <class D = void>
inline constexpr std::meta::info tiny_ttf_create = find("lv_tiny_ttf_create_data");
""",
    "tiny_ttf_set_size": """template <class D = void>
inline constexpr std::meta::info tiny_ttf_set_size = find("lv_tiny_ttf_set_size");
""",
    "tiny_ttf_destroy": """template <class D = void>
inline constexpr std::meta::info tiny_ttf_destroy = find("lv_tiny_ttf_destroy");
""",
    "png_decoder": """template <class D = void>
inline constexpr std::meta::info png_decoder = find("lv_lodepng_init");
""",
    "jpeg_decoder": """template <class D = void>
inline constexpr std::meta::info jpeg_decoder = find("lv_tjpgd_init");
""",
}

FIXED = {
    "look_selectors": "template <class D = void>\n"
    "inline constexpr std::span<const std::meta::info> look_selectors = std::define_static_array(look_targets<D>());\n",
    "theme_classes": "template <class D = void>\n"
    "inline constexpr std::span<const std::meta::info> theme_classes = std::define_static_array(theme_widgets<D>());\n",
    "tiny_ttf_create": "template <class D = void>\n"
    "inline constexpr std::meta::info tiny_ttf_create = find_later<D>(\"lv_tiny_ttf_create_data\");\n",
    "png_decoder": "template <class D = void>\n"
    "inline constexpr std::meta::info png_decoder = find_later<D>(\"lv_lodepng_init\");\n",
}

SCAN_WITHOUT_PARAMETER = """consteval std::vector<theme_item> theme_items() {
  std::vector<theme_item> items;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked())) items.push_back({m});
  return items;
}
"""
SCAN_WITH_PARAMETER = "template <class D>\n" + SCAN_WITHOUT_PARAMETER


def found(text: str) -> list[str]:
    return [key for key, _ in tool.findings("h.hpp", text, SCANNING)]


class EagerCases(unittest.TestCase):
    def test_each_of_the_seven_is_found(self):
        for name, text in EAGER.items():
            with self.subTest(name=name):
                self.assertEqual(found(text), [f"h.hpp: {name}"])

    def test_their_fixed_forms_pass(self):
        for name, text in FIXED.items():
            with self.subTest(name=name):
                self.assertEqual(found(text), [])

    def test_a_scan_without_a_template_parameter_is_found(self):
        self.assertEqual(found(SCAN_WITHOUT_PARAMETER), ["h.hpp: theme_items"])

    def test_a_scan_with_a_template_parameter_passes(self):
        self.assertEqual(found(SCAN_WITH_PARAMETER), [])


class Values(unittest.TestCase):
    def test_an_initializer_that_names_the_parameter_passes(self):
        text = "template <fixed W, class = void>\ninline constexpr auto m = std::define_static_array(members_before_braces(^^props<W>));\n"
        self.assertEqual(found(text), [])

    def test_a_cheap_value_of_a_variable_passes(self):
        self.assertEqual(found("inline constexpr bezier in = of(LV_ANIM_EASE_IN);\n"), [])

    def test_a_scanning_call_in_a_variable_is_found(self):
        text = "inline constexpr std::span<const std::meta::info> api = std::define_static_array(scan_api());\n"
        self.assertEqual(found(text), ["h.hpp: api"])

    def test_a_comment_or_a_string_is_not_code(self):
        self.assertEqual(found("// inline constexpr auto t = find(\"x\");\n"), [])

    def test_the_scanning_functions_are_found_through_calls(self):
        texts = ["consteval int a() { for (auto m : std::meta::members_of(^^::, ctx)) {} return 0; }\n",
                 "consteval int b() { return a(); }\nconsteval int c() { return 1; }\n"]
        self.assertEqual(tool.scanning_functions(texts), {"a", "b"})


class AllowList(unittest.TestCase):
    def test_the_repository_is_green_with_its_allow_list(self):
        self.assertEqual(tool.problems_in(sorted(tool.HEADERS.rglob("*.hpp")), tool.allowed()), [])

    def test_every_entry_has_a_reason(self):
        for key, reason in tool.allowed().items():
            with self.subTest(key=key):
                self.assertGreater(len(reason.split()), 5)

    def test_an_entry_that_is_no_longer_found_is_stale(self):
        problems = tool.stale({"a.hpp: gone": "why"}, set(), {"a.hpp"})
        self.assertEqual(len(problems), 1)
        self.assertIn("no longer found", problems[0])


if __name__ == "__main__":
    unittest.main()
