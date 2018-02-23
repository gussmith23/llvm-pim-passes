# From https://github.com/abenkhadra/llvm-pass-tutorial
cmake_minimum_required(VERSION 3.4)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

set(PASS_LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/pass-libs)

function(add_pass PASS_NAME PASS_SOURCES)
  add_library(${PASS_NAME} MODULE ${PASS_SOURCES})
  target_compile_features(${PASS_NAME} PRIVATE cxx_range_for cxx_auto_type)
  set_target_properties(${PASS_NAME} PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
    LIBRARY_OUTPUT_DIRECTORY ${PASS_LIBRARY_OUTPUT_DIR}
  )
endfunction(add_pass)

add_subdirectory(pim-pass)
add_subdirectory(srivatsa_transforms)
