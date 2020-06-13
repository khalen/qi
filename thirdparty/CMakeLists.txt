# GLAD
add_library(glad STATIC glad/src/glad.c include/iconfont.h)
target_include_directories(glad PUBLIC include)

file(GLOB IMGUI_SRCS CONFIGURE_DEPENDS imgui/*.cpp imgui/*.h)
add_library(imgui STATIC ${IMGUI_SRCS})
target_include_directories(imgui PUBLIC imgui)
target_include_directories(imgui PRIVATE ../code)


