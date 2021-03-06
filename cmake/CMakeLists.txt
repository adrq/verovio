cmake_minimum_required(VERSION 2.8.8)

project(Verovio)

set(CMAKE_BUILD_TYPE Release)

if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif (APPLE)

option(NO_DARMS_SUPPORT         "Disable DARMS support"                        ON)
option(NO_PAE_SUPPORT           "Disable Plaine & Easie support"               OFF)
option(NO_ABC_SUPPORT           "Disable ABC support"                          OFF)
option(NO_HUMDRUM_SUPPORT       "Disable Humdrum support"                      OFF)
option(MUSICXML_DEFAULT_HUMDRUM "Enable MusicXML to Humdrum by default"        OFF)
option(BUILD_AS_LIBRARY         "Build verovio as library"                     OFF)

if (NO_HUMDRUM_SUPPORT AND MUSICXML_DEFAULT_HUMDRUM)
    message(SEND_ERROR "Default MusicXML to Humdrum cannot be enabled by default without Humdrum support")
endif()

include_directories(
    ../include
    ../include/midi
    ../include/hum
    ../include/json
    ../include/pugi
    ../include/utf8
    ../include/vrv
    ../libmei
)

if(MSVC)
    add_definitions(/W2)
    add_definitions(/wd4244)          # suppress warning of possible loss of precision
    add_definitions(-DNO_PAE_SUPPORT) # regex is working differently under Windows so PAE is not supported (yet)
    add_definitions(/std:c++17)
    include_directories(../include/win32)
else()
    # jsonxx raises -Wdollar-in-identifier-extension
    # gcc 8.3.1 does not like -Wdollar-in-identifier-extension option.
    add_definitions(-Wall -W -pedantic -Wno-unused-parameter -Wno-dollar-in-identifier-extension)
    add_definitions(-g)
    add_definitions(-std=c++17)

    # extra warnings similar to Xcode compiling settings (most probably covered by -Wall):
    # https://github.com/llvm-mirror/clang/blob/master/include/clang/Basic/DiagnosticGroups.td
    # non-gcc: add_definitions(-Wblock-capture-autoreleasing)
    # non-gcc: add_definitions(-Wbool-conversion)
    # non-gcc: add_definitions(-Wconstant-conversion)
    add_definitions(-Wdeprecated-declarations)
    add_definitions(-Wempty-body)
    # non-gcc: add_definitions(-Wenum-conversion)
    # non-gcc: add_definitions(-Winfinite-recursion)
    # non-gcc (ObjC only): add_definitions(-Wint-conversion)
    add_definitions(-Winvalid-offsetof)
    # non-gcc: add_definitions(-Wmove)
    # non-gcc: add_definitions(-Wno-c++11-extensions)
    # non-gcc: add_definitions(-Wno-comma)
    add_definitions(-Wno-conversion)
    # non-gcc: add_definitions(-Wno-exit-time-destructors)
    add_definitions(-Wno-float-conversion)
    # non-gcc: add_definitions(-Wno-four-char-constants)
    add_definitions(-Wno-missing-braces)
    add_definitions(-Wno-missing-field-initializers)
    # non-gcc (ObjC only): add_definitions(-Wno-missing-prototypes)
    # non-gcc: add_definitions(-Wno-newline-eof)
    # non-gcc: add_definitions(-Wno-non-virtual-dtor)
    add_definitions(-Wno-overloaded-virtual)
    # non-gcc: add_definitions(-Wno-semicolon-before-method-body)
    add_definitions(-Wno-shadow)
    add_definitions(-Wno-sign-conversion)
    add_definitions(-Wno-trigraphs)
    add_definitions(-Wno-unknown-pragmas)
    add_definitions(-Wno-unused-label)
    # non-gcc: add_definitions(-Wnon-literal-null-conversion)
    # non-gcc: add_definitions(-Wobjc-literal-conversion)
    add_definitions(-Wparentheses)
    # non-gcc: add_definitions(-Wrange-loop-analysis)
    # non-gcc: add_definitions(-Wshorten-64-to-32)
    # non-gcc (ObjC only: add_definitions(-Wstrict-prototypes)
    add_definitions(-Wswitch)
    add_definitions(-Wuninitialized)
    add_definitions(-Wunreachable-code)
    add_definitions(-Wunused-function)
    add_definitions(-Wunused-value)
    add_definitions(-Wunused-variable)

endif()

execute_process(
    COMMAND ../tools/get_git_commit.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
)

if(NO_DARMS_SUPPORT)
    add_definitions(-DNO_DARMS_SUPPORT)
