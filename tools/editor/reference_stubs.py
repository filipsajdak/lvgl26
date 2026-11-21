"""The part of the editor stubs that comes from the reference data.

`header(reference)` returns the text of lv/generated.hpp: for one LVGL version, the
structs that reflection makes in a real build, written out as plain C++.

    enums        a type for each enum, which knows the names code writes in quotes
    style        lv::stub::style_t, one member for each style property
    props        lv::stub::props_<widget>, one member for each property, event and style
    animatable   lv::stub::animatable_<widget>, the properties lv::animate can run
    getters      which LVGL function view.get<"name">() calls
    events       lv::stub::events, what view.next() returns
    theme        lv::stub::look and lv::stub::theme_spec, what lv::theme takes

Uses the standard library only.
"""

import re

# What a value's `write` text starts with, and the C++ type that takes such a value.
# The longer of two texts that start alike comes first.
UNITS = [
    ("a list of options", "lv::option_list"),
    ("true or false", "lv::strict<bool>"),
    ("a whole number", "lv::strict<std::int32_t>"),
    ("a length", "lv::length"),
    ("a width or x", "lv::horizontal"),
    ("a height or y", "lv::vertical"),
    ("an angle in whole degrees", "lv::angle"),
    ("an angle", "lv::decidegrees"),
    ("a time", "lv::duration"),
    ("an opacity", "lv::opa"),
    ("a color", "lv_color_t"),
]
FONT = "a font from a typography token"
TOKEN = "a design token"
ANIMATABLE = {"lv::strict<std::int32_t>", "lv::length", "lv::horizontal", "lv::vertical", "lv::angle", "lv::decidegrees",
              "lv::opa"}
ANGLES = {"lv::angle", "lv::decidegrees"}
C_TYPE = re.compile(r"(const )?(unsigned )?[A-Za-z_]\w*( const)?[ *]*(const)?[*]*")
C_FUNCTION = re.compile(r"void \(\*\)\(([A-Za-z_]\w*\*?)?\)")
MOST_NAMES_IN_A_COMMENT = 48


def identifier(name: str) -> str:
    """`name` as part of a C++ identifier."""
    return re.sub(r"\W", "_", name)


def c_type(text: str) -> str | None:
    """`text` as a C++ type, when it is the name of one of LVGL's types."""
    if C_FUNCTION.fullmatch(text):
        return text
    if not C_TYPE.fullmatch(text):
        return None
    # A name that starts with an underscore is a struct tag. Said with `struct`, it is
    # also a type where LVGL's configuration leaves the widget out.
    return re.sub(r"\b(_lv_\w+)", r"struct \1", text)


def value_type(write: str) -> str:
    """The C++ type of one value of a property, from how the reference says to write it."""
    if write.startswith(FONT):
        return "lv::styled<const lv_font_t*>"
    if write.startswith(TOKEN) and ", holding " in write:
        return f"lv::styled<{value_type(write.split(', holding ', 1)[1])}>"
    for start, unit in UNITS:
        if write.startswith(start):
            return unit
    return c_type(write) or "stub::anything"


def enum_names_type(enum: str) -> str:
    """The stub type that knows the names of the LVGL enum `enum`."""
    return f"stub::{identifier(enum.removesuffix('_t'))}_names"


def member_type(member: dict) -> str:
    """The type of a property member: its values, or enumerator names when all are enums."""
    takes = member["takes"]
    if all("enum" in take for take in takes):
        return f"stub::enum_arg<{', '.join(enum_names_type(take['enum']) for take in takes)}>"
    values = [f"stub::name_of<{enum_names_type(take['enum'])}>" if "enum" in take else value_type(take["write"])
              for take in takes]
    return f"lv::arg<{', '.join(values)}>"


def names_text(names: list[str]) -> str:
    shown = ", ".join(names[:MOST_NAMES_IN_A_COMMENT])
    return shown + (", ..." if len(names) > MOST_NAMES_IN_A_COMMENT else "")


