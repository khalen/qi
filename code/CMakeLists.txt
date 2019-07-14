cmake_minimum_required(VERSION 3.14)
project(qi)

set(CMAKE_CXX_STANDARD 17)

set(DATA_DIR "\"~/work/qi/data\"")
set(THIRDPARTY "~/work/qi/thirdparty")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
    set(WIN32 TRUE)
endif()

set(QI_DEFINES "-DQI_DEV_DATA_DIR=${DATA_DIR} -Wno-deprecated-declarations")
set(QI_LINK_OPTS "")

if (MACOSX)

  if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(QI_DEFINES "${QI_DEFINES} -g -DQI_DEV_BUILD=1 -DQI_PROFILE_BUILD=1")
  elseif(CMAKE_BUILD_TYPE MATCHES "Profile")
    set(QI_DEFINES "${QI_DEFINES} -O3 -g -DQI_DEV_BUILD=1 -DQI_PROFILE_BUILD=1")
  elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebugInfo")
    set(QI_DEFINES "${QI_DEFINES} -O3 -g")
  endif()

  set(QI_DEFINES "${QI_DEFINES} -DQI_OSX_BUILD=1 -fms-extensions -fvisibility=hidden -fvisibility-inlines-hidden")
  set(QI_LINK_OPTS "-framework Cocoa")
  set(QI_EXTRA_SRCS osx_qi.mm qi_ogl.cpp)

  link_directories(${THIRDPARTY}/lib/osx)

elseif(WIN32)

  set(QI_DEFINES "${QI_DEFINES} -DQI_WIN32_BUILD=1")
  link_directories(${THIRDPARTY}/lib/x64)

endif()

include_directories(. ${THIRDPARTY}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QI_DEFINES}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${QI_LINK_OPTS}")

add_library(${PROJECT_NAME}_game

  SHARED

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
)

add_executable(${PROJECT_NAME}
  ${QI_EXTRA_SRCS}
)

target_link_libraries(${PROJECT_NAME} glad)
target_link_libraries(${PROJECT_NAME}_game glad)

add_executable(${PROJECT_NAME}_test
    qi_test.cpp
    qi_noise.cpp
    qi_lexer.cpp
)
