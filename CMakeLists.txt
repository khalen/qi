cmake_minimum_required(VERSION 3.16.5)

project(qi
  VERSION 0.1.0.0
  DESCRIPTION "Quantum Immortality game and engine c. 2020 Jon Davis and Quantum Immortality"
  LANGUAGES C CXX)

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_EXTENSIONS off)
  set(CMAKE_CXX_STANDARD 20)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  
  set(MACOSX $<PLATFORM_ID:Darwin>)
  set(WIN32 $<PLATFORM_ID:Windows>)

  set(SRC_DIR "${CMAKE_SOURCE_DIR}/code")
  set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
  set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
  set(THIRDPARTY "${CMAKE_SOURCE_DIR}/thirdparty")
  set(IMGUI "${THIRDPARTY}/imgui")

  set(IS_OPTIMIZED $<OR:$<CONFIG:Profile>,$<CONFIG:Release>,$<CONFIG:RelWithDebugInfo>>)
  set(HAS_DEBUG_INFO $<OR:$<CONFIG:Profile>,$<CONFIG:Debug>,$<CONFIG:RelWithDebugInfo>>)
  set(IS_PROFILE $<CONFIG:Profile>)
  set(IS_RELEASE $<CONFIG:Release>)
  set(IS_DEV $<NOT:${IS_RELEASE}>)

  if (MSVC)
    set(IS_CLANG 0)
  else()
    set(IS_CLANG 1)
  endif()

  set(GAME_EXE_NAME ${PROJECT_NAME})
  set(GAME_LIB_NAME ${PROJECT_NAME}_game)
  set(GAME_TST_NAME ${PROJECT_NAME}_test)
endif()

add_subdirectory(thirdparty)
add_subdirectory(code)

