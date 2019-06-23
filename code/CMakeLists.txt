cmake_minimum_required(VERSION 3.14)
project(code)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(code
        basictypes.h
        has.h
        osx_qi.mm
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
        qi_test.cpp
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
        qi_vector_traits.h)

