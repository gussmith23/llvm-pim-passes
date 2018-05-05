# LLVM Passes for Processing in Memory Design
_This repository contains code used to implement [my Master's Thesis](https://github.com/gussmith23/masters-thesis/releases/). Thesis code documentation: [ms-thesis-doc](https://github.com/gussmith23/ms-thesis-doc)._

This repository contains a number of LLVM passes which perform various analyses over programs.

## Building
First, make sure you have the dependencies:
- LLVM; specifically, [this branch in my fork of LLVM.](https://github.com/gussmith23/llvm/tree/add-cache-intrinsic) One of the passes in the ld-ld-op-st pass library replaces ld-ld-op-st patterns with new instructions which I added into LLVM; thus, my branch is needed. In the future, this project should be refactored to build with mainline LLVM; see [this issue](https://github.com/gussmith23/llvm-pim-passes/issues/10).
- (optional) [dg.](https://github.com/mchalupa/dg). Used by the dg-def-use library. Building this library will be disabled if you do not specify the location of dg.

```shell
git clone https://github.com/gussmith23/llvm-pim-passes
cd llvm-pim-passes
mkdir build && cd build
cmake .. -DLLVM_DIR=/path/to/llvm_build/lib/cmake/llvm
make
```
Building will produce a new directory within the build directory which is filled with LLVM pass libraries. These libraries can then be used in LLVM's `opt` tool via `opt -load <pass-library> --<pass-flag>`. Alternatively, you can use [the data generation tool](https://github.com/gussmith23/masters-thesis-data-generation) which I built for my thesis; the tool simply needs to be passed the location of the directory containing the built LLVM pass libraries. 

### Troubleshooting
LLVM is an evolving project. Updates to LLVM in the future may break this code. If you see compiler errors claiming that certain LLVM symbols do not exist, this is likely the cause.

## Code Map
- [ld-ld-op-st.](ld-ld-op-st) The passes in this file find basic load-load-op-store patterns, as described in my thesis. There is also an unfinished pass which replaces load-load-op-store patterns with custom instructions.
- [pim-subgraphs.](pim-subgraphs) This pass finds the more complex offloadable subgraphs, as described in my thesis.
- [dg-def-use.](dg-def-use) An earlier version of the pim-subgraphs pass built with the dg library. I abandoned this pass when I discovered LLVM provided the dependency information I needed already.
- [old-passes.](old-passes) I inherited this collection of passes from a labmate. They served as early examples and inspiration for my thesis. 
- [util.](util) Utilities shared by the passes.
