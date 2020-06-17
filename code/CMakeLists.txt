if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  execute_process(COMMAND "sdl2-config" "--cflags" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_CFLAGS)
  separate_arguments(SDL_CFLAGS)

  execute_process(COMMAND "sdl2-config" "--static-libs" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE SDL_LDFLAGS)
  separate_arguments(SDL_LDFLAGS)
else()
    if(${IS_CLANG})
        set(SDL_CFLAGS -I${THIRDPARTY}/SDL2/include -D_THREAD_SAFE)
        set(SDL_LDFLAGS ${THIRDPARTY}/SDL2/lib/x64/sdl2.lib)
    else()
        set(SDL_CFLAGS /I${THIRDPARTY}/SDL2/include /D_THREAD_SAFE)
        set(SDL_LDFLAGS ${THIRDPARTY}/SDL2/lib/x64/sdl2.lib)
    endif()
    set(COMPILE_DEFINITIONS
        _CRT_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_WARNINGS
        _CRT_OBSOLETE_NO_WARNINGS
        )
endif()

add_library(${GAME_LIB_NAME} SHARED "" keystore.cpp util.cpp gamedb.cpp gamedb.h qed_parse.cpp qed_parse.h hwi.h)
add_executable(${GAME_TST_NAME} "")
add_executable(${GAME_EXE_NAME} "" hwi.h)

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
    set(LINK_FLAGS "")
else()
    set(COMPILE_FLAGS
        $<${IS_OPTIMIZED}:/O2 /Ot /Oi>
        $<${HAS_DEBUG_INFO}:/Zi /Zo>
        )
    set(LINK_FLAGS $<${HAS_DEBUG_INFO}:/DEBUG>)
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

target_link_options(${GAME_EXE_NAME} PRIVATE ${SDL_LDFLAGS} ${LINK_FLAGS})
target_link_options(${GAME_LIB_NAME} PRIVATE ${LINK_FLAGS})
target_link_options(${GAME_TST_NAME} PRIVATE ${LINK_FLAGS})

file(GLOB HEADER_LIST CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_LIST})

set(QI_EXE_SRCS memory.cpp main_sdl.cpp)

file(GLOB IMGUI_SRCS CONFIGURE_DEPENDS ${IMGUI}/*.cpp ${IMGUI}/*.h)

target_sources(${GAME_LIB_NAME}
  PRIVATE

        game.cpp
        array.cpp
        debug.cpp
        deque.cpp
        entity.cpp
        gjk.cpp
        lexer.cpp
        math_util.cpp
        memory.cpp
        noise.cpp
        hw_ogl.cpp
        profile.cpp
        sound.cpp
        stringtable.cpp
        tile.cpp
        bitmap.cpp

  ${HEADER_LIST}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_TST_NAME}
  PRIVATE

        test.cpp
        noise.cpp
        lexer.cpp
  )

target_sources(${GAME_EXE_NAME}
  PRIVATE

  ${QI_EXE_SRCS}
  ${IMGUI_SRCS}
  )

target_sources(${GAME_TST_NAME}
  PRIVATE

        test.cpp
        noise.cpp
        lexer.cpp
  )

target_link_libraries(${GAME_EXE_NAME} PRIVATE imgui glad)
target_link_libraries(${GAME_LIB_NAME} PRIVATE glad)
target_link_libraries(${GAME_EXE_NAME} PRIVATE ${GAME_LIB_NAME})

install(
  TARGETS ${GAME_EXE_NAME} ${GAME_LIB_NAME} ${GAME_TST_NAME}
  DESTINATION "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
  )
