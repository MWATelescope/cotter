cmake_minimum_required(VERSION 2.6)

project(cotter)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0>=3.0.0)
pkg_check_modules(SIGCXX sigc++-2.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

find_package(AOFlagger 3 REQUIRED)
include_directories(${AOFLAGGER_INCLUDE_DIR})

find_package(CFITSIO REQUIRED)
include_directories(${CFITSIO_INCLUDE_DIR})

set(CASACORE_MAKE_REQUIRED_EXTERNALS_OPTIONAL TRUE)
find_package(Casacore REQUIRED COMPONENTS casa ms tables measures)
include_directories(${CASACORE_INCLUDE_DIRS})

find_package(Boost 1.55.0 REQUIRED COMPONENTS date_time filesystem)
include_directories(${Boost_INCLUDE_DIR})

find_library(LIBPAL_LIB pal REQUIRED)
find_path(LIBPAL_INCLUDE_DIR NAMES star/pal.h)
include_directories(${LIBPAL_INCLUDE_DIR})

find_library(PTHREAD_LIB pthread REQUIRED)

option(PORTABLE "Compile for portability" OFF) #OFF by default
if(PORTABLE)    
	add_compile_options(-march=x86-64)
else()    
	add_compile_options(-march=native)
endif(PORTABLE)
add_compile_options(
  -O3
  -DNDEBUG
  -Wall
  -Wno-noexcept-type
  -Wnon-virtual-dtor
  -Wduplicated-branches
  -Wundef
  -Wpointer-arith
  -Wextra
  -Wno-unused-parameter)
unset(PORTABLE CACHE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

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
	${CASACORE_LIBRARIES}
	${AOFLAGGER_LIB}
	${CFITSIO_LIBRARY}
	${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY}
	${LIBPAL_LIB}
	${PTHREAD_LIB}
)

target_link_libraries(fixmwams
	${CFITSIO_LIBRARY}
	${CASACORE_LIBRARIES}
	${LIBPAL_LIB}
)

install (TARGETS cotter fixmwams DESTINATION bin)
