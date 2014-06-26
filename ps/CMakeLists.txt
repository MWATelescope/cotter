cmake_minimum_required(VERSION 2.6)

project(mwa-reduce)

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -DNDEBUG -march=native -msse4.1 -std=c++0x")

#find_library(CASA_MS_LIB casa_ms REQUIRED)
#find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
find_library(CASA_FITS_LIB casa_fits REQUIRED)
find_package(Boost COMPONENTS filesystem system thread REQUIRED)
find_library(FITSIO_LIB cfitsio REQUIRED)
find_path(FITSIO_INCLUDE_DIR NAMES fitsio.h REQUIRED)
#find_library(DL_LIB dl REQUIRED)
find_library(GSL_LIB NAMES gsl REQUIRED)
find_path(GSL_INCLUDE_DIR NAMES gsl/gsl_version.h REQUIRED)
#find_library(CBLAS_LIB NAMES gslcblas)
#find_path(GTS_INCLUDE_DIR NAMES gts.h)
#find_path(GLIB_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)
#find_package(LibXml2)
find_library(FFTW3_LIB fftw3 REQUIRED)
find_path(FFTW3_INCLUDE_DIR NAMES fftw3.h)

set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_MEASURES_LIB} ${CASA_CASA_LIB} ${CASA_FITS_LIB} ${DL_LIB})

find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)

include_directories(${CASA_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${FITSIO_INCLUDE_DIR})
include_directories(${FFTW3_INCLUDE_DIR})
include_directories(${GSL_INCLUDE_DIR})
#if(GTS_INCLUDE_DIR AND GLIB_INCLUDE_DIR)
#  include_directories(${GTS_INCLUDE_DIR} ${GLIB_INCLUDE_DIR})
#endif(GTS_INCLUDE_DIR AND GLIB_INCLUDE_DIR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -Wvla")

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

#add_executable(psintegrate psintegrate.cpp fitsreader.cpp)
#target_link_libraries(psintegrate ${FITSIO_LIB} ${Boost_LIBRARIES} ${CASA_LIBS})

add_executable(psweight psweight.cpp fitsreader.cpp fitswriter.cpp)
target_link_libraries(psweight ${FITSIO_LIB} ${Boost_LIBRARIES} ${CASA_LIBS})

add_executable(pstransform pstransform.cpp fft2d.cpp fitsreader.cpp fitswriter.cpp)
target_link_libraries(pstransform ${FITSIO_LIB} ${Boost_LIBRARIES} ${CASA_LIBS} ${FFTW3_LIB})

#add_executable(psplot psplot.cpp fitsreader.cpp fitswriter.cpp)
#target_link_libraries(psplot ${FITSIO_LIB} ${Boost_LIBRARIES} ${CASA_LIBS})

add_executable(psfitfg psfitfg.cpp nlplfitter.cpp)
target_compile_definitions(psfitfg PUBLIC HAVE_GSL)
target_link_libraries(psfitfg ${FITSIO_LIB} ${Boost_LIBRARIES} ${CASA_LIBS} ${GSL_LIB})
