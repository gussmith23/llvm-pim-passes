# From https://github.com/abenkhadra/llvm-pass-tutorial
cmake_minimum_required(VERSION 3.4)

# Brute-force fix
add_definitions(-DHAVE_LLVM -DENABLE_CFG)

find_package(LLVM REQUIRED CONFIG)

set(PASS_LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/pass-libs)

function(add_pass PASS_NAME PASS_SOURCES)
  add_library(${PASS_NAME} MODULE ${PASS_SOURCES})
  target_compile_features(${PASS_NAME} PRIVATE cxx_range_for cxx_auto_type)
  set_target_properties(${PASS_NAME} PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
    LIBRARY_OUTPUT_DIRECTORY ${PASS_LIBRARY_OUTPUT_DIR}
  )

  target_include_directories(${PASS_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})
  # See https://github.com/mchalupa/dg/issues/182 to understand why this
  # is commented out.
  # TODO I don't get why this isn't breaking things...
  #llvm_map_components_to_libnames(LLVM_LIBS core)#support core irreader)
  #target_link_libraries(${PASS_NAME} PRIVATE ${LLVM_LIBS})
endfunction(add_pass)

add_subdirectory(./util)

# Add the passes
add_subdirectory(pim-pass)
# TODO broken by change in LLVM API
#add_subdirectory(srivatsa_transforms)
add_subdirectory(test-module-pass)
add_subdirectory(./dg-test)
add_subdirectory(./ld-ld-op-st)
add_subdirectory(./dg-def-use)
add_subdirectory(./pim-subgraphs)
