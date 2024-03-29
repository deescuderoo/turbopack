cmake_minimum_required( VERSION 3.14 )

project( turbo-packing DESCRIPTION "Turbo-Packing" )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Werror -std=gnu++17")

set(OURS "ours.x")
set(DN07 "dn07.x")

set(TP_SOURCE_FILES
  src/tp/gate.cc
  src/tp/mult_gate.cc
  src/tp/input_gate.cc
  src/tp/output_gate.cc

  src/tp/circuits/building.cc
  src/tp/circuits/cleartext.cc
  src/tp/circuits/generic.cc
  src/tp/circuits/online.cc
  src/tp/circuits/offline.cc  

  src/tp/correlator.cc  
  src/tp/fi_prep.cc  

  src/tp/dn07.cc  
)

set(TEST_SOURCE_FILES
  test/main.cc

  test/test_circuit.cc
  test/test_input.cc
  test/test_add.cc
  test/test_output.cc
  test/test_mult.cc
  test/test_fd_prep.cc

  test/test_dn07.cc
)


add_subdirectory( secure-computation-library )

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0" )

  set(TEST_EXECUTABLE "tests.x")
  find_package( Catch2 REQUIRED )
  include( CTest )
  include( Catch )

  ## Executable for tests
  add_executable( ${TEST_EXECUTABLE} ${TP_SOURCE_FILES} ${TEST_SOURCE_FILES} )
  target_include_directories(${TEST_EXECUTABLE} PUBLIC
    "${CMAKE_SOURCE_DIR}/secure-computation-library/include"
    "${CMAKE_SOURCE_DIR}/src")
  set( libscl "${CMAKE_SOURCE_DIR}/secure-computation-library/build/libscl.so.0.3" )
  target_link_libraries( ${TEST_EXECUTABLE} Catch2::Catch2 pthread scl)

else()
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")
  ## Executable of our protocol
  add_executable(${OURS} experiments/ours.cc ${TP_SOURCE_FILES})
  target_include_directories(${OURS} PUBLIC
    "${CMAKE_SOURCE_DIR}/secure-computation-library/include"
    "${CMAKE_SOURCE_DIR}/src")
  target_link_libraries(${OURS} pthread scl)

  ## Executable of DN07
  add_executable(${DN07} experiments/dn07.cc ${TP_SOURCE_FILES})
  target_include_directories(${DN07} PUBLIC
    "${CMAKE_SOURCE_DIR}/secure-computation-library/include"
    "${CMAKE_SOURCE_DIR}/src")
  target_link_libraries(${DN07} pthread scl)
endif()
