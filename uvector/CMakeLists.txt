cmake_minimum_required(VERSION 2.6)
    
project(uvector)

add_executable(uvector main.cpp)
set_target_properties(uvector PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(t01construct t01construct.cpp)
set_target_properties(t01construct PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(t02pushback t02pushback.cpp)
set_target_properties(t02pushback PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(t03insert t03insert.cpp)
set_target_properties(t03insert PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(t04iterate t04iterate.cpp)
set_target_properties(t04iterate PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(t05index t05index.cpp)
set_target_properties(t05index PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

add_executable(tcompare tcompare.cpp stopwatch.cpp)
set_target_properties(tcompare PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic -O3 -march=native")

install(TARGETS uvector RUNTIME DESTINATION bin)
install(FILES uvector.h DESTINATION include)

find_package(Doxygen)

if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM)

endif(DOXYGEN_FOUND)
