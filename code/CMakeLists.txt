cmake_minimum_required(VERSION 3.17)

project(qi CXX)

set(CMAKE_CXX_STANDARD 17)

set(MACOSX $<PLATFORM_ID:Darwin>)
set(WIN32 $<PLATFORM_ID:Win32>)

set(BIN_DIR "${CMAKE_SOURCE_DIR}/../bin")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/../data")
set(THIRDPARTY "${CMAKE_SOURCE_DIR}/../thirdparty")
set(IMGUI "${THIRDPARTY}/imgui")

set(IS_OPTIMIZED $<OR:$<CONFIG:Profile>,$<CONFIG:Release>,$<CONFIG:RelWithDebugInfo>>)
set(HAS_DEBUG_INFO $<OR:$<CONFIG:Profile>,$<CONFIG:Debug>,$<CONFIG:RelWithDebugInfo>>)
set(IS_PROFILE $<CONFIG:Profile>)
set(IS_RELEASE $<CONFIG:Release>)
set(IS_DEV $<NOT:${IS_RELEASE}>)
set(IS_CLANG $<CXX_COMPILER_ID:Clang>)

set(CONFIG_EXTENSION $<$<CONFIG:Debug>:_d>$<$<CONFIG:Profile>:_p>$<$<CONFIG:RelWithDebugInfo>:_r>)

execute_process(COMMAND "sdl2-config" "--cflags" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_CFLAGS)
separate_arguments(SDL_CFLAGS)

execute_process(COMMAND "sdl2-config" "--static-libs" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_LDFLAGS)
separate_arguments(SDL_LDFLAGS)

message("SDL_CFLAGS: ${SDL_CFLAGS}")
message("SDL_LDFLAGS: ${SDL_LDFLAGS}")

add_compile_definitions(
  QI_DEV_DATA_DIR="${DATA_DIR}"
  QI_DEV_BUILD=${IS_DEV}
  QI_PROFILE_BUILD=${IS_PROFILE}
  QI_RELEASE_BUILD=${IS_RELEASE}
  QI_WIN32_BUILD=${WIN32}
  QI_OSX_BUILD=${MACOSX}
  QI_COMPILER_CLANG=${IS_CLANG}
  )

add_compile_options(
    $<${IS_OPTIMIZED}:-O3>
    $<${HAS_DEBUG_INFO}:-g>
    $<${IS_CLANG}:-fms-extensions>
    $<${IS_CLANG}:-fvisibility=hidden>
    $<${IS_CLANG}:-fvisibility-inlines-hidden>
    )

  include_directories(. ${THIRDPARTY}/include ${THIRDPARTY}/imgui ${PROJECT_BINARY_DIR})

add_link_options(${SDL_LDFLAGS})

# Define targets
add_library(${PROJECT_NAME}_game SHARED "")
add_executable(${PROJECT_NAME}_test "")

add_compile_options(${SDL_CFLAGS})
add_executable(${PROJECT_NAME} "")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}${CONFIG_EXTENSION})
set_target_properties(${PROJECT_NAME}_game PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_game${CONFIG_EXTENSION})
set_target_properties(${PROJECT_NAME}_test PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_test${CONFIG_EXTENSION})

# Common definitions

set(QI_EXE_SRCS qi_memory.cpp qi_ogl.cpp sdl_qi.cpp)

link_directories($<{MACOSX}:${THIRDPARTY}/lib/osx>)
link_directories($<{WIN32}:${THIRDPARTY}/lib/x64>)

file(GLOB IMGUI_SRCS ${IMGUI}/*.cpp ${IMGUI}/*.h)
message("IMGUI_SRCS: ${IMGUI_SRCS}")

target_sources(${PROJECT_NAME}_game PRIVATE
  basictypes.h
  has.h
  qi.cpp
  qi.h
  qi_array.cpp
  qi_array.h
  qi_common_binops.h
  qi_debug.cpp
  qi_debug.h
  qi_deque.cpp
  qi_deque.h
  qi_ent.cpp
  qi_ent.h
  qi_gjk.cpp
  qi_gjk.h
  qi_hash.h
  qi_keyvalues.h
  qi_lexer.cpp
  qi_lexer.h
  qi_math.cpp
  qi_math.h
  qi_memory.cpp
  qi_memory.h
  qi_noise.cpp
  qi_noise.h
  qi_ogl.cpp
  qi_ogl.h
  qi_profile.cpp
  qi_profile.h
  qi_sound.cpp
  qi_sound.h
  qi_stringtable.cpp

  qi_stringtable.h
  qi_template_utils.h
  qi_tile.cpp
  qi_tile.h
  qi_util.h
  qi_vecmath.h
  qi_vector.h
  qi_vector_base.h
  qi_vector_functions.h
  qi_vector_helper.h
  qi_vector_proxy.h
  qi_vector_scalar_support.h
  qi_vector_swizzle.h
  qi_vector_traits.h

  ${IMGUI_SRCS}
  )

target_sources(${PROJECT_NAME} PRIVATE
  ${QI_EXE_SRCS}
  ${IMGUI_SRCS}
  )

find_library(GLAD_LIB_PATH glad PATHS "../thirdparty/lib" PATH_SUFFIXES "osx")
target_link_libraries(${PROJECT_NAME} ${GLAD_LIB_PATH})
target_link_libraries(${PROJECT_NAME}_game ${GLAD_LIB_PATH})

target_sources(${PROJECT_NAME}_test PRIVATE
    qi_test.cpp
    qi_noise.cpp
    qi_lexer.cpp
)

install(
  TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_game ${PROJECT_NAME}_test
  DESTINATION "${CMAKE_SOURCE_DIR}/../bin"
  )
