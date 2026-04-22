cmake_minimum_required(VERSION 3.5...3.28)

project(qore-magic-module)

set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)
set (VERSION_PATCH 0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

FIND_PACKAGE (Qore 0.9 REQUIRED)

find_package(LibMagic REQUIRED)

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

include_directories( ${CMAKE_SOURCE_DIR}/src )
include_directories( ${LIBMAGIC_INCLUDE_DIR} )

set(CPP_SRC
    src/qoremagic.cpp
    src/magic-module.cpp
)

set(QPP_SRC
    src/magic.qpp
    src/ql_magic.qpp
)

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SRC ${QPP_SRC})

foreach (it ${QPP_SOURCES})
    GET_FILENAME_COMPONENT(_outfile ${it} NAME_WE)
    set(QPP_DOX ${QPP_DOX} ${CMAKE_CURRENT_BINARY_DIR}/${_outfile}.dox.h)
endforeach()

SET (module_name "magic")

set(_XTRA_LIBS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
   set(_XTRA_LIBS "${_XTRA_LIBS};shlwapi;regex")
endif()

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})

set(QORE_DOX_TMPL_SRC
    docs/release-notes.doxygen.tmpl
)

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

if (DOXYGEN_FOUND)
    qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
endif()

set(MODULE_DOX_INPUT ${CMAKE_SOURCE_DIR}/src/main.cpp ${QPP_DOX} ${QORE_DOX_SRC})
string(REPLACE ";" " " MODULE_DOX_INPUT "${MODULE_DOX_INPUT}")

qore_external_binary_module(${module_name} "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" ${LIBMAGIC_LIBRARY} ${_XTRA_LIBS})

if (DOXYGEN_FOUND)
    add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
    add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()

qore_dist("${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SRC})
else()
    install(FILES ${QPP_META_SRC}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

qore_config_info()
