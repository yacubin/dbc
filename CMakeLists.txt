cmake_minimum_required(VERSION 3.12)
project(dbc VERSION 0.0.1
  DESCRIPTION "DBC handles parsing and encoding CAN data using DBC files for automotive systems"
  HOMEPAGE_URL "https://github.com/yacubin/dbc"
  LANGUAGES C
  )

include(CheckTypeSize)
include(GNUInstallDirs)

set(DBC_LIBRARY_TYPE STATIC CACHE STRING "Library type")
set_property(CACHE DBC_LIBRARY_TYPE PROPERTY STRINGS STATIC SHARED)

option(DBC_LOG_ENABLE "Enable log" OFF)

check_type_size("void*"  SIZEOF_VOIDP)
if (SIZEOF_VOIDP EQUAL 4)
  set(DBC_CPU_32BIT ON)
endif ()

set(DBC_HEADERS
  dbc/DBCAlloc.h
  dbc/DBCAssert.h
  dbc/DBCDocBuilder.h
  dbc/DBCDocument.h
  dbc/DBCDocWriter.h
  dbc/DBCParser.h
  dbc/DBCID.h
  dbc/DBCJ1939.h
  dbc/DBCLog.h
  dbc/DBCKeywords.h
  dbc/DBCErrorCode.h
  dbc/DBCTypes.h
  )

set(DBC_SOURCES
  dbc/DBCAlloc.c
  dbc/DBCDocBuilder.c
  dbc/DBCDocument.c
  dbc/DBCDocWriter.c
  dbc/DBCParser.c
  dbc/DBCID.c
  dbc/DBCJ1939.c
  dbc/DBCLog.c
  dbc/DBCTypes.c
  )

set(DBC_DATAFILES
  config.h.cmake
  LICENSE
  README.md
  )

set(DBC_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
  )

set(DBC_LIBRARIES
  )

set(DBC_DEFINITIONS
  $<$<CONFIG:Debug>:DBC_LOG_ENABLE>
  )

configure_file(config.h.cmake "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DBC_HEADERS} ${DBC_SOURCES} ${DBC_DATAFILES})
add_library(dbc ${DBC_LIBRARY_TYPE} ${DBC_HEADERS} ${DBC_SOURCES} ${DBC_DATAFILES})
target_include_directories(dbc PUBLIC "$<BUILD_INTERFACE:${DBC_INCLUDES}>")
target_link_libraries(dbc PUBLIC ${DBC_LIBRARIES})
set_target_properties(dbc PROPERTIES COMPILE_DEFINITIONS "${DBC_DEFINITIONS}")
set_target_properties(dbc PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET dbc PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS dbc
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )

foreach(_header IN LISTS DBC_HEADERS)
  get_filename_component(_dir "${_header}" DIRECTORY)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${_dir}")
endforeach()
