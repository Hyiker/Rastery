
#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Core/Macros.h"

namespace Rastery {

enum class TextureWrap { Repeat, ClampToEdge, Count };

enum class TextureFilter { Nearest, Linear, MipMap, Count };

enum class TextureFormat { Rgba8, Rgba32F, Count };

int getFormatBytes(TextureFormat format);

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
    TextureFormat format;

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

    TextureFormat format;
    const void* pData;
};

class RASTERY_API Texture {
   public:
    using SharedPtr = std::shared_ptr<Texture>;
    Texture(const TextureDesc& desc, const TextureSubresourceDesc* pInitData = nullptr);

    Texture(const Texture& other) = delete;
    Texture(Texture&& other) noexcept;

    void uploadData(const TextureSubresourceDesc& subresourceDesc) const;

    void generateMipMap() const;

    [[nodiscard]] const TextureDesc& getDesc() const { return mDesc; }

    [[nodiscard]] uint32_t getId() const { return mId; }

    Texture& operator=(const Texture& other) = delete;
    Texture& operator=(Texture&& other) noexcept;

    ~Texture();

   private:
    TextureDesc mDesc;
    uint32_t mId;
};

template <typename T>
class CpuTexelProxy {
   public:
    CpuTexelProxy(void* ptr) : mpData(ptr) {}

    T* ptr() { return (T*)mpData; }
    const T* ptr() const { return (const T*)mpData; }
    T& deref() { return *ptr(); }
    const T& deref() const { return *ptr(); }

    CpuTexelProxy<T> operator=(const T& v) {
        deref() = v;
        return *this;
    }
    operator T() const { return deref(); }

   private:
    void* mpData;
};

class RASTERY_API CpuTexture {
   public:
    using SharedPtr = std::shared_ptr<CpuTexture>;
    CpuTexture(const TextureDesc& desc);

    void* getPtr();

    template <typename T>
    CpuTexelProxy<T> fetch(int x, int y);

    ~CpuTexture() = default;

    [[nodiscard]] const TextureDesc& getDesc() const { return mDesc; }

   private:
    TextureDesc mDesc;
    std::vector<unsigned char> mData;  ///< Texture data stored in row major.
};

template <typename T>
CpuTexelProxy<T> CpuTexture::fetch(int x, int y) {
    int offset = (x + y * mDesc.width) * getFormatBytes(mDesc.format);
    return CpuTexelProxy<T>(mData.data() + offset);
}

}  // namespace Rastery
