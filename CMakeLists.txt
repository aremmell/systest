cmake_minimum_required(
    VERSION 3.12.0
)

set(PROJECT_NAME systest)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED true)

# policy settings, so CMake doesn't whine.
cmake_policy(SET CMP0025 NEW) # AppleClang
cmake_policy(SET CMP0048 NEW) # project VERSION
cmake_policy(SET CMP0065 NEW) # ENABLE_EXPORTS
cmake_policy(SET CMP0056 NEW) # CMAKE_EXE_LINKER_FLAGS
cmake_policy(SET CMP0066 NEW) # CMAKE_<LANG>_FLAGS_<BUILDTYPE>

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
    cmake_policy(SET CMP0077 NEW) # option
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
    cmake_policy(SET CMP0102 NEW) # mark_as_advanced
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
    cmake_policy(SET CMP0126 NEW) # cache
endif()

# define the project
project(
    ${PROJECT_NAME}
    VERSION 1.0.1
    LANGUAGES C
    DESCRIPTION "aremmell's portability testbed"
)

# debug/release preprocessor
add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if (MSVC)
    add_compile_options(
        /W4 /MP /GS /experimental:c11atomics /wd4267
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/Ox>
    )
else()
    set(CMAKE_THREAD_PREFER_PTHREAD true)
    set(THREADS_PREFER_PTHREAD_FLAG true)
    find_package(Threads REQUIRED)

    # -Wformat-nonliteral is also an option
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror -Wdouble-promotion -Wconversion -Wno-sign-conversion -Wno-string-conversion -Wshift-overflow
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Release>:-O3>
    )

    if (CMAKE_COMPILER_IS_GNUCC)
        add_compile_options(
            -Wformat-truncation
        )
    endif()

    if (THREADS_HAVE_PTHREAD_ARG)
        add_compile_options(
            -pthread
        )
        add_link_options(
            -pthread
        )
    endif()

    add_compile_definitions(
        $<$<CONFIG:Release>:_FORTIFY_SOURCE=2>
    )
endif()

add_executable(
    ${PROJECT_NAME}
    systest.c
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (!WIN32)
    target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Threads::Threads
    )
endif()

target_compile_features(
    ${PROJECT_NAME}
    PUBLIC
    ${C_STANDARD}
)
