# Again, this comes from https://github.com/abenkhadra/llvm-pass-tutorial
add_library(PimPass MODULE pim-pass.cc)

# Target C++11. Do we just pick features at random, or are these features
# actually needed?
target_compile_features(PimPass PRIVATE cxx_range_for cxx_auto_type)

# Otherwise, we get linker errors.
# TODO understand this more.
set_target_properties(PimPass PROPERTIES
  COMPILE_FLAGS "-fno-rtti"
)
