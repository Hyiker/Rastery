#include "Image.h"

#include <lodepng.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <span>

#include "Core/Error.h"
#include "Core/Math.h"
#include "Logger.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

namespace Rastery {
void Image::resize(size_t width, size_t height) {
    this->m_width = int(width);
    this->m_height = int(height);
    int area = int(width * height);
    std::for_each(m_rawData.begin(), m_rawData.end(),
                  [area](auto& layer) { layer.resize(area, 0.f); });
}

void Image::setPixel(int x, int y, int channels, float value) {
    setPixel(x + y * m_width, channels, value);
}
void Image::setPixel(int index, int channels, float value) {
    RASTERY_ASSERT(index < m_width * m_height);
    RASTERY_ASSERT(channels < this->m_channels);
    m_rawData[channels][index] = value;
}

void Image::writeEXR(const std::filesystem::path& filename) const {
    EXRHeader header;
    EXRImage image;
    InitEXRHeader(&header);
    InitEXRImage(&image);

    // Split RGBRGBRGB... into R, G and B layer
    RASTERY_ASSERT(m_channels >= 3);
    std::array<const float*, 3> layerPointer;
    layerPointer[0] = m_rawData[2].data();  // B
    layerPointer[1] = m_rawData[1].data();  // G
    layerPointer[2] = m_rawData[0].data();  // R

    image.images = reinterpret_cast<unsigned char**>(
        const_cast<float**>(layerPointer.data()));
    image.width = m_width;
    image.height = m_height;

    header.num_channels = 3;
    header.channels = static_cast<EXRChannelInfo*>(
        malloc(sizeof(EXRChannelInfo) * header.num_channels));
    // Must be (A)BGR order, since most of EXR viewers expect this channel
    // order.
    strncpy(header.channels[0].name, "B", 255);
    header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255);
    header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255);
    header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types =
        static_cast<int*>(malloc(sizeof(int) * header.num_channels));
    header.requested_pixel_types =
        static_cast<int*>(malloc(sizeof(int) * header.num_channels));
    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] =
            TINYEXR_PIXELTYPE_FLOAT;  // pixel type of input image
        header.requested_pixel_types[i] =
            TINYEXR_PIXELTYPE_HALF;  // pixel type of output image to be stored
                                     // in .EXR
    }

    const char* err = nullptr;  // or nullptr in C++11 or later.
    int ret =
        SaveEXRImageToFile(&image, &header, filename.string().c_str(), &err);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    if (ret != TINYEXR_SUCCESS) {
        logError("Saving EXR error: {}", err);

        FreeEXRErrorMessage(err);
        return;
    }
}

void Image::readEXR(const std::filesystem::path& filename) {
    logDebug("Loading EXR file from {}", filename.string());
    EXRVersion exrVersion;
    EXRImage exrImage;
    EXRHeader exrHeader;

    InitEXRHeader(&exrHeader);

    int ret = ParseEXRVersionFromFile(&exrVersion, filename.string().c_str());
    if (ret != TINYEXR_SUCCESS) {
        logError("EXR version parse error");
        return;
    }
    const char* err = nullptr;

    ret = ParseEXRHeaderFromFile(&exrHeader, &exrVersion,
                                 filename.string().c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err != nullptr) {
            logError("EXR header parse error: {}", err);
            FreeEXRErrorMessage(err);
        }
        return;
    }

    for (int i = 0; i < exrHeader.num_channels; i++) {
        exrHeader.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    InitEXRImage(&exrImage);
    ret = LoadEXRImageFromFile(&exrImage, &exrHeader, filename.string().c_str(),
                               &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err != nullptr) {
            logError("EXR image load error: {}", err);
            FreeEXRErrorMessage(err);
        }
        return;
    }

    // Assuming the image is RGB(A)
    RASTERY_ASSERT(exrHeader.num_channels >= 3);

    // mapping exr channels to rgb
    std::array<int, 3> channelMapping;
    for (int i = 0; i < 3; i++) {
        channelMapping[i] = -1;
        for (int j = 0; j < exrHeader.num_channels; j++) {
            if (exrHeader.channels[j].name[0] == "RGB"[i]) {
                channelMapping[i] = j;
                break;
            }
        }
        RASTERY_ASSERT(channelMapping[i] != -1);
    }

    resize(exrImage.width, exrImage.height);
    resizeChannels(exrHeader.num_channels);
    int area = getArea();

    for (int c = 0; c < 3; c++) {
        int exrChannel = channelMapping[c];
        std::span<const float> layer(
            reinterpret_cast<float*>(exrImage.images[exrChannel]), area);
        std::copy(layer.begin(), layer.end(), m_rawData[c].begin());
    }

    FreeEXRHeader(&exrHeader);
    FreeEXRImage(&exrImage);

    logDebug("Loaded EXR file from {}", filename.string());
}

