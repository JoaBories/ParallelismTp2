# Inclusion commune pour un exercice TP (C++20 + pthread).
# Avant include : définir TP_SOURCES (liste de .cpp relatifs au CMakeLists courant).

cmake_minimum_required(VERSION 3.10)
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

if(NOT DEFINED TP_SOURCES)
  message(FATAL_ERROR "TP_SOURCES doit être défini avant d'inclure TpCommon.cmake")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(FATAL_ERROR "Ce TP se compile avec gcc (CXX=${CMAKE_CXX_COMPILER_ID}).")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(TP_BIN tp)
else()
  file(RELATIVE_PATH TP_BIN "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
  string(REPLACE "/" "_" TP_BIN "${TP_BIN}")
endif()

find_package(Threads REQUIRED)

add_executable(${TP_BIN} ${TP_SOURCES})
set_target_properties(${TP_BIN} PROPERTIES
  OUTPUT_NAME tp
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${TP_BIN} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(${TP_BIN} PRIVATE Threads::Threads)
target_compile_options(${TP_BIN} PRIVATE -Wall)