def take_text(take: dict, enums: dict) -> str:
    if "enum" not in take:
        return take["write"]
    return f"one of the names of {take['enum']}, in quotes: {names_text(enums[take['enum']])}"


def property_comment(member: dict, enums: dict) -> str:
    """The doc comment of a property: what it takes, and the LVGL function it sets."""
    takes = "; then ".join(take_text(take, enums) for take in member["takes"])
    lines = [f"  /// Takes {takes}."]
    if "sets" in member:
        lines.append(f"  /// Sets {member['sets']}.")
    return "\n".join(lines)


def event_enumerator(member_name: str) -> str:
    """`on_clicked` -> `LV_EVENT_CLICKED`."""
    return "LV_EVENT_" + member_name.removeprefix("on_").upper()


def style_part(member_name: str) -> str:
    """`style_pressed` -> `pressed`; `style` -> the widget itself."""
    part = member_name.removeprefix("style").removeprefix("_")
    return f"while it is {part}, or of its {part}" if part else "itself"


def props_member(member: dict, enums: dict) -> str:
    """One member of a props struct, with its doc comment."""
    kind, name = member.get("kind", "property"), member["name"]
    if kind == "style":
        return f"  /// The style of the widget {style_part(name)}: design tokens, or a named lv::style.\n  style_t {name}{{}};"
    if kind == "event":
        return (f"  /// Called on {event_enumerator(name)}: a function, or a lambda without captures.\n"
                f"  /// It takes nothing, or an lv::event.\n  lv::handler {name}{{}};")
    if kind == "id":
        return '  /// A name for this node: root->get<"name">() is its view after lv::mount.\n  id_name id{};'
    return f"{property_comment(member, enums)}\n  {member_type(member)} {name}{{}};"


def enum_struct(enum: str, names: list[str]) -> str:
    quoted = ", ".join(f'"{name}"' for name in names)
    name = enum_names_type(enum).removeprefix("stub::")
    return (f"/// The names of {enum}: {names_text(names)}.\n"
            f"struct {name} {{\n"
            f"  using type = {enum};\n"
            f"  static constexpr bool has(std::string_view name) {{ return detail::is_one_of(name, {{{quoted}}}); }}\n"
            f"}};")


def members_of(reference: dict, widget: str) -> list[dict]:
    """The members of props<widget>, in alphabetical order: its own win over the base's."""
    own = reference["widgets"].get(widget, [])
    own_names = {member["name"] for member in own}
    kept = [member for member in reference["base"] if member["name"] not in own_names] + own
    return sorted(kept, key=lambda member: member["name"])


def widgets_of(reference: dict) -> list[str]:
    return ["obj"] + [widget for widget in reference["widgets"] if widget != "obj"]


def style_struct(reference: dict) -> str:
    members = "\n".join(f"{property_comment(member, reference['enums'])}\n  {member_type(member)} {member['name']}{{}};"
                        for member in reference["style"])
    return ("/// A style: one member for each style property of LVGL, in alphabetical order.\n"
            "/// A design value takes a design token or lv::raw(...).\n"
            f"struct style_t {{\n{members}\n}};")


def props_struct(reference: dict, widget: str) -> str:
    members = "\n".join(props_member(member, reference["enums"]) for member in members_of(reference, widget))
    name = identifier(widget)
    return (f'/// The properties of the LVGL widget "{widget}", in alphabetical order.\n'
            f"struct props_{name} {{\n{members}\n}};\n"
            f'template <>\nstruct props_for<"{widget}"> {{\n  using type = props_{name};\n}};')


def animatable_type(member: dict) -> str | None:
    """The type of a property that lv::animate can run: one number, or nothing."""
    takes = member.get("takes", [])
    if member.get("kind", "property") != "property" or len(takes) != 1 or "enum" in takes[0]:
        return None
    unit = value_type(takes[0]["write"])
    return unit if unit in ANIMATABLE else None


