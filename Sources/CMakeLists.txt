add_executable(Rastery
    Core/API/Texture.cpp
    Core/API/Shader.cpp

    Core/App.cpp
    Core/Error.cpp
    Core/Rastery.cpp
    Core/Window.cpp

    Utils/Image.cpp
    Utils/Logger.cpp
)

target_sources(Rastery
    PRIVATE

    # imgui
    ../External/imgui/imgui_demo.cpp
    ../External/imgui/imgui_draw.cpp
    ../External/imgui/imgui_impl_glfw.cpp
    ../External/imgui/imgui_impl_opengl3.cpp
    ../External/imgui/imgui_tables.cpp
    ../External/imgui/imgui_widgets.cpp
    ../External/imgui/imgui.cpp

    # glad
    ../External/glad/glad.c

    # lodepng
    ../External/lodepng/lodepng.cpp
    
    # tinyexr
    ../External/tinyexr/miniz.c
)

# C++ Standard version
target_compile_features(Rastery PUBLIC cxx_std_20)
set_target_properties(Rastery PROPERTIES CXX_EXTENSIONS OFF)

# Compile options
target_compile_options(Rastery PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_include_directories(Rastery
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../External/imgui
    ../External/glad
    ../External/lodepng
    ../External/tinyexr
)
target_compile_definitions(Rastery
    PRIVATE
    RASTERY_STATIC_LINK=1
)

target_link_libraries(Rastery
    PRIVATE
    fmt::fmt
    glm::glm
    TBB::tbb
    glfw
)