void Image::writePNG(const std::filesystem::path& filename, bool toSrgb) const {
    lodepng::State state;

    switch (m_channels) {
        case 1:
            state.info_raw.colortype = LCT_GREY;
            state.info_raw.bitdepth = 8;

            state.info_png.color.colortype = LCT_GREY;
            state.info_png.color.bitdepth = 8;
            break;
        case 3:
            state.info_raw.colortype = LCT_RGB;
            state.info_raw.bitdepth = 8;

            state.info_png.color.colortype = LCT_RGB;
            state.info_png.color.bitdepth = 8;
            break;
        case 4:
            state.info_raw.colortype = LCT_RGBA;
            state.info_raw.bitdepth = 8;

            state.info_png.color.colortype = LCT_RGBA;
            state.info_png.color.bitdepth = 8;
            break;
        default:
            logFatal("Unsupported PNG write channel count: {}", m_channels);
            return;
    }
    auto hdrToSdr = [&](float value) {
        // Do color space conversion first
        if (toSrgb) {
            switch (m_colorSpace) {
                case ColorSpace::Linear:
                    value = std::pow(value, 1 / 2.2f);
                    break;
                case ColorSpace::sRGB:
                    break;
            }
            return saturate(value);
        } else {
            return value;
        }
    };
    // From RRR... GGG... BBB... to RGBRGBRGB...
    std::vector<unsigned char> image((size_t)m_width * m_height * m_channels);
    for (int c = 0; c < m_channels; c++) {
        for (int i = 0; i < m_width * m_height; i++) {
            image[i * m_channels + c] =
                (unsigned char)(hdrToSdr(m_rawData[c][i]) * 255.f);
        }
    }
    std::vector<unsigned char> buffer;  // Raw png data
    if (auto error = lodepng::encode(buffer, image, m_width, m_height, state);
        error) {
        logFatal("PNG encode error: {}", lodepng_error_text(error));
    } else {
        lodepng::save_file(buffer, filename.string());
    }
}

void Image::readPNG(const std::filesystem::path& filename) {
    std::vector<unsigned char> buffer, image;
    uint32_t width, height;
    lodepng::State state;

    if (auto error = lodepng::load_file(buffer, filename.string()); error) {
        logFatal("PNG load error: {}", lodepng_error_text(error));
        return;
    }

    if (auto error = lodepng::decode(image, width, height, state, buffer);
        error) {
        logFatal("PNG decode error: {}", lodepng_error_text(error));
        return;
    }

    switch (state.info_png.color.colortype) {
        case LCT_GREY:
            resizeChannels(1);
            break;
        case LCT_RGB:
            resizeChannels(3);
            break;
        case LCT_RGBA:
            resizeChannels(4);
            break;
        default:
            logFatal("Unsupported color type: {}",
                     (int)state.info_png.color.colortype);
            return;
    }

    resize(width, height);
    int area = getArea();
    for (int c = 0; c < m_channels; c++) {
        auto& layer = m_rawData[c];
        for (int i = 0; i < area; i++) {
            layer[i] = float(image[i * m_channels + c]) / 255.f;
        }
    }
}

Image Image::load(const std::filesystem::path& filename) {
    Image image(0, 0, 3);
    if (filename.extension() == ".png") {
        image.readPNG(filename);
    } else if (filename.extension() == ".exr") {
        image.readEXR(filename);
    } else {
        logFatal("Unsupported image format: {}", filename.extension().string());
    }
    return image;
}

}  // namespace Rastery