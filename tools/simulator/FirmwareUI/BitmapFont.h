#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <cstdint>
#include <unordered_map>
#include <vector>

class BitmapFont {
public:
  struct Glyph {
    int width = 0;
    int height = 0;
    int xOffset = 0;
    int yOffset = 0;
    int deltaX = 0;
    std::vector<std::uint16_t> rows{};
  };

  BitmapFont();
  explicit BitmapFont(const std::uint8_t *fontData, int spacing = 1);

  int Width() const { return width_; }
  int Height() const { return height_; }
  int Spacing() const { return spacing_; }

  const Glyph &GetGlyph(char character) const;
  const Glyph &GetGlyph(std::uint16_t encoding) const;

  static BitmapFont CreateU8g2_4x6();
  static BitmapFont CreateU8g2_5x8();
  static BitmapFont CreateU8g2_6x10();

private:
  struct FontInfo {
    std::uint8_t bitsPer0 = 0;
    std::uint8_t bitsPer1 = 0;
    std::uint8_t bitsPerCharWidth = 0;
    std::uint8_t bitsPerCharHeight = 0;
    std::uint8_t bitsPerCharX = 0;
    std::uint8_t bitsPerCharY = 0;
    std::uint8_t bitsPerDeltaX = 0;
    int maxCharWidth = 0;
    int maxCharHeight = 0;
    std::uint16_t startPosUpperA = 0;
    std::uint16_t startPosLowerA = 0;
    std::uint16_t startPosUnicode = 0;
  };

  static std::uint8_t ReadUnsignedBits(const std::uint8_t *&decodePtr,
                                       std::uint8_t &decodeBitPos,
                                       std::uint8_t count);
  static int ReadSignedBits(const std::uint8_t *&decodePtr,
                            std::uint8_t &decodeBitPos,
                            std::uint8_t count);
  static std::uint16_t ReadWord(const std::uint8_t *fontData, int offset);

  const std::uint8_t *FindGlyphData(std::uint16_t encoding) const;
  Glyph DecodeGlyph(std::uint16_t encoding) const;
  Glyph DecodeMissingGlyph() const;
  void DecodeRunLength(std::uint8_t runLength, bool isForeground, Glyph &glyph,
                       int &localX, int &localY) const;
  bool HasFontData() const { return fontData_ != nullptr; }

  const std::uint8_t *fontData_ = nullptr;
  FontInfo info_{};
  int width_ = 0;
  int height_ = 0;
  int spacing_ = 1;
  Glyph fallbackGlyph_{};
  mutable std::unordered_map<std::uint16_t, Glyph> glyphCache_{};
};

#endif
