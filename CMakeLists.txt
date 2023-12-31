# copyright defined in abieos/LICENSE.txt

cmake_minimum_required (VERSION 3.8)
project(abieos VERSION 0.1 LANGUAGES CXX C)

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

option(ABIEOS_NO_INT128 "disable use of __int128" OFF)
option(ABIEOS_ONLY_LIBRARY "define and build the ABIEOS library" OFF)
option(ABIEOS_BUILD_SHARED_LIB "build ABIEOS share library" ON)
option(ABIEOS_ENABLE_PROTOBUF "enable protobuf support" ON)
option(ABIEOS_DISABLE_TESTS "diables tests" OFF)

if(NOT DEFINED SKIP_SUBMODULE_CHECK)
  execute_process(COMMAND git submodule status --recursive
                  COMMAND grep -c "^[+\-]"
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE submodule_status
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(submodule_status GREATER 0)
     message(FATAL_ERROR "git submodules are not up to date.
Please run the command 'git submodule update --init --recursive'.")
  endif()
endif()

find_package(Threads)
include(GNUInstallDirs)

if (ABIEOS_ENABLE_PROTOBUF)
  if (Linux)
    set(Protobuf_USE_STATIC_LIBS ON)  
  endif()
  find_package(Protobuf 3.15 REQUIRED)
endif()

add_library(abieos STATIC src/abieos.cpp src/abi.cpp src/crypto.cpp include/eosio/fpconv.c)
target_include_directories(abieos PUBLIC 
                          "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include" 
                          "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/external/rapidjson/include"
                          "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

if (Protobuf_FOUND)
  target_compile_definitions(abieos PUBLIC ABIEOS_HAS_PROTOBUF)
  target_link_libraries(abieos PUBLIC protobuf::libprotoc protobuf::libprotobuf)
  target_sources(abieos PRIVATE src/pb_support.cpp)
endif()

if(ABIEOS_NO_INT128)
target_compile_definitions(abieos PUBLIC ABIEOS_NO_INT128)
endif()

if (ABIEOS_BUILD_SHARED_LIB)
  add_library(abieos_module MODULE src/abieos.cpp src/abi.cpp src/crypto.cpp include/eosio/fpconv.c)
  target_include_directories(abieos_module PUBLIC 
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include;" 
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/external/rapidjson/include" 
                            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_link_libraries(abieos_module PUBLIC ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(abieos_module PROPERTIES OUTPUT_NAME "abieos")

  if (Protobuf_FOUND)
    target_compile_definitions(abieos_module PUBLIC ABIEOS_HAS_PROTOBUF)
    target_link_libraries(abieos_module PUBLIC protobuf::libprotoc protobuf::libprotobuf)
    target_sources(abieos_module PRIVATE src/pb_support.cpp)
  endif()
endif()

if (NOT CMAKE_CROSSCOMPILING)
  add_executable(ship_abi_gen src/ship_abi_gen.cpp)
  target_link_libraries(ship_abi_gen abieos ${CMAKE_THREAD_LIBS_INIT})
  add_custom_command(OUTPUT ship_abi.json 
                    COMMAND ship_abi_gen > ship_abi.json 
                    DEPENDS ship_abi_gen)

  add_custom_target(ship_abi_json
    DEPENDS ship_abi.json 
  )

else()
  include(ExternalProject)
  externalproject_add(ship_abi_json
                    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}" "-DCMAKE_BUILD_TYPE=Release"
                    BUILD_COMMAND ${CMAKE_COMMAND} --build . --target install.ship_abi_json
                    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/ship_abi.json
                    INSTALL_COMMAND "")
endif()

add_custom_command(OUTPUT ship_abi.cpp
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/gen_ship_abi_cpp.cmake 
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ship_abi_json 
)

add_library(ship_abi ${CMAKE_CURRENT_BINARY_DIR}/ship_abi.cpp)

# Causes build issues on some platforms
# add_executable(test_abieos_sanitize src/test.cpp src/abieos.cpp src/abi.cpp src/crypto.cpp include/eosio/fpconv.c)
# target_include_directories(test_abieos_sanitize PRIVATE include external/outcome/single-header external/rapidjson/include external/date/include)
# target_link_libraries(test_abieos_sanitize abieos -fno-omit-frame-pointer -fsanitize=address,undefined ${CMAKE_THREAD_LIBS_INIT})
# target_compile_options(test_abieos_sanitize PUBLIC -fno-omit-frame-pointer -fsanitize=address,undefined)
# add_test(NAME test_abieos_sanitize COMMAND test_abieos_sanitize)

# add_executable(fuzzer src/fuzzer.cpp src/abieos.cpp)
# target_include_directories(fuzzer PRIVATE external/outcome/single-header external/rapidjson/include external/date/include)
# target_link_libraries(fuzzer -fsanitize=fuzzer,address,undefined,signed-integer-overflow -fstandalone-debug ${CMAKE_THREAD_LIBS_INIT})
# target_compile_options(fuzzer PRIVATE -fsanitize=fuzzer,address,undefined,signed-integer-overflow -fstandalone-debug)

if (CMAKE_CXX_COMPILER_ID MATCHES Clang|AppleClang)
    target_compile_options(abieos PRIVATE -Wall -Wextra -Wno-unused-parameter -fcolor-diagnostics)
endif()

if (NOT ABIEOS_ONLY_LIBRARY)
add_subdirectory(tools)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio DESTINATION include)

install(TARGETS abieos ship_abi
  EXPORT abieos
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT abieos
  DESTINATION "share/abieos"
  FILE abieos-targets.cmake
)

export(TARGETS abieos ship_abi
       FILE ${CMAKE_CURRENT_BINARY_DIR}/share/abieos/abieos-targets.cmake)
configure_file(abieos-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/share/abieos/abieos-config.cmake COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/abieos/abieos-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/ship_abi.json 
        DESTINATION "share/abieos")

add_custom_target(install.ship_abi_json
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ship_abi.json ${CMAKE_INSTALL_PREFIX}
        DEPENDS ship_abi.json)

if (NOT ABIEOS_DISABLE_TESTS)
  enable_testing()

  add_executable(test_abieos src/test.cpp src/abieos.cpp)
  target_link_libraries(test_abieos abieos ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME test_abieos COMMAND test_abieos)

  if (CMAKE_CXX_COMPILER_ID MATCHES Clang|AppleClang)
    target_compile_options(test_abieos PRIVATE -Wall -Wextra -Wno-unused-parameter -fcolor-diagnostics)
  endif()

  if(NOT ABIEOS_NO_INT128)
      add_executable(test_abieos_template src/template_test.cpp src/abieos.cpp)
      target_link_libraries(test_abieos_template abieos ${CMAKE_THREAD_LIBS_INIT})
      add_test(NAME test_abieos_template COMMAND test_abieos_template)
  endif()

  add_executable(test_abieos_key src/key_test.cpp src/abieos.cpp)
  target_link_libraries(test_abieos_key abieos ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME test_abieos_key COMMAND test_abieos_key)

  add_executable(test_abieos_reflect src/reflect_test.cpp)
  target_include_directories(test_abieos_reflect PRIVATE include)
  add_test(NAME test_abieos_reflect COMMAND test_abieos_reflect)

  add_executable(kv_abi_test src/kv_abi_test.cpp)
  target_link_libraries(kv_abi_test PRIVATE abieos)
  add_test(NAME kv_abi_test COMMAND kv_abi_test)
endif()
