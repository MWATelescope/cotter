cmake_minimum_required(VERSION 2.6)

project(cotter)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0>=3.0.0)
pkg_check_modules(SIGCXX sigc++-2.0)

find_library(AOFLAGGER_LIB aoflagger)
find_path(AOFLAGGER_INCLUDE_DIR NAMES aoflagger.h)
find_library(CFITSIO_LIB cfitsio)
find_path(FITSIO_INCLUDE_DIR NAMES fitsio.h)

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
find_package(Boost 1.55.0 REQUIRED COMPONENTS date_time filesystem python numpy system)
find_library(FFTW3_LIB fftw3 REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(LIBPAL_LIB pal REQUIRED)
find_path(LIBPAL_INCLUDE_DIR NAMES star/pal.h)
find_library(PNG_LIB png REQUIRED)
find_library(PTHREAD_LIB pthread REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-noexcept-type -DNDEBUG -O3 -march=native -std=c++11")

include_directories(${AOFLAGGER_INCLUDE_DIR})
include_directories(${CASA_INCLUDE_DIR})
include_directories(${FITSIO_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${LIBPAL_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})

# The GTKMM and Sigc++ libraries are not explicitly used by Cotter, but when AOFlagger
# found those libraries, it will use those, and some platforms then
# require those libraries to be linked in here as well
if(GTKMM_FOUND)
	message(STATUS "GTKMM found.") 
	include_directories(${GTKMM_INCLUDE_DIRS})
	link_directories(${GTKMM_LIBDIR})
endif(GTKMM_FOUND)

if(SIGCXX_FOUND)
	message(STATUS "SIGCXX found.") 
	include_directories(${SIGCXX_INCLUDE_DIRS})
	link_directories(${SIGCXX_LIBDIR})
endif(SIGCXX_FOUND)

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
	${CFITSIO_LIB}
	${AOFLAGGER_LIB}
	${CASACORE_LIBS}
	${LAPACK_lapack_LIBRARY} ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES}
	${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_PYTHON_LIBRARIES}
	${LIBXML2_LIBRARIES}
	${FFTW3_LIB}
	${LIBPAL_LIB}
	${PNG_LIB}
	${PTHREAD_LIB}
	${PYTHON_LIBRARIES}
)

if(GTKMM_FOUND)
	target_link_libraries(cotter ${GTKMM_LIBRARIES})
endif(GTKMM_FOUND)
if(SIGCXX_FOUND)
	target_link_libraries(cotter ${SIGCXX_LIBRARIES})
endif(SIGCXX_FOUND)

target_link_libraries(fixmwams ${CFITSIO_LIB} ${CASACORE_LIBS} ${LIBPAL_LIB})

install (TARGETS cotter fixmwams DESTINATION bin)
