# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
set(CMAKE_CXX_FLAGS
        "-std=c++11 -Wall -Wextra -Wno-unused-parameter -Wextra -Wno-sign-compare -Wno-missing-field-initializers -Wundef -Wuninitialized -Wunused -Wno-unused-parameter -Wwrite-strings -Wchar-subscripts -funsigned-char -MMD -MF -O2 -flto -os"
        )
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-rtti")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libunwind.a")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} j,--exclude-libs,ALL,--gc-sections,ALL")
#
## remove redundant sections
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -funwind-tables -fno-short-enums ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -funwind-tables -fno-short-enums")

if (CMAKE_BUILD_TYPE MATCHES "Release")

    add_definitions(-DNDEBUG)

    MESSAGE("Build Release Version " ${ANDROID_ABI})

    # can used to strip so, trim so
    # only can use on release
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")

else () # end release

    MESSAGE("Build Debug Version " ${ANDROID_ABI})

    # memory debug
    add_definitions(-DDEBUG_MEMORY)

endif ()  #end debug

add_definitions(-DLYNX_SIMPLIFY)


SET(QuickJs "quickjs")
include_directories(
        ${QuickJs}/
        .
)

SET(QuickJsSrc
        ${QuickJs}/cutils.c
        ${QuickJs}/libregexp.c
        ${QuickJs}/libunicode.c
        ${QuickJs}/quickjs.c
        ${QuickJs}/quickjs-libc.c
        # no need
#        ${QuickJs}/jscompress.c
#        ${QuickJs}/libbf.c
#        ${QuickJs}/unicode_gen.c
#        ${QuickJs}/run-test262.c
#        ${QuickJs}/qjs.c
#        ${QuickJs}/qjsc.c
        )
add_definitions(-D CONFIG_VERSION="\\"2019-07-21\\"")
add_definitions(-D JS_SHARED_LIBRARY)
#add_definitions(-D EMSCRIPTEN)

add_library( # Sets the name of the library.
        quickjs

        # Sets the library as a shared library.
        SHARED

        ${QuickJsSrc}
        # Provides a relative path to your source file(s).
        native-lib.cpp
        quickjs_test.cpp
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        quickjs

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})