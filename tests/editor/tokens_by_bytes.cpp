// A token file that the editor's file overlay does not cover: the stub of lv::tokens
// then finds its struct by the bytes of the file. Read by Clang with the editor stubs
// only (the test editor_tokens_by_bytes), which embeds a copy of tests/data/tokens.json
// under a name the stub generator has not seen.
#include <lv/lv.hpp>

#include <concepts>

constexpr char tokens_json[] = {
#embed "unseen_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

static_assert(std::same_as<decltype(tok.color.primary), lv::token<lv_color_t>>);  // C API
constexpr auto card = lv::style({.bg_color = tok.color.card});
