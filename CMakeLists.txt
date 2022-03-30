cmake_minimum_required( VERSION 3.14 )

project( turbo-packing DESCRIPTION "Turbo-Packing" )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Werror -std=gnu++17")


set(TEST_EXECUTABLE "tests.x")

set(TP_SOURCE_FILES
  src/tp/circuits.cc
  src/tp/gate.cc
)

set(TEST_SOURCE_FILES
  test/main.cc

  test/test_circuit.cc
  test/test_gates.cc
)


find_package( Catch2 REQUIRED )
include( CTest )
include( Catch )

# add_subdirectory( secure-computation-library )
add_executable( ${TEST_EXECUTABLE} ${TP_SOURCE_FILES} ${TEST_SOURCE_FILES} )
target_include_directories(${TEST_EXECUTABLE} PUBLIC
  "${CMAKE_SOURCE_DIR}/secure-computation-library/include"
  "${CMAKE_SOURCE_DIR}/src")
set( libscl "${CMAKE_SOURCE_DIR}/secure-computation-library/build/libscl.so.0.3" )
target_link_libraries( ${TEST_EXECUTABLE} Catch2::Catch2 pthread ${libscl})
# target_link_libraries( ${TEST_EXECUTABLE} Catch2::Catch2 pthread scl)
