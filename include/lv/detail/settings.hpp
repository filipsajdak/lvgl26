#pragma once

// The project's settings as the library reads them: the settings file when the project names
// one, otherwise lv::cfg. See lv/config.hpp.
//
// A mistake in the file is reported by a static_assert with a computed message, which GCC
// prints once; a message from a failed constant evaluation it prints three times. So the
// reader returns its first mistake as text instead of stopping where it finds it.

#include <lv/config.hpp>
#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/nearest.hpp>
#include <lv/json.hpp>

#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace lv::detail {

// The settings tag, named through a template parameter so that the settings are read where
// a template is used and not where its header is.
template <class...>
struct settings {
  using tag = override;
};

// Where a value is: the file, and the keys from its top ("allow.threads").
struct settings_place {
  std::string_view file;  // the name the project gave its file: static storage
  std::string path;
};

consteval settings_place deeper(const settings_place& at, std::string_view key) {
  return {at.file, at.path.empty() ? std::string{key} : at.path + "." + str(key)};
}

consteval std::string section_name(const settings_place& at) {
  return at.path.empty() ? std::string{"the file"} : at.path;
}

// The message of a mistake at `at`. An empty text is no mistake.
consteval std::string mistake(const settings_place& at, std::string_view what) {
  return join({"lv: ", at.file, ": ", what});
}

// A limit: a struct with a value and a table of units.
template <class T>
concept settings_amount = requires { T::units; T{}.value; };

// A section: a struct of settings.
template <class T>
concept settings_section = std::is_class_v<T> && !settings_amount<T>;

template <settings_section S>
consteval std::vector<std::string> key_names() {
  std::vector<std::string> out;
  for (auto m : std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()))
    out.push_back(str(std::meta::identifier_of(m)));
  return out;
}

template <settings_section S>
consteval std::string unknown_key(std::string_view key, const settings_place& at) {
  const std::string where = at.path.empty() ? std::string{} : " in " + at.path;
  return mistake(at, "unknown key '" + str(key) + "'" + where + "." + did_you_mean(key, key_names<S>()) +
                         " The keys of " + section_name(at) + ": " + joined_names(key_names<S>()) + ".");
}

consteval std::string read_into(bool& out, json::value v, const settings_place& at) {
  if (!v.is(json::kind::boolean)) return mistake(at, at.path + " takes true or false, not " + str(v.text()) + ".");
  out = v.boolean();
  return {};
}

template <settings_amount Q>
consteval std::vector<std::string> unit_names() {
  std::vector<std::string> names;
  for (const settings_unit& u : Q::units) names.push_back(u.name);
  return names;
}

// How many of the base unit `unit` is, or 0 if it is no unit of Q.
template <settings_amount Q>
consteval std::size_t unit_factor(std::string_view unit) {
  for (const settings_unit& u : Q::units)
    if (unit == u.name) return u.factor;
  return 0;
}

// The largest whole number a double holds exactly: 2^53.
inline constexpr double exact_whole_limit = 9007199254740992.0;

consteval std::string whole_amount(std::size_t& out, json::value v, std::string_view unit, std::size_t factor,
                                   std::string_view base, const settings_place& at) {
  const std::string given = at.path + " is " + str(v.text()) + " " + str(unit);
  const double amount = v.number() * static_cast<double>(factor);
  if (amount < 0) return mistake(at, given + ", and a limit cannot be negative.");
  if (amount >= exact_whole_limit) return mistake(at, given + ", more than a limit can hold.");
  out = static_cast<std::size_t>(amount);
  if (static_cast<double>(out) != amount) return mistake(at, given + ", which is no whole number of " + str(base) + ".");
  return {};
}

consteval bool is_amount_object(json::value v) {
  return v.is(json::kind::object) && v.size() == 2 && v["value"].is(json::kind::number) &&
         v["unit"].is(json::kind::string);
}