def animatable_struct(reference: dict, widget: str) -> str:
    lines = []
    for member in members_of(reference, widget):
        unit = animatable_type(member)
        if unit is not None:
            lines.append(f"  /// From and to: {member['takes'][0]['write']}. Animates {member['sets']}.\n"
                         f"  lv::from_to<{unit}> {member['name']}{{}};")
    name = identifier(widget)
    members = "\n".join(lines)
    return (f'/// The properties of "{widget}" that lv::animate can run, each with its two ends.\n'
            f"struct animatable_{name} {{\n{members}\n}};\n"
            f'template <>\nstruct animatable_for<"{widget}"> {{\n  using type = animatable_{name};\n}};')


def getter(widget: str, member: dict) -> str | None:
    """Which LVGL function view<widget>.get<"name">() calls, if the property has a setter."""
    sets, name = member.get("sets", ""), member["name"]
    if not sets.endswith(f"_set_{name}"):
        return None
    function = sets.removesuffix(f"_set_{name}") + f"_get_{name}"
    unit = animatable_type(member)
    result = f"decltype({function}(o, a...))"
    if unit in ANGLES:
        result = f"unit_of<{unit}, {result}>"
    return (f'template <>\nstruct getter<"{widget}", "{name}"> {{\n'
            f"  template <class O, class... A>\n  static auto call(O* o, A&&... a) -> {result};\n}};")


def getters(reference: dict) -> list[str]:
    found = [getter("obj", member) for member in reference["base"]]
    for widget, members in reference["widgets"].items():
        found += [getter(widget, member) for member in members]
    return [text for text in found if text is not None]


def events_struct(reference: dict) -> str:
    lines = []
    for member in reference["base"]:
        if member.get("kind") != "event":
            continue
        code = event_enumerator(member["name"])
        name = "gone" if code == "LV_EVENT_DELETE" else member["name"].removeprefix("on_")
        lines.append(f"  /// {code}: co_await it, or give it to lv::first with .then(handler).\n"
                     f"  lv::event_ref<{code}> {name};")
    members = "\n".join(lines)
    return f"/// The events of a widget by name: what view.next() returns.\nstruct events {{\n{members}\n}};"


def theme_member(widget: str) -> str:
    return "toggle" if widget == "switch" else identifier(widget)


def theme_structs(reference: dict) -> str:
    styles = "\n".join(f"  /// The style of every such widget {style_part(member['name'])}.\n  style_t {member['name']}{{}};"
                       for member in reference["base"] if member.get("kind") == "style")
    kinds = sorted({theme_member(widget): widget for widget in widgets_of(reference)}.items())
    looks = [f'  /// The styles of every "{widget}" widget.\n  look {name}{{}};' for name, widget in kinds]
    looks.append("  /// The styles of a widget without a parent: a screen.\n  look screen{};")
    members = "\n".join(sorted(looks, key=lambda text: text.rsplit(" ", 1)[1]))
    return ("/// What a theme says about one kind of widget: a style for the widget, and one\n"
            "/// for each of its parts and states.\n"
            f"struct look {{\n{styles}\n}};\n\n"
            "/// What lv::theme takes: one member for each kind of widget, in alphabetical order.\n"
            f"struct theme_spec {{\n{members}\n}};")


def header(reference: dict, source: str) -> str:
    """The text of lv/generated.hpp for the reference data `reference`, read from `source`."""
    enums = [enum_struct(enum, names) for enum, names in reference["enums"].items()]
    widgets = widgets_of(reference)
    parts = [
        "#pragma once",
        f"// Written by tools/editor/generate.py from {source} (LVGL {reference['lvgl']}).\n"
        "// For editors only: it says what reflection generates in a real build. Do not edit.",
        "#include <lv/events.hpp>\n#include <lv/stub.hpp>\n#include <lv/values.hpp>\n\n#include <string_view>",
        "namespace lv::stub {",
        *enums,
        style_struct(reference),
        *[props_struct(reference, widget) for widget in widgets],
        *[animatable_struct(reference, widget) for widget in widgets],
        *getters(reference),
        events_struct(reference),
        theme_structs(reference),
        "}  // namespace lv::stub",
    ]
    return "\n\n".join(parts) + "\n"
