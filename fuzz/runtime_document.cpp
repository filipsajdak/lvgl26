// Fuzz target for runtime documents and patches (include/lv/runtime.hpp).
//
// This code needs reflection, so it is built with GCC. It is a plain program that
// reads one input file, which is how AFL++ drives it:
//   afl-g++-fast (or g++-16) ... fuzz/runtime_document.cpp -o build/fuzz/runtime_document
//   afl-fuzz -i fuzz/corpus/document -o build/fuzz/out -- build/fuzz/runtime_document @@
//
// The input is a document, optionally followed by a line with only "---" and a
// patch. The program loads the document and applies the patch. Whatever the input
// is, it must end without a crash or a sanitizer report, and a rejected document
// must leave nothing on the screen.

#include <lv/lv.hpp>

#include <cstdio>
#include <string>
#include <string_view>

namespace {

using ui = lv::runtime<"obj", "label", "button", "arc", "bar", "slider", "switch", "checkbox",
                       "textarea", "dropdown", "roller">;

void require(bool ok) {
  if (!ok) __builtin_trap();
}

std::string read_file(const char* path) {
  std::string text;
  if (std::FILE* f = std::fopen(path, "rb")) {
    char buffer[4096];
    for (std::size_t n; (n = std::fread(buffer, 1, sizeof buffer, f)) > 0;) text.append(buffer, n);
    std::fclose(f);
  }
  return text;
}

void run(std::string_view input) {
  constexpr std::string_view separator = "\n---\n";
  const auto cut = input.find(separator);
  const auto document = input.substr(0, cut);
  const auto patch = cut == std::string_view::npos ? std::string_view{}
                                                   : input.substr(cut + separator.size());

  const auto screen = lv::screen_active();
  const auto before = lv_obj_get_child_count(screen.raw());
  {
    auto loaded = ui::load(document, screen);
    if (!loaded) {
      require(lv_obj_get_child_count(screen.raw()) == before);  // nothing stays behind
      return;
    }
    require(static_cast<bool>(loaded->root()));
    if (!patch.empty()) (void)loaded->apply(patch);
    lv_obj_update_layout(screen.raw());  // lay out what was built
    lv_refr_now(nullptr);                // and draw it
  }
  require(lv_obj_get_child_count(screen.raw()) == before);  // the screen is deleted with its owner
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: runtime_document <input file>\n");
    return 2;
  }
  const std::string input = read_file(argv[1]);
  lv_init();
  lv_test_display_create(320, 240);
  run(input);
  lv_deinit();
  return 0;
}
