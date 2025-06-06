#include "Texture.h"

#include <glad.h>

#include <bit>
#include <cstdint>
#include <span>

#include "Core/Error.h"

namespace Rastery {

static int toGraphicsEnum(TextureWrap textureWrap) {
    switch (textureWrap) {
        case TextureWrap::Repeat:
            return GL_REPEAT;
        case TextureWrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
    }
    RASTERY_UNREACHABLE();
}

static int toGraphicsEnum(TextureFilter textureFilter) {
    switch (textureFilter) {
        case TextureFilter::Nearest:
            return GL_NEAREST;
        case TextureFilter::Linear:
            return GL_LINEAR;
        case TextureFilter::MipMap:
            return GL_LINEAR_MIPMAP_LINEAR;
    }
    RASTERY_UNREACHABLE();
}

static int toGraphicsEnum(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8:
            return GL_RGBA8;
        case TextureFormat::Rgba32F:
            return GL_RGBA32F;
        case TextureFormat::R32F:
            return GL_R32F;
    }
    RASTERY_UNREACHABLE();
}

static int toSubresourceFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8:
        case TextureFormat::Rgba32F:
            return GL_RGBA;
        case TextureFormat::R32F:
            return GL_R;
    }
    RASTERY_UNREACHABLE();
}

static int toSubresourceType(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::Rgba32F:
        case TextureFormat::R32F:
            return GL_FLOAT;
    }
    RASTERY_UNREACHABLE();
}

int getFormatBytes(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8:
            return 4;
        case TextureFormat::Rgba32F:
            return 4 * 4;
        case TextureFormat::R32F:
            return 4;
    }
    RASTERY_UNREACHABLE();
}

Texture::Texture(const TextureDesc& desc, const TextureSubresourceDesc* pInitData) : mDesc(desc) {
    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGraphicsEnum(desc.wrapDesc.wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGraphicsEnum(desc.wrapDesc.wrapT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGraphicsEnum(desc.filterDesc.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGraphicsEnum(desc.filterDesc.magFilter));
    RASTERY_CHECK_GL_ERROR();

    if (pInitData) {
        glTexImage2D(GL_TEXTURE_2D, desc.layers, toGraphicsEnum(desc.format), desc.width, desc.height, 0,
                     toSubresourceFormat(pInitData->format), toSubresourceType(pInitData->format), pInitData->pData);
    } else {
        glTexImage2D(GL_TEXTURE_2D, desc.layers, toGraphicsEnum(desc.format), desc.width, desc.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);
    }
    RASTERY_CHECK_GL_ERROR();
}

Texture::Texture(Texture&& other) noexcept : mId(other.mId) { other.mId = 0u; }

Texture& Texture::operator=(Texture&& other) noexcept {
    mId = other.mId;
    other.mId = 0;
    return *this;
}

void Texture::uploadData(const TextureSubresourceDesc& subresourceDesc) const {
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, subresourceDesc.xOffset, subresourceDesc.yOffset, subresourceDesc.width, subresourceDesc.height,
                    toSubresourceFormat(subresourceDesc.format), toSubresourceType(subresourceDesc.format), subresourceDesc.pData);
}

void Texture::generateMipMap() const {
    glBindTexture(GL_TEXTURE_2D, mId);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() { glDeleteTextures(1, &mId); }

CpuTexture::CpuTexture(const TextureDesc& desc) : mDesc(desc) { mData.resize(desc.width * desc.height * getFormatBytes(desc.format), 0); }

void* CpuTexture::getPtr() { return mData.data(); }

void CpuTexture::clear(float4 color) {
    // TODO: use more flexible typing + byteSize
    TextureFormat format = mDesc.format;
    switch (format) {
        case TextureFormat::Rgba8: {
            color *= 255.f;
            unsigned char colorRgba8[4]{
                (unsigned char)color[0],
                (unsigned char)color[1],
                (unsigned char)color[2],
                (unsigned char)color[3],
            };
            uint32_t v;
            std::memcpy(&v, colorRgba8, sizeof(uint32_t));
            std::span<uint32_t> view((uint32_t*)(mData.data()), mData.size() / getFormatBytes(format));
            std::fill(view.begin(), view.end(), v);
        } break;
        case TextureFormat::Rgba32F: {
            std::span<float4> view((float4*)(mData.data()), mData.size() / getFormatBytes(format));
            std::fill(view.begin(), view.end(), color);
        } break;
        case TextureFormat::R32F: {
            std::span<float> view((float*)(mData.data()), mData.size() / getFormatBytes(format));
            std::fill(view.begin(), view.end(), color.r);
        } break;
        default: {
            RASTERY_UNREACHABLE();
        } break;
    }
}

int computeMipMpaLevels(uint32_t width, uint32_t height) {
    int n = 0;
    while (width || height) {
        n++;
        width >>= 1;
        height >>= 1;
    }
    return n;
}

}  // namespace Rastery