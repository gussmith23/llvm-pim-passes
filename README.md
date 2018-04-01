## Dependencies
- LLVM
- [dg](https://github.com/mchalupa/dg), though it isn't used at the moment. Feel free to refactor the build to remove this depencency.
## Building
```shell
git clone https://github.com/gussmith23/llvm-pim-passes
cd llvm-pim-passes
mkdir build && cd build
cmake .. -DLLVM_DIR=/path/to/llvm_build/lib/cmake/llvm
make
```
Produces a directory filled with LLVM pass libraries.

## Troubleshooting/Warnings
As stated above, there's also a dependence on dg which needs to be removed. I think some paths to dg may be hardcoded to my system's layout; you'll have to change these around. 
