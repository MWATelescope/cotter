cmake_minimum_required(VERSION 2.6)

project(chgcentre)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)
find_library(GSL_LIB gsl REQUIRED)
find_library(CBLAS_LIB cblas REQUIRED)

set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB} ${CASA_MEASURES_LIB})

find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)

include_directories(${CASA_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -DNDEBUG -march=native")

add_executable(chgcentre main.cpp)

target_link_libraries(chgcentre ${CASA_LIBS})

install (TARGETS chgcentre DESTINATION bin)