endif()

if(NO_PAE_SUPPORT)
    add_definitions(-DNO_PAE_SUPPORT)
endif()

if(NO_HUMDRUM_SUPPORT)
    add_definitions(-DNO_HUMDRUM_SUPPORT)
else()
    file(GLOB hum_SRC "../src/hum/*.cpp")
    if (MUSICXML_DEFAULT_HUMDRUM)
        add_definitions(-DMUSICXML_DEFAULT_HUMDRUM)
    endif()
endif()

file(GLOB verovio_SRC "../src/*.cpp")
file(GLOB midi_SRC "../src/midi/*.cpp")

# Add header files to custom target, otherwise they are not shown in some IDEs (e.g. QtCreator)
file(GLOB_RECURSE LibFiles "../include/*.h")
add_custom_target(headers SOURCES ${LibFiles})

set(all_SRC
    ${verovio_SRC}
    ${hum_SRC}
    ${midi_SRC}
    ../src/json/jsonxx.cc
    ../src/pugi/pugixml.cpp
    ../libmei/attconverter.cpp
    ../libmei/atts_analytical.cpp
    ../libmei/atts_cmn.cpp
    ../libmei/atts_cmnornaments.cpp
    ../libmei/atts_critapp.cpp
    ../libmei/atts_gestural.cpp
    ../libmei/atts_externalsymbols.cpp
    ../libmei/atts_facsimile.cpp
    ../libmei/atts_mei.cpp
    ../libmei/atts_mensural.cpp
    ../libmei/atts_midi.cpp
    ../libmei/atts_neumes.cpp
    ../libmei/atts_pagebased.cpp
    ../libmei/atts_shared.cpp
    ../libmei/atts_visual.cpp
)

##################
# Shared library #
##################

if (BUILD_AS_LIBRARY)
    message(STATUS "***** Building Verorio as shared library *****")
    add_library(verovio SHARED ../tools/c_wrapper.cpp ${all_SRC})

###########
# WASM BC #
###########

elseif (BUILD_AS_WASM)
    message(STATUS "***** Building Verorio as WASM BC *****")
    add_library(verovio STATIC ../tools/c_wrapper.cpp ${all_SRC})

#################
# Python module #
#################

elseif (BUILD_AS_PYTHON)
    message(STATUS "***** Building Verorio as Python module *****")

    # clear out policies
    cmake_policy(SET CMP0078 NEW)
    cmake_policy(SET CMP0086 NEW)

    find_package(SWIG REQUIRED)
    include(${SWIG_USE_FILE})
    set(CMAKE_SWIG_FLAGS "")
    set_source_files_properties(../bindings/python/verovio.i PROPERTIES CPLUSPLUS ON)

    # needed for static build on linux
    add_definitions(-fPIC)

    ### For building for specific version of Python (example for 3.4 on OS X with MacPorts installation)
    #set(PYTHON_INCLUDE_DIR /opt/local/Library/Frameworks/Python.framework/Versions/3.4/include/python3.4m)
    #set(PYTHON_LIBRARY /opt/local/Library/Frameworks/Python.framework/Versions/3.4/lib/libpython3.4.dylib)
    #include_directories(${PYTHON_INCLUDE_DIR})
    #swig_add_module(verovio_module_custom python ../bindings/python/verovio.i)
    #swig_link_libraries(verovio_module_custom verovio ${PYTHON_LIBRARY})

    ### For building for found version of Python (comment when specific version is set)
    find_package(PythonLibs)
    include_directories(${PYTHON_INCLUDE_PATH})
    message(STATUS "***** Building for Python ${PYTHONLIBS_VERSION_STRING} *****")
    string(REPLACE "." "_" OUTPUT_VERSION_STRING ${PYTHONLIBS_VERSION_STRING})
    swig_add_module(verovio_module_${OUTPUT_VERSION_STRING} python ../bindings/python/verovio.i)
    swig_link_libraries(verovio_module_${OUTPUT_VERSION_STRING} verovio ${PYTHON_LIBRARIES})

    add_library(verovio STATIC ../tools/c_wrapper.cpp ${all_SRC})

#####################
# Command-line tool #
#####################

else()
    message(STATUS "***** Building Verorio as command-line tool *****")
    add_executable(verovio ../tools/main.cpp ${all_SRC})

endif()

install(
    TARGETS verovio
    DESTINATION /usr/local/bin
)
install(
    DIRECTORY ../data/
    DESTINATION share/verovio
    FILES_MATCHING PATTERN "*.xml" PATTERN "*.svg"
)