template <settings_amount Q>
consteval std::string read_into(Q& out, json::value v, const settings_place& at) {
  const std::string units = joined_names(unit_names<Q>());
  if (!is_amount_object(v))
    return mistake(at, at.path + " takes a value and a unit, as {\"value\": 4, \"unit\": \"" + str(Q::units[0].name) +
                           "\"}, not " + str(v.text()) + ". Its units: " + units + ".");
  const std::string_view unit = v["unit"].string();
  const std::size_t factor = unit_factor<Q>(unit);
  if (factor == 0)
    return mistake(at, at.path + " has the unit '" + str(unit) + "'." + did_you_mean(unit, unit_names<Q>()) +
                           " Its units: " + units + ".");
  return whole_amount(out.value, v["value"], unit, factor, Q::units[0].name, at);
}

template <settings_section S>
consteval std::string read_member(S& out, json::member m, const settings_place& at);

template <settings_section S>
consteval std::string read_into(S& out, json::value v, const settings_place& at) {
  if (!v.is(json::kind::object))
    return mistake(at, section_name(at) + " takes an object of settings, not " + str(v.text()) +
                           ". Its keys: " + joined_names(key_names<S>()) + ".");
  for (const json::member m : v.members())
    if (std::string wrong = read_member(out, m, at); !wrong.empty()) return wrong;
  return {};
}

template <settings_section S>
consteval std::string read_member(S& out, json::member m, const settings_place& at) {
  if (at.path.empty() && m.key == "$schema") return {};  // for editors; not a setting
  template for (constexpr auto member : std::define_static_array(
                    std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()))) {
    if (std::meta::identifier_of(member) == m.key)
      return read_into(out.[:member:], json::value{m.text}, deeper(at, m.key));
  }
  return unknown_key<S>(m.key, at);
}

// The settings of a document, from the defaults of S, or the first mistake in it.
template <settings_section S>
struct settings_read {
  S value;
  std::string_view mistake;  // empty, or a static string
};

template <settings_section S>
consteval settings_read<S> read_settings(settings_text file) {
  const settings_place top{file.name, ""};
  const auto document = json::value::parse(std::string_view{file.text, file.size});
  if (!document) return {S{}, std::define_static_string(mistake(top, "the file is no well-formed JSON."))};
  S out{};
  const std::string wrong = read_into(out, *document, top);
  return {out, std::define_static_string(wrong)};
}

// The settings of the document `File`; a mistake in it is a compile error that names it.
template <settings_section S, const settings_text& File>
consteval S settings_from() {
  constexpr settings_read<S> read = read_settings<S>(File);
  static_assert(read.mistake.empty(), read.mistake);
  return read.value;
}

template <class O>
consteval config project_settings() {
  if constexpr (settings_file<O>.size == 0)
    return cfg<O>;
  else
    return settings_from<config, settings_file<O>>();
}

// The settings of the project, by the settings tag O: its file, or lv::cfg when it has none.
template <class O>
inline constexpr config project = project_settings<O>();

consteval std::string_view feature_words(std::string_view key) {
  return key == "floating_point" ? std::string_view{"floating point"} : key;
}

consteval std::string_view feature_pronoun(std::string_view key) {
  return key == "threads" || key == "files" ? "them" : "it";
}

// The message for `who`, which needs the feature `Key` of lv::allow_settings, or nothing when
// the project allows it.
template <std::meta::info Key, class O>
consteval std::string_view refusal(std::string_view who, std::string_view why) {
  if (project<O>.allow.[:Key:]) return {};
  const std::string_view key = std::meta::identifier_of(Key);
  const std::string where = settings_file<O>.size == 0 ? std::string{} : " in " + str(settings_file<O>.name);
  return std::define_static_string(join({"lv: ", who, " needs ", feature_words(key),
                                         ", and this project does not allow ", feature_pronoun(key), " (allow.",
                                         key, where, ").", why}));
}

// A compile error where `Who` is used, if the project does not allow the feature `Key`:
// "lv: lv::ui_scheduler needs threads, and this project does not allow them (allow.threads)."
// `Why` follows when the reason is not plain.
template <std::meta::info Key, class O, fixed Who, fixed Why = "">
consteval void require() {
  constexpr std::string_view message = refusal<Key, O>(Who.sv(), Why.sv());
  static_assert(message.empty(), message);
}

}  // namespace lv::detail
