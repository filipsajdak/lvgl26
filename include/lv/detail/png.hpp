#pragma once

// A small PNG encoder for screenshots. Plain C++23: no LVGL, no reflection.
//
// LVGL's public API has a PNG decoder but no encoder, so the file is written here.
// The pixels are compressed with deflate's fixed Huffman codes and one kind of
// match only: "the same byte as four bytes ago", which is a run of equal pixels.
// A user interface is mostly flat areas, so this is many times smaller than
// storing the pixels, and every PNG reader can read it.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace lv::detail::png {

using bytes = std::vector<std::uint8_t>;

// Bits packed into bytes, least significant bit first, as deflate wants them.
class bit_writer {
  bytes& out_;
  std::uint32_t pending_ = 0;
  int count_ = 0;

 public:
  explicit bit_writer(bytes& out) : out_(out) {}

  // The lowest `count` bits of `value`, lowest bit first.
  void bits(std::uint32_t value, int count) {
    pending_ |= value << count_;
    count_ += count;
    for (; count_ >= 8; count_ -= 8, pending_ >>= 8) out_.push_back(static_cast<std::uint8_t>(pending_));
  }

  // A Huffman code of `length` bits, highest bit first.
  void code(std::uint32_t value, int length) {
    for (int i = length - 1; i >= 0; --i) bits((value >> i) & 1U, 1);
  }

  // Fills the last byte with zero bits.
  void flush() {
    if (count_ > 0) bits(0, 8 - count_);
  }
};

// One byte as it is, in deflate's fixed code.
inline void literal(bit_writer& w, std::uint8_t byte) {
  if (byte < 144)
    w.code(0x30U + byte, 8);
  else
    w.code(0x190U + (byte - 144U), 9);
}

// "Repeat the bytes from four back, `length` times". Only the lengths that need
// no extra bits are used: 3 to 10, and 258.
inline void match(bit_writer& w, std::size_t length) {
  if (length == 258)
    w.code(0xC5, 8);  // symbol 285
  else
    w.code(static_cast<std::uint32_t>(length - 2), 7);  // symbols 257 to 264
  w.code(3, 5);  // distance 4
}

inline constexpr std::size_t pixel = 4;  // bytes in a pixel: the distance of a match

// How many bytes from data[at] on are equal to the byte one pixel before them.
inline std::size_t run_at(std::span<const std::uint8_t> data, std::size_t at) {
  if (at < pixel) return 0;
  std::size_t n = 0;
  while (at + n < data.size() && data[at + n] == data[at + n - pixel]) ++n;
  return n;
}

// Writes a run of `length` repeated bytes as matches. Returns how many bytes of it
// are left over (fewer than three, which a match cannot express).
inline std::size_t write_run(bit_writer& w, std::size_t length) {
  for (; length >= 258; length -= 258) match(w, 258);
  while (length >= 3) {
    const std::size_t part = length < 10 ? length : 10;
    match(w, part);
    length -= part;
  }
  return length;
}

// `data` as one deflate block, appended to `out`.
inline void deflate(std::span<const std::uint8_t> data, bytes& out) {
  bit_writer w{out};
  w.bits(1, 1);  // the last block
  w.bits(1, 2);  // fixed Huffman codes
  for (std::size_t i = 0; i < data.size();) {
    const std::size_t run = run_at(data, i);
    if (run >= 3) {
      i += run - write_run(w, run);
    } else {
      literal(w, data[i++]);
    }
  }
  w.code(0, 7);  // end of block
  w.flush();
}

inline std::uint32_t adler32(std::span<const std::uint8_t> data) {
  std::uint32_t a = 1;
  std::uint32_t b = 0;
  for (const std::uint8_t byte : data) {
    a = (a + byte) % 65521U;
    b = (b + a) % 65521U;
  }
  return (b << 16) | a;
}

inline constexpr std::array<std::uint32_t, 256> crc_table = [] {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t n = 0; n < 256; ++n) {
    std::uint32_t c = n;
    for (int k = 0; k < 8; ++k) c = (c & 1U) ? 0xEDB88320U ^ (c >> 1) : c >> 1;
    table[n] = c;
  }
  return table;
}();

// The CRC of PNG, continued from `crc` (start with 0xFFFFFFFF, invert at the end).
inline std::uint32_t crc32(std::uint32_t crc, std::span<const std::uint8_t> data) {
  for (const std::uint8_t byte : data) crc = crc_table[(crc ^ byte) & 0xFFU] ^ (crc >> 8);
  return crc;
}

inline void put_u32(bytes& out, std::uint32_t v) {
  for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(v >> shift));
}

// One chunk of a PNG file: length, type, data, CRC of type and data.
inline void chunk(bytes& out, std::string_view type, std::span<const std::uint8_t> data) {
  put_u32(out, static_cast<std::uint32_t>(data.size()));
  const std::size_t start = out.size();
  out.insert(out.end(), type.begin(), type.end());
  out.insert(out.end(), data.begin(), data.end());
  put_u32(out, ~crc32(0xFFFFFFFFU, std::span{out}.subspan(start)));
}

// What PNG compresses: every row of pixels after a zero byte ("no filter").
// `pixels` holds rows of `stride` bytes; a pixel is blue, green, red, alpha.
inline bytes scanlines(const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height,
                       std::size_t stride) {
  bytes raw;
  raw.reserve((std::size_t{width} * pixel + 1) * height);
  for (std::uint32_t y = 0; y < height; ++y) {
    const std::uint8_t* row = pixels + stride * y;
    raw.push_back(0);
    for (std::uint32_t x = 0; x < width; ++x, row += pixel)
      raw.insert(raw.end(), {row[2], row[1], row[0], row[3]});  // red, green, blue, alpha
  }
  return raw;
}

// A PNG file of 8-bit RGBA from rows of blue, green, red, alpha pixels.
inline bytes encode(const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height,
                    std::size_t stride) {
  bytes file{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

  bytes header;
  put_u32(header, width);
  put_u32(header, height);
  header.insert(header.end(), {8, 6, 0, 0, 0});  // 8 bits, RGBA, deflate, no filter, no interlace
  chunk(file, "IHDR", header);

  const bytes raw = scanlines(pixels, width, height, stride);
  bytes data{0x78, 0x01};  // a zlib stream
  deflate(raw, data);
  put_u32(data, adler32(raw));
  chunk(file, "IDAT", data);

  chunk(file, "IEND", {});
  return file;
}

}  // namespace lv::detail::png
