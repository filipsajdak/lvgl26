// Runs a libFuzzer target once on each file given, without libFuzzer. This lets
// GCC build the target and ctest replay its corpus under every preset.

#include <cstdint>
#include <cstdio>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string bytes;
    if (std::FILE* f = std::fopen(argv[i], "rb")) {
      char buffer[4096];
      for (std::size_t n; (n = std::fread(buffer, 1, sizeof buffer, f)) > 0;) bytes.append(buffer, n);
      std::fclose(f);
    }
    LLVMFuzzerTestOneInput(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
  }
  std::printf("%d inputs\n", argc - 1);
  return 0;
}
