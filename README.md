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
Produces a directory filled with LLVM pass libraries.

## Troubleshooting/Warnings
As stated above, there's also a dependence on dg which needs to be removed. I think some paths to dg may be hardcoded to my system's layout; you'll have to change these around. 

LLVM is an evolving project. Updates to LLVM in the future may break this code. If you see compiler errors claiming that certain LLVM symbols do not exist, this is likely the cause.
