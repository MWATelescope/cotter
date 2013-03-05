project(compress)

find_library(CASA_MS_LIB casa_ms REQUIRED)
find_library(CASA_TABLES_LIB casa_tables REQUIRED)
find_library(CASA_CASA_LIB casa_casa REQUIRED)
find_library(CASA_MEASURES_LIB casa_measures REQUIRED)

#set(Boost_USE_STATIC_LIBS ON)
#find_package(Boost COMPONENTS thread REQUIRED)

set(CASA_LIBS ${CASA_MS_LIB} ${CASA_TABLES_LIB} ${CASA_CASA_LIB})

find_path(CASA_INCLUDE_DIR NAMES ms/MeasurementSets/MeasurementSet.h PATHS /usr/local/include/casacore PATH_SUFFIXES casacore)

include_directories(${CASA_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -DNDEBUG -march=native")

# Note: casapy fails if Casa is linked in the storage manager, so we have to trust that
# casapy's version of casacore is binary compatible with this storage manager's casacore.
add_library(offringastman SHARED register.cpp gausencoder.cpp offringastman.cpp offringacomplexcol.cpp offringaweightcol.cpp weightencoder.cpp)
target_link_libraries(offringastman gsl cblas)

add_executable(test test.cpp stopwatch.cpp)
target_link_libraries(test gsl cblas ${CASA_LIBS} offringastman)

add_executable(compress compress.cpp)
target_link_libraries(compress gsl cblas ${CASA_LIBS} offringastman)

add_executable(simcompress simcompress.cpp)
target_link_libraries(simcompress gsl cblas ${CASA_LIBS} offringastman)
