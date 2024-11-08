#pragma once

#include <filesystem>
#include <vector>

namespace Rastery {

enum class ColorSpace {
    Linear,  ///< linear color space
    sRGB,    ///< sRGB color space, gamma corrected
};

/**
 * @brief abstract image file interface
 */
class Image {
   public:
    Image(size_t width, size_t height, int channels,
          ColorSpace colorSpace = ColorSpace::Linear)
        : m_width(int(width)),
          m_height(int(height)),
          m_channels(channels),
          m_colorSpace(colorSpace),
          m_rawData(channels, std::vector<float>(width * height, 0.f)) {}
    Image(const Image& other) = delete;
    Image(Image&& other) noexcept
        : m_width(other.m_width),
          m_height(other.m_height),
          m_channels(other.m_channels),
          m_rawData(std::move(other.m_rawData)) {}
    void resize(size_t width, size_t height);
    /** Resize the number of channels of the image, keep the original data and
     * fill the new channels with 0.
     */
    void resizeChannels(int channels) {
        this->m_channels = channels;
        m_rawData.resize(channels,
                         std::vector<float>(size_t(m_width * m_height), 0.f));
    }

    // Image properties
    [[nodiscard]] auto getWidth() const { return m_width; }
    [[nodiscard]] auto getHeight() const { return m_height; }
    [[nodiscard]] auto getChannels() const { return m_channels; }
    [[nodiscard]] auto getArea() const { return m_width * m_height; }
    [[nodiscard]] const auto& getRawData() const { return m_rawData; }

    // Pixel accessing
    [[nodiscard]] float getPixel(int x, int y, int channels) const {
        return m_rawData[channels][x + y * m_width];
    }
    [[nodiscard]] float getPixel(int index, int channels) const {
        return m_rawData[channels][index];
    }
    float& getPixel(int x, int y, int channels) {
        return m_rawData[channels][x + y * m_width];
    }
    float& getPixel(int index, int channels) {
        return m_rawData[channels][index];
    }

    void setPixel(int x, int y, int channels, float value);
    void setPixel(int index, int channels, float value);

    void writeEXR(const std::filesystem::path& filename) const;
    void readEXR(const std::filesystem::path& filename);

    void writePNG(const std::filesystem::path& filename,
                  bool toSrgb = true) const;
    void readPNG(const std::filesystem::path& filename);

    /** Construct an image object from a file.
     */
    static Image load(const std::filesystem::path& filename);

   private:
    int m_width;
    int m_height;
    int m_channels;

    ColorSpace m_colorSpace = ColorSpace::Linear;  ///< color space of the image
    std::vector<std::vector<float>>
        m_rawData;  // raw data layers splitted by channels, width x height x
                    // channels, m_rawData[channel][x + y * width]
};

}  // namespace Rastery