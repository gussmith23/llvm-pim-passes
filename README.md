# LLVM Passes for Processing in Memory Design
_This repository contains code used to implement [my Master's Thesis](https://github.com/gussmith23/masters-thesis/releases/)._

This repository contains a number of LLVM passes which perform various analyses over programs. In this README, we will highlight the most important passes.

## Building
First, make sure you have the dependencies:
- LLVM
- [dg](https://github.com/mchalupa/dg), though it isn't used at the moment. Feel free to refactor the build to remove this depencency.

```shell
git clone https://github.com/gussmith23/llvm-pim-passes
cd llvm-pim-passes
mkdir build && cd build
cmake .. -DLLVM_DIR=/path/to/llvm_build/lib/cmake/llvm
make
```
Building will produce a new directory within the build directory which is filled with LLVM pass libraries. These libraries can then be used in LLVM's `opt` tool via `opt -load <pass-library> --<pass-flag>`. 

### Troubleshooting
As stated above, there's also a dependence on dg which needs to be removed. I think some paths to dg may be hardcoded to my system's layout; you'll have to change these around. 

LLVM is an evolving project. Updates to LLVM in the future may break this code. If you see compiler errors claiming that certain LLVM symbols do not exist, this is likely the cause.

## Passes
In this section, I will describe in more detail the passes in this repository, with special attention given to the more substantial passes.
- [ld-ld-op-st.](ld-ld-op-st) This pass finds basic load-load-op-store patterns as described in my thesis.
- [pim-subgraphs.](pim-subgraphs) This pass finds the more complex offloadable subgraphs, as described in my thesis.
- [dg-def-use.](dg-def-use) An earlier version of the pim-subgraphs pass built with the dg library. I abandoned this pass when I discovered LLVM provided the dependency information I needed already.
- [srivatsa_transforms.](srivatsa_transforms) I inherited this collection of passes from a labmate. They served as early examples and inspiration for my thesis. 
