cmake_minimum_required(VERSION 2.8)

project(offringastman)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
find_library(GSL_LIB gsl REQUIRED)
find_library(CBLAS_LIB cblas REQUIRED)

#set(Boost_USE_STATIC_LIBS ON)
#find_package(Boost COMPONENTS thread REQUIRED)

set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB})

find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)

include_directories(${CASA_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -DNDEBUG -march=native")

# Note: casapy fails if Casa is linked in the storage manager, so we have to trust that
# casapy's version of casacore is binary compatible with this storage manager's casacore.
add_library(offringastman SHARED gausencoder.cpp offringastman.cpp offringacomplexcol.cpp offringaweightcol.cpp rmscollector.cpp rmstable.cpp weightencoder.cpp)
target_link_libraries(offringastman gsl cblas)

add_executable(test test.cpp stopwatch.cpp)
target_link_libraries(test ${GSL_LIB} ${CBLAS_LIB} ${CASA_LIBS} offringastman)

add_executable(compress compress.cpp)
target_link_libraries(compress ${GSL_LIB} ${CBLAS_LIB} ${CASA_LIBS} offringastman)

add_executable(decompress decompress.cpp)
target_link_libraries(decompress ${GSL_LIB} ${CBLAS_LIB} ${CASA_LIBS} offringastman)

add_executable(rmsdiff rmsdiff.cpp)
target_link_libraries(rmsdiff ${GSL_LIB} ${CBLAS_LIB} ${CASA_LIBS} offringastman)

# add target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)
