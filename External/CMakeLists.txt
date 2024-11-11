include(CPM.cmake)

# fmtlib
# v11.0.2 has issue with enum format
# https://github.com/fmtlib/fmt/issues/4091
CPMAddPackage(
    NAME fmt
    URL "https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz"
)

# glm
CPMAddPackage(
    NAME glm
    URL "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz"
    OPTIONS
    "GLM_ENABLE_CXX_20 ON"
)

# GLFW
CPMAddPackage(
    NAME glfw
    URL "https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz"
    OPTIONS
    "GLM_ENABLE_CXX_20 ON"
)

# tbb
CPMAddPackage(
    NAME tbb
    URL "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.13.0.tar.gz"
    OPTIONS
    "TBB_TEST OFF"
    "BUILD_SHARED_LIBS ON"
)
