if (${MACOSX})
  execute_process(COMMAND "sdl2-config" "--cflags" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_CFLAGS)
  separate_arguments(SDL_CFLAGS)

  execute_process(COMMAND "sdl2-config" "--static-libs" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_LDFLAGS)
  separate_arguments(SDL_LDFLAGS)
else()
    set(SDL_CFLAGS /I${THIRDPARTY}/SDL2/include /D_THREAD_SAFE)
    set(SDL_LDFLAGS ${THIRDPARTY}/SDL2/lib/x64/sdl2.lib)
    set(COMPILE_DEFINITIONS
        _CRT_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_WARNINGS
        _CRT_OBSOLETE_NO_WARNINGS
        )
endif()

add_library(${GAME_LIB_NAME} SHARED "")
add_executable(${GAME_TST_NAME} "")
add_executable(${GAME_EXE_NAME} "")

message("IS_CLANG: ${IS_CLANG}")
set(COMPILE_DEFINITIONS
  QI_DEV_DATA_DIR="${DATA_DIR}"
  QI_DEV_BUILD=${IS_DEV}
  QI_PROFILE_BUILD=${IS_PROFILE}
  QI_RELEASE_BUILD=${IS_RELEASE}
  QI_WIN32_BUILD=${WIN32}
  QI_OSX_BUILD=${MACOSX}
  QI_COMPILER_CLANG=${IS_CLANG}
    ${COMPILE_DEFINITIONS}
  )

if (${IS_CLANG})
    set(COMPILE_FLAGS
      $<${IS_OPTIMIZED}:-O3>
      $<${HAS_DEBUG_INFO}:-g>
      $<${IS_CLANG}:-fms-extensions>
      $<${IS_CLANG}:-fvisibility=hidden>
      $<${IS_CLANG}:-fvisibility-inlines-hidden>
      )
else()
    set(COMPILE_FLAGS
        $<${IS_OPTIMIZED}:/O2 /Ot /Oi>
        $<${HAS_DEBUG_INFO}:/Z7>
        )
endif()

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${THIRDPARTY}/include ${THIRDPARTY}/imgui)

target_include_directories(${GAME_EXE_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${GAME_LIB_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${GAME_TST_NAME} PRIVATE ${INCLUDE_DIRS})

target_compile_definitions(${GAME_EXE_NAME} PRIVATE ${COMPILE_DEFINITIONS})
target_compile_definitions(${GAME_LIB_NAME} PRIVATE ${COMPILE_DEFINITIONS})
target_compile_definitions(${GAME_TST_NAME} PRIVATE ${COMPILE_DEFINITIONS})

target_compile_options(${GAME_EXE_NAME} PRIVATE ${SDL_CFLAGS} ${COMPILE_FLAGS})
target_compile_options(${GAME_LIB_NAME} PRIVATE ${COMPILE_FLAGS})
target_compile_options(${GAME_TST_NAME} PRIVATE ${COMPILE_FLAGS})

target_link_options(${GAME_EXE_NAME} PRIVATE ${SDL_LDFLAGS})

file(GLOB HEADER_LIST CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_LIST})

set(QI_EXE_SRCS qi_memory.cpp qi_ogl.cpp sdl_qi.cpp)

target_sources(${GAME_LIB_NAME}
  PRIVATE

  qi.cpp
  qi_array.cpp
  qi_debug.cpp
  qi_deque.cpp
  qi_ent.cpp
  qi_gjk.cpp
  qi_lexer.cpp
  qi_math.cpp
  qi_memory.cpp
  qi_noise.cpp
  qi_ogl.cpp
  qi_profile.cpp
  qi_sound.cpp
  qi_stringtable.cpp
  qi_tile.cpp

  ${HEADER_LIST}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_EXE_NAME}
  PRIVATE

  ${QI_EXE_SRCS}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_TST_NAME}
  PRIVATE

  qi_test.cpp
  qi_noise.cpp
  qi_lexer.cpp
  )

file(GLOB IMGUI_SRCS CONFIGURE_DEPENDS ${IMGUI}/*.cpp ${IMGUI}/*.h)

target_sources(${GAME_LIB_NAME}
  PRIVATE

  qi.cpp
  qi_array.cpp
  qi_debug.cpp
  qi_deque.cpp
  qi_ent.cpp
  qi_gjk.cpp
  qi_lexer.cpp
  qi_math.cpp
  qi_memory.cpp
  qi_noise.cpp
  qi_ogl.cpp
  qi_profile.cpp
  qi_sound.cpp
  qi_stringtable.cpp
  qi_tile.cpp

  ${HEADER_LIST}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_EXE_NAME}
  PRIVATE

  ${QI_EXE_SRCS}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_TST_NAME}
  PRIVATE

  qi_test.cpp
  qi_noise.cpp
  qi_lexer.cpp
  )

target_link_libraries(${GAME_EXE_NAME} PRIVATE imgui glad)
target_link_libraries(${GAME_LIB_NAME} PRIVATE glad)
target_link_libraries(${GAME_EXE_NAME} PRIVATE ${GAME_LIB_NAME})

target_sources(${PROJECT_NAME}_test PRIVATE
    qi_test.cpp
    qi_noise.cpp
    qi_lexer.cpp
)

install(
  TARGETS ${GAME_EXE_NAME} ${GAME_LIB_NAME} ${GAME_TST_NAME}
  DESTINATION "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
  )
