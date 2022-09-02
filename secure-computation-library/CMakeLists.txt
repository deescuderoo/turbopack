cmake_minimum_required( VERSION 3.14 )

project( scl VERSION 0.3 DESCRIPTION "Secure Computation Library" )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Werror -std=gnu++17")

set(TEST_EXECUTABLE "tests-scl.x")

set(TEST_SOURCE_FILES
  test/scl/main.cc

  test/scl/test_hash.cc
  test/scl/test_prg.cc

  test/scl/math/fields/test_mersenne61.cc
  test/scl/math/fields/test_mersenne127.cc
  test/scl/math/test_vec.cc
  test/scl/math/test_mat.cc
  test/scl/math/test_la.cc
  test/scl/math/test_ff.cc
  test/scl/math/test_z2k.cc

  test/scl/ss/test_additive.cc
  test/scl/ss/test_poly.cc
  test/scl/ss/test_evpoly.cc
  test/scl/ss/test_shamir.cc
  test/scl/ss/test_packed.cc

  test/scl/net/util.cc
  test/scl/net/test_config.cc
  test/scl/net/test_mem_channel.cc
  test/scl/net/test_tcp_channel.cc
  test/scl/net/test_threaded_sender.cc
  test/scl/net/test_network.cc
  test/scl/net/test_discover.cc

  test/scl/p/test_simple.cc)

set(SCL_SOURCE_FILES
  src/scl/prg.cc
  src/scl/hash.cc

  src/scl/math/str.cc
  src/scl/math/fields/mersenne61.cc
  src/scl/math/fields/mersenne127.cc

  src/scl/net/config.cc
  src/scl/net/mem_channel.cc
  src/scl/net/tcp_channel.cc
  src/scl/net/threaded_sender.cc
  src/scl/net/tcp_utils.cc
  src/scl/net/network.cc
  src/scl/net/discovery/server.cc
  src/scl/net/discovery/client.cc)

include_directories( include )

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0" )
  find_package( Catch2 REQUIRED )
  include( CTest )
  include( Catch )

  add_compile_definitions(SCL_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data/")

  # this speeds up tests considerably
  add_compile_definitions(MAX_VEC_READ_SIZE=1024)
  add_compile_definitions(MAX_MAT_READ_SIZE=1024)

  add_executable( ${TEST_EXECUTABLE} ${SCL_SOURCE_FILES} ${TEST_SOURCE_FILES} )
  target_link_libraries( ${TEST_EXECUTABLE} Catch2::Catch2 pthread )
  catch_discover_tests( ${TEST_EXECUTABLE} )

  # Also compile library (for tests in TurboPack)
  add_library( scl SHARED ${SCL_SOURCE_FILES} )
  set_target_properties( scl PROPERTIES VERSION ${PROJECT_VERSION} )

else()
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")
  add_library( scl SHARED ${SCL_SOURCE_FILES} )
  set_target_properties( scl PROPERTIES VERSION ${PROJECT_VERSION} )
endif()
