add_library(PimInstructionsFlag SHARED
            ./pim-instructions-flag.h
            ./pim-instructions-flag.cpp)
target_compile_features(PimInstructionsFlag PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(PimInstructionsFlag PROPERTIES
  COMPILE_FLAGS "-fno-rtti"
  LIBRARY_OUTPUT_DIRECTORY ${PASS_LIBRARY_OUTPUT_DIR}
)
target_include_directories(PimInstructionsFlag PRIVATE ${LLVM_INCLUDE_DIRS})
