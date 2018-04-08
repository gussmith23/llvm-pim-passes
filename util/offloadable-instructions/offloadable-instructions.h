#include <llvm/IR/Value.h>

/**
 * Determine whether an LLVM value (e.g. an instruction or immediate) can be
 * offloaded to memory.
 */
bool canOffload(llvm::Value* value);
