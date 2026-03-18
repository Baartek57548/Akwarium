#include "BitmapFont.h"

#include <algorithm>
#include <cstddef>
#include <utility>

extern const std::uint8_t u8g2_font_5x8_tr[];
extern const std::uint8_t u8g2_font_6x10_tr[];
extern const std::uint8_t u8g2_font_4x6_tr[];

namespace {

constexpr int FontHeaderSize = 23;
constexpr std::uint16_t GlyphRowBitsLimit = 16;

} // namespace

BitmapFont::BitmapFont() {
  width_ = 5;
  height_ = 8;
  spacing_ = 1;
  fallbackGlyph_ = DecodeMissingGlyph();
}

BitmapFont::BitmapFont(const std::uint8_t *fontData, int spacing)
    : fontData_(fontData), spacing_(std::max(0, spacing)) {
  if (!HasFontData()) {
    width_ = 5;
    height_ = 8;
    fallbackGlyph_ = DecodeMissingGlyph();
    return;
  }

  info_.bitsPer0 = fontData_[2];
  info_.bitsPer1 = fontData_[3];
  info_.bitsPerCharWidth = fontData_[4];
  info_.bitsPerCharHeight = fontData_[5];
  info_.bitsPerCharX = fontData_[6];
  info_.bitsPerCharY = fontData_[7];
  info_.bitsPerDeltaX = fontData_[8];

  info_.maxCharWidth = static_cast<int>(static_cast<std::int8_t>(fontData_[9]));
  info_.maxCharHeight = static_cast<int>(static_cast<std::int8_t>(fontData_[10]));
  info_.startPosUpperA = ReadWord(fontData_, 17);
  info_.startPosLowerA = ReadWord(fontData_, 19);
  info_.startPosUnicode = ReadWord(fontData_, 21);

  width_ = std::max(1, info_.maxCharWidth);
  height_ = std::max(1, info_.maxCharHeight);
  fallbackGlyph_ = DecodeGlyph(static_cast<std::uint16_t>('?'));
  if (fallbackGlyph_.width <= 0 || fallbackGlyph_.height <= 0) {
    fallbackGlyph_ = DecodeMissingGlyph();
  }
}

const BitmapFont::Glyph &BitmapFont::GetGlyph(char character) const {
  const auto encoding = static_cast<std::uint16_t>(static_cast<unsigned char>(character));
  return GetGlyph(encoding);
}

const BitmapFont::Glyph &BitmapFont::GetGlyph(std::uint16_t encoding) const {
  const auto found = glyphCache_.find(encoding);
  if (found != glyphCache_.end()) {
    return found->second;
  }

  Glyph decoded = DecodeGlyph(encoding);
  if (decoded.width <= 0 || decoded.height <= 0 || decoded.rows.empty()) {
    decoded = fallbackGlyph_;
  }

  const auto inserted = glyphCache_.emplace(encoding, std::move(decoded));
  return inserted.first->second;
}

BitmapFont BitmapFont::CreateU8g2_4x6() { return BitmapFont(u8g2_font_4x6_tr, 1); }

BitmapFont BitmapFont::CreateU8g2_5x8() { return BitmapFont(u8g2_font_5x8_tr, 1); }

BitmapFont BitmapFont::CreateU8g2_6x10() { return BitmapFont(u8g2_font_6x10_tr, 1); }

std::uint8_t BitmapFont::ReadUnsignedBits(const std::uint8_t *&decodePtr,
                                          std::uint8_t &decodeBitPos,
                                          std::uint8_t count) {
  if (count == 0) {
    return 0;
  }

  std::uint8_t value = static_cast<std::uint8_t>(*decodePtr >> decodeBitPos);
  std::uint8_t bitPosPlusCount = static_cast<std::uint8_t>(decodeBitPos + count);
  if (bitPosPlusCount >= 8U) {
    const std::uint8_t shift = static_cast<std::uint8_t>(8U - decodeBitPos);
    decodePtr++;
    value = static_cast<std::uint8_t>(value | static_cast<std::uint8_t>(*decodePtr << shift));
    bitPosPlusCount = static_cast<std::uint8_t>(bitPosPlusCount - 8U);
  }

  const std::uint8_t mask = static_cast<std::uint8_t>((1U << count) - 1U);
  value = static_cast<std::uint8_t>(value & mask);
  decodeBitPos = bitPosPlusCount;
  return value;
}

int BitmapFont::ReadSignedBits(const std::uint8_t *&decodePtr,
                               std::uint8_t &decodeBitPos, std::uint8_t count) {
  if (count == 0) {
    return 0;
  }

  const auto value = static_cast<int>(ReadUnsignedBits(decodePtr, decodeBitPos, count));
  const auto delta = static_cast<int>(1 << (count - 1));
  return value - delta;
}

std::uint16_t BitmapFont::ReadWord(const std::uint8_t *fontData, int offset) {
  const auto high = static_cast<std::uint16_t>(fontData[offset]);
  const auto low = static_cast<std::uint16_t>(fontData[offset + 1]);
  return static_cast<std::uint16_t>((high << 8) | low);
}

