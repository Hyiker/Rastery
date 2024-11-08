
#pragma once
#include <cstdint>
#include <memory>

#include "Core/Macros.h"
namespace Rastery {

enum class TextureWrap { Repeat, ClampToEdge, Count };

enum class TextureFilter { Nearest, Linear, MipMap, Count };

struct TextureWrapDesc {
    TextureWrap wrapS = TextureWrap::Repeat;
    TextureWrap wrapT = TextureWrap::Repeat;
};
struct TextureFilterDesc {
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
};

struct TextureDesc {
    int type;
    int format;

    int width;
    int height;
    int depth;
    int layers;

    TextureWrapDesc wrapDesc;
    TextureFilterDesc filterDesc;
};

struct TextureSubresourceDesc {
    int width;
    int height;
    int depth;

    int xOffset;
    int yOffset;
    int zOffset;

    int format;
    int type;
    const void* pData;
};

class RASTERY_API Texture {
   public:
    using SharedPtr = std::shared_ptr<Texture>;
    Texture(const TextureDesc& desc,
            const TextureSubresourceDesc* pInitData = nullptr);

    void uploadData(const TextureSubresourceDesc& subresourceDesc) const;

    void generateMipMap() const;

    [[nodiscard]] uint32_t getId() const { return mId; }

   private:
    TextureDesc mDesc;
    uint32_t mId;
};
}  // namespace Rastery
