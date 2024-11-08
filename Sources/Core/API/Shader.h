#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Core/API/Texture.h"
#include "Core/Macros.h"

namespace Rastery {

enum class ShaderStage {
    Vertex,
    Geometry,
    TessellationControl,
    TessellationEval,
    Fragment,
    Compute,
    Count
};

// From
// https://www.reddit.com/r/cpp_questions/comments/panivh/constexpr_if_statement_to_check_if_template/
template <typename T>
struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
struct is_unique_ptr : std::false_type {};
template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

class UniformProxy {
   public:
    UniformProxy(int location);

    template <typename T>
    UniformProxy& operator=(const T& d) {
        if constexpr (std::is_pointer_v<T> || is_shared_ptr<T>::value ||
                      is_unique_ptr<T>::value) {
            *this = *d;
        } else {
            this->setImpl(d);
        }
        return *this;
    }

   private:
    void setImpl(const Texture& t) const;

    int mLocation;
};

class ShaderVars {
   public:
    ShaderVars(uint32_t programId);

    UniformProxy operator[](const std::string& key) const;

   private:
    uint32_t mProgramId;
};

struct RASTERY_API ShaderProgram {
    using SharedPtr = std::shared_ptr<ShaderProgram>;
    uint32_t id;

    void use() const;

    [[nodiscard]] ShaderVars getRootVars() const;

    ~ShaderProgram();
};

RASTERY_API ShaderProgram::SharedPtr createShaderProgram(
    const std::vector<std::pair<ShaderStage, std::string>>& descs);

}  // namespace Rastery