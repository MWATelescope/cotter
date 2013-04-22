cmake_minimum_required(VERSION 2.6)

project(cotter)

find_library(AOFLAGGER_LIB aoflagger)
find_path(AOFLAGGER_INCLUDE_DIR NAMES aoflagger.h)
find_library(CFITSIO_LIB cfitsio)
find_path(FITSIO_INCLUDE_DIR NAMES fitsio.h)
find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-2.4>=2.18)
pkg_check_modules(SIGCXX sigc++-2.0)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB} ${CASA_MEASURES_LIB})
find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)
enable_language(Fortran OPTIONAL)
find_package(LAPACK REQUIRED)
find_package(Boost REQUIRED COMPONENTS date_time thread filesystem system)
find_library(FFTW3_LIB fftw3 REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(LIBSLA_LIB sla REQUIRED)
find_library(PNG_LIB png REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DNDEBUG -funroll-loops -msse2 -O0 -ggdb -march=native")

include_directories(${AOFLAGGER_INCLUDE_DIR})
include_directories(${CASA_INCLUDE_DIR})
include_directories(${FITSIO_INCLUDE_DIR})

add_executable(cotter main.cpp cotter.cpp averagingmswriter.cpp flagwriter.cpp fitsuser.cpp fitswriter.cpp gpufilereader.cpp metafitsfile.cpp mwaconfig.cpp mwams.cpp mswriter.cpp)
target_link_libraries(cotter ${CFITSIO_LIB} ${AOFLAGGER_LIB} ${CASA_LIBS} ${LAPACK_lapack_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${LIBXML2_LIBRARIES} ${FFTW3_LIB} ${LIBSLA_LIB} ${PNG_LIB})

install (TARGETS cotter DESTINATION bin)
