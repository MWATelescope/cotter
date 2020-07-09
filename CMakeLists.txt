cmake_minimum_required(VERSION 2.6)

project(cotter)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0>=3.0.0)
pkg_check_modules(SIGCXX sigc++-2.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
find_package(AOFlagger 2 REQUIRED)
include_directories(${AOFLAGGER_INCLUDE_DIR})

find_package(CFITSIO)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
set(CASACORE_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB} ${CASA_MEASURES_LIB})
find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)
enable_language(Fortran OPTIONAL)
find_package(LAPACK REQUIRED)
find_package(PythonLibs 3 REQUIRED)
find_package(PythonInterp 3 REQUIRED)
message(STATUS "Using python version ${PYTHON_VERSION_STRING}")
set(BOOSTPY ${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
find_package(Boost 1.55.0 REQUIRED COMPONENTS date_time filesystem python${BOOSTPY})
set(BOOST_PART_LIBRARIES ${Boost_LIBRARIES})
# Numpy3 is in some versions of Boost part of python3, so don't 'require' it:                                                                                           
find_package(Boost 1.55.0 COMPONENTS numpy${BOOSTPY})
set(Boost_LIBRARIES ${BOOST_PART_LIBRARIES} ${Boost_LIBRARIES})

find_library(FFTW3_LIB fftw3 REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(LIBPAL_LIB pal REQUIRED)
find_path(LIBPAL_INCLUDE_DIR NAMES star/pal.h)
find_library(PNG_LIB png REQUIRED)
find_library(PTHREAD_LIB pthread REQUIRED)

add_compile_options(-Wall -Wno-noexcept-type -DNDEBUG -O3 -march=native)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include_directories(${CASA_INCLUDE_DIR})
include_directories(${FITSIO_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${LIBPAL_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})

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

add_executable(cotter main.cpp cotter.cpp applysolutionswriter.cpp averagingwriter.cpp flagwriter.cpp fitsuser.cpp fitswriter.cpp gpufilereader.cpp metafitsfile.cpp mwaconfig.cpp mwafits.cpp mwams.cpp mswriter.cpp progressbar.cpp stopwatch.cpp subbandpassband.cpp threadedwriter.cpp)

add_executable(fixmwams fixmwams.cpp fitsuser.cpp metafitsfile.cpp mwaconfig.cpp mwams.cpp)

target_link_libraries(cotter
	${AOFLAGGER_LIB}
	${CASACORE_LIBS}
	${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_PYTHON_LIBRARIES}
	${LIBPAL_LIB}
	${PTHREAD_LIB}
	${PYTHON_LIBRARIES}
)

target_link_libraries(fixmwams ${CFITSIO_LIBRARY} ${CASACORE_LIBS} ${LIBPAL_LIB})

install (TARGETS cotter fixmwams DESTINATION bin)
