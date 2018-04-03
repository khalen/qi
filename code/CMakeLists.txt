cmake_minimum_required(VERSION 3.9)
project(qi_game)

set(CMAKE_CXX_STANDARD 17)

include_directories(../thirdparty/include)

add_library(qi_game SHARED
        basictypes.h
        has.h
        qi.cpp
        qi.h
        qi_array.cpp
        qi_array.h
        qi_debug.cpp
        qi_debug.h
        qi_deque.cpp
        qi_deque.h
        qi_ent.cpp
        qi_ent.h
        qi_gjk.cpp
        qi_gjk.h
        qi_hash.h
        qi_math.cpp
        qi_math.h
        qi_memory.cpp
        qi_memory.h
        qi_ogl.cpp
        qi_ogl.h
        qi_profile.cpp
        qi_profile.h
        qi_sound.cpp
        qi_sound.h
        qi_tile.cpp
        qi_tile.h
        qi_util.h)

add_executable(osx_qi
        osx_qi.mm
        qi_ogl.cpp
        qi_ogl.h
        )

find_library(GLAD_LIB glad PATHS ../thirdparty/lib PATH_SUFFIXES osx)
target_compile_definitions(qi_game PRIVATE QI_DEV_BUILD=1 QI_OSX_BUILD=1 QI_PROFILE_BUILD=1)
target_link_libraries(qi_game ${GLAD_LIB})

set(DEV_DATA_DIR_REL ../data)
get_filename_component(DEV_DATA_DIR ${DEV_DATA_DIR_REL} ABSOLUTE)
find_library(COCOA_FRAMEWORK Cocoa)
target_compile_definitions(osx_qi PRIVATE QI_DEV_BUILD=1 QI_OSX_BUILD=1 QI_PROFILE_BUILD=1 QI_DEV_DATA_DIR="${DEV_DATA_DIR}")
target_link_libraries(osx_qi ${GLAD_LIB} ${COCOA_FRAMEWORK})

