cmake_minimum_required(VERSION 3.5)
project(my_first_test)
enable_testing()


set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.70 COMPONENTS unit_test_framework system filesystem thread)
if(Boost_FOUND)
   message(STATUS "boost is found, include = ${Boost_INCLUDE_DIRS}")
endif()
# indicates the location of the boost installation tree.
# hard-coded for our simple example.
# set(BOOST_INCLUDE_DIRS $boost_installation_prefix/include)

# creates the executable
add_executable(test_executable
   test/async.cpp
   src/dir_monitor.hpp
   src/basic_dir_monitor.hpp
   src/inotify/basic_dir_monitor_service.hpp
   src/inotify/dir_monitor_impl.hpp
   )


# indicates the include paths
target_include_directories(test_executable PRIVATE ${Boost_INCLUDE_DIRS}
   src
   )


message(STATUS "BOOST = ${Boost_LIBRARIES}")
target_link_libraries(test_executable pthread ${Boost_LIBRARIES})

# declares a test with our executable
add_test(NAME test1 COMMAND test_executable)
