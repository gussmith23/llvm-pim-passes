add_library(OffloadableInstructions SHARED
            ./offloadable-instructions.h
            ./offloadable-instructions.cc)
target_compile_features(OffloadableInstructions PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(OffloadableInstructions PROPERTIES
  COMPILE_FLAGS "-fno-rtti"
  LIBRARY_OUTPUT_DIRECTORY ${PASS_LIBRARY_OUTPUT_DIR}
)
target_include_directories(OffloadableInstructions PRIVATE ${LLVM_INCLUDE_DIRS})