const std::uint8_t *BitmapFont::FindGlyphData(std::uint16_t encoding) const {
  if (!HasFontData()) {
    return nullptr;
  }

  const std::uint8_t *cursor = fontData_ + FontHeaderSize;
  if (encoding <= 255) {
    if (encoding >= static_cast<std::uint16_t>('a')) {
      cursor += info_.startPosLowerA;
    } else if (encoding >= static_cast<std::uint16_t>('A')) {
      cursor += info_.startPosUpperA;
    }

    while (cursor[1] != 0) {
      if (cursor[0] == encoding) {
        return cursor + 2;
      }
      cursor += cursor[1];
    }
    return nullptr;
  }

  if (info_.startPosUnicode == 0) {
    return nullptr;
  }

  const std::uint8_t *unicodeCursor = fontData_ + FontHeaderSize + info_.startPosUnicode;
  const std::uint8_t *unicodeLookupTable = unicodeCursor;
  std::uint16_t sectionMaxEncoding = 0;

  for (int i = 0; i < 512; i++) {
    const auto sectionOffset = ReadWord(unicodeLookupTable, 0);
    if (sectionOffset == 0) {
      return nullptr;
    }

    unicodeCursor += sectionOffset;
    sectionMaxEncoding = ReadWord(unicodeLookupTable, 2);
    unicodeLookupTable += 4;
    if (sectionMaxEncoding >= encoding) {
      break;
    }
  }

  if (sectionMaxEncoding < encoding) {
    return nullptr;
  }

  for (;;) {
    std::uint16_t current = static_cast<std::uint16_t>(unicodeCursor[0]);
    current = static_cast<std::uint16_t>((current << 8) | unicodeCursor[1]);
    if (current == 0) {
      break;
    }
    if (current == encoding) {
      return unicodeCursor + 3;
    }
    unicodeCursor += unicodeCursor[2];
  }

  return nullptr;
}

BitmapFont::Glyph BitmapFont::DecodeMissingGlyph() const {
  Glyph glyph;
  glyph.width = std::max(1, width_);
  glyph.height = std::max(1, height_);
  glyph.xOffset = 0;
  glyph.yOffset = 0;
  glyph.deltaX = glyph.width + spacing_;
  glyph.rows.assign(static_cast<std::size_t>(glyph.height), 0);

  const int maxDrawableWidth = std::min(glyph.width, static_cast<int>(GlyphRowBitsLimit));
  for (int y = 0; y < glyph.height; y++) {
    std::uint16_t row = 0;
    for (int x = 0; x < maxDrawableWidth; x++) {
      const bool border = (y == 0) || (y == glyph.height - 1) || (x == 0) || (x == maxDrawableWidth - 1);
      if (border || (x == (maxDrawableWidth / 2) && y >= glyph.height / 3 && y <= (glyph.height * 2) / 3)) {
        row |= static_cast<std::uint16_t>(1U << (maxDrawableWidth - 1 - x));
      }
    }
    glyph.rows[static_cast<std::size_t>(y)] = row;
  }

  return glyph;
}

void BitmapFont::DecodeRunLength(std::uint8_t runLength, bool isForeground, Glyph &glyph,
                                 int &localX, int &localY) const {
  if (runLength == 0 || glyph.width <= 0 || glyph.height <= 0) {
    return;
  }

  auto remaining = static_cast<int>(runLength);
  while (remaining > 0) {
    if (localY >= glyph.height) {
      return;
    }

    const auto remToRight = std::max(0, glyph.width - localX);
    if (remToRight <= 0) {
      localX = 0;
      localY++;
      continue;
    }

    const auto current = std::min(remaining, remToRight);
    if (isForeground) {
      const auto drawableWidth = std::min(glyph.width, static_cast<int>(GlyphRowBitsLimit));
      for (int pixel = 0; pixel < current; pixel++) {
        const int glyphX = localX + pixel;
        if (glyphX >= 0 && glyphX < drawableWidth) {
          glyph.rows[static_cast<std::size_t>(localY)] |=
              static_cast<std::uint16_t>(1U << (drawableWidth - 1 - glyphX));
        }
      }
    }

    remaining -= current;
    localX += current;

    if (localX >= glyph.width) {
      localX = 0;
      localY++;
    }
  }
}

BitmapFont::Glyph BitmapFont::DecodeGlyph(std::uint16_t encoding) const {
  if (!HasFontData()) {
    return DecodeMissingGlyph();
  }

  const auto glyphData = FindGlyphData(encoding);
  if (glyphData == nullptr) {
    return Glyph{};
  }

  const std::uint8_t *decodePtr = glyphData;
  std::uint8_t decodeBitPos = 0;

  Glyph glyph;
  glyph.width = static_cast<int>(ReadUnsignedBits(decodePtr, decodeBitPos, info_.bitsPerCharWidth));
  glyph.height = static_cast<int>(ReadUnsignedBits(decodePtr, decodeBitPos, info_.bitsPerCharHeight));
  glyph.xOffset = ReadSignedBits(decodePtr, decodeBitPos, info_.bitsPerCharX);
  glyph.yOffset = ReadSignedBits(decodePtr, decodeBitPos, info_.bitsPerCharY);
  glyph.deltaX = ReadSignedBits(decodePtr, decodeBitPos, info_.bitsPerDeltaX);

  if (glyph.width <= 0 || glyph.height <= 0) {
    glyph.width = std::max(1, width_);
    glyph.height = std::max(1, height_);
    glyph.deltaX = std::max(1, glyph.deltaX);
    return glyph;
  }

  glyph.rows.assign(static_cast<std::size_t>(glyph.height), 0);

  int localX = 0;
  int localY = 0;
  while (localY < glyph.height) {
    const auto zeroLen = ReadUnsignedBits(decodePtr, decodeBitPos, info_.bitsPer0);
    const auto oneLen = ReadUnsignedBits(decodePtr, decodeBitPos, info_.bitsPer1);

    do {
      DecodeRunLength(zeroLen, false, glyph, localX, localY);
      DecodeRunLength(oneLen, true, glyph, localX, localY);
    } while (ReadUnsignedBits(decodePtr, decodeBitPos, 1) != 0);
  }

  if (glyph.deltaX <= 0) {
    glyph.deltaX = glyph.width + spacing_;
  }

  return glyph;
}
