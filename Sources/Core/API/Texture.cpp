#include "Texture.h"

#include <glad.h>

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

Texture::Texture(const TextureDesc& desc,
                 const TextureSubresourceDesc* pInitData)
    : mDesc(desc) {
    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    toGraphicsEnum(desc.wrapDesc.wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    toGraphicsEnum(desc.wrapDesc.wrapT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    toGraphicsEnum(desc.filterDesc.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    toGraphicsEnum(desc.filterDesc.magFilter));
    RASTERY_CHECK_GL_ERROR();

    if (pInitData) {
        glTexImage2D(GL_TEXTURE_2D, desc.layers, desc.format, desc.width,
                     desc.height, 0, pInitData->format, pInitData->type,
                     pInitData->pData);
    } else {
        glTexImage2D(GL_TEXTURE_2D, desc.layers, desc.format, desc.width,
                     desc.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    RASTERY_CHECK_GL_ERROR();
}

void Texture::uploadData(const TextureSubresourceDesc& subresourceDesc) const {
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, subresourceDesc.xOffset,
                    subresourceDesc.yOffset, subresourceDesc.width,
                    subresourceDesc.height, subresourceDesc.format,
                    subresourceDesc.type, subresourceDesc.pData);
}

void Texture::generateMipMap() const {
    glBindTexture(GL_TEXTURE_2D, mId);
    glGenerateMipmap(GL_TEXTURE_2D);
}
}  // namespace Rastery