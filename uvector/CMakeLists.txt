project(uvector)

add_executable(uvector main.cpp)

install(TARGETS uvector RUNTIME DESTINATION bin)
set_target_properties(uvector PROPERTIES COMPILE_FLAGS "-std=c++11 -Wall -pedantic")
