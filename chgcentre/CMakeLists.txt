cmake_minimum_required(VERSION 2.6)

project(chgcentre)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
find_library(GSL_LIB gsl REQUIRED)
enable_language(Fortran OPTIONAL)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB} ${CASA_MEASURES_LIB})

find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)

include_directories(${CASA_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -DNDEBUG -march=native")

# The following stuff will set the "rpath" correctly, so that
# LD_LIBRARY_PATH doesn't have to be set.

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

add_executable(chgcentre main.cpp progressbar.cpp)

target_link_libraries(chgcentre ${CASA_LIBS} ${LAPACK_lapack_LIBRARY})

install (TARGETS chgcentre DESTINATION bin)
