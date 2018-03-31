#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

llvm::cl::opt<std::string> pimInstructions(
    "pi",
    llvm::cl::desc("Comma-separated list of LLVM instructions which can be "
                   "offloaded to memory"),
    llvm::cl::value_desc("inst1,inst2,..."));

namespace {

/**
 * Determine whether an LLVM value (e.g. an instruction or immediate) can be
 * offloaded to memory.
 */
bool canOffload(llvm::Value* value);
std::set<unsigned int> offloadableInstructions = {
    llvm::Instruction::Add,           llvm::Instruction::Sub,
    llvm::Instruction::And,           llvm::Instruction::Or,
    llvm::Instruction::Xor,           llvm::Instruction::Mul,
    llvm::Instruction::BinaryOps::Shl};

/**
 * Represents a connected subgraph of nodes, where each of the nodes can be
 * offloaded to memory.
 */
class PimSubgraph {
 public:
  static std::unordered_map<llvm::Value*, std::shared_ptr<PimSubgraph>>
      pimSubgraphs;

  /**
   * First checks whether node is in pimSubgraphs, so as not to duplicate work.
   * If the node isn't in pimSubgraphs, this function constructs a new
   * PimSubgraph, inserts it into pimSubgraphs, and returns it.
   * Expands outward from node, consuming all offloadable instructions to create
   * a PimSubgraph.
   */
  static std::shared_ptr<PimSubgraph> getSubgraphFromValue(llvm::Value* value);

  std::set<llvm::Value*> values;

  // The nodes on the frontier of the graph.
  // Rear frontier nodes are nodes with at least one operand that cannot be
  // offloaded to memory.
  // Similarly, frontier nodes are nodes with at least one user that can't be
  // offloaded to memory.
  std::set<llvm::Value*> rearFrontier, frontier;

 private:
  PimSubgraph() {}

  /**
   * dir: the direction we were traveling when this call to probe() occured. If
   * this node can't be offloaded to memory, this determines whether the node
   * gets added to the frontier or the rear frontier.
   */
  enum TRAVERSE_DIR { BACKWARD, FORWARD, INIT };
  void probe(llvm::Value* value, TRAVERSE_DIR dir = INIT);
};
// See https://stackoverflow.com/questions/9282354/static-variable-link-error
std::unordered_map<llvm::Value*, std::shared_ptr<PimSubgraph>>
    PimSubgraph::pimSubgraphs;

/***
 * A more complex Def-Use pass built with the dg library.
 */
class PimSubgraphPass : public llvm::ModulePass {
 public:
  static char ID;
  PimSubgraphPass() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module& m) override {
    if (pimInstructions.c_str()) {
      offloadableInstructions.clear();
      char* string = strdup(pimInstructions.c_str());
      char* token = strtok(string, ",");
      while (token != NULL) {
        if (strcmp(token, "add") == 0)
          offloadableInstructions.insert(llvm::Instruction::Add);
        else if (strcmp(token, "sub") == 0)
          offloadableInstructions.insert(llvm::Instruction::Sub);
        else if (strcmp(token, "and") == 0)
          offloadableInstructions.insert(llvm::Instruction::And);
        else if (strcmp(token, "or") == 0)
          offloadableInstructions.insert(llvm::Instruction::Or);
        else if (strcmp(token, "mul") == 0)
          offloadableInstructions.insert(llvm::Instruction::Mul);
        else if (strcmp(token, "shl") == 0)
          offloadableInstructions.insert(llvm::Instruction::Shl);
        else if (strcmp(token, "ashr") == 0)
          offloadableInstructions.insert(llvm::Instruction::AShr);
        else if (strcmp(token, "lshr") == 0)
          offloadableInstructions.insert(llvm::Instruction::LShr);
        else if (strcmp(token, "sdiv") == 0)
          offloadableInstructions.insert(llvm::Instruction::SDiv);
        else if (strcmp(token, "udiv") == 0)
          offloadableInstructions.insert(llvm::Instruction::UDiv);
        else if (strcmp(token, "srem") == 0)
          offloadableInstructions.insert(llvm::Instruction::SRem);
        else if (strcmp(token, "urem") == 0)
          offloadableInstructions.insert(llvm::Instruction::URem);
        else if (strcmp(token, "sext") == 0)
          offloadableInstructions.insert(llvm::Instruction::SExt);
        else if (strcmp(token, "zext") == 0)
          offloadableInstructions.insert(llvm::Instruction::ZExt);
        token = strtok(NULL, ",");
      }
      free(string);
    }

    for (auto& f : m) {
      for (auto& bb : f) {
        for (auto& i : bb) {
          if (canOffload(&i)) sgs.insert(PimSubgraph::getSubgraphFromValue(&i));
        }
      }
    }

    return false;
  }

  void print(llvm::raw_ostream& os, const llvm::Module* m) const override {
    os << "Total number of subgraphs:\n" << sgs.size() << "\n";

    for (auto sg : sgs) {
      os << "\nSubgraph " << sg.get() << "\n";

      os << "\nRear frontier:\n";
      for (auto i : sg->rearFrontier) {
        os << *i << "\n";
      }

      /*
      std::unordered_map<unsigned int, unsigned long> rearFrontierOpcodeHist;
      for (auto i : sg->rearFrontier) {
        rearFrontierOpcodeHist[i->
      }*/

      os << "\nSubgraph:\n";
      for (auto i : sg->values) {
        os << *i << "\n";
      }

      os << "\nFrontier:\n";
      for (auto i : sg->frontier) {
        os << *i << "\n";
      }
    }
  }

 private:
  std::set<std::shared_ptr<PimSubgraph>> sgs;
};

/*
void runOnDependenceGraph(dg::LLVMDependenceGraph* dg) {
  if (dg == nullptr) return;

  // We're going to recurse, so keep track of the subgraphs we've already
  // recursed into.
  if (seen.find(dg) != seen.end())
    return;
  else
    seen.insert(dg);

  for (auto bb_it : dg->getBlocks()) {
    for (dg::LLVMNode* node : bb_it.second->getNodes()) {
      // Depending on what kind of instruction it is, do something.

      // If it's a call, we have to recurse into its subgraphs.
      if (llvm::dyn_cast<llvm::CallInst>(node->getValue())) {
        for (auto subgraph : node->getSubgraphs())
          runOnDependenceGraph(subgraph);
      }

      else if (llvm::Instruction* inst =
                   llvm::dyn_cast<llvm::Instruction>(node->getValue())) {
        if (offloadableInstructions.find(inst->getOpcode()) !=
            offloadableInstructions.end()) {
          llvm::errs() << "Found supported instruction:\n" << *inst << "\n";
          PimSubgraph::getSubgraphFromNode(node);
        }
      }
    }
  }
}*/

std::shared_ptr<PimSubgraph> PimSubgraph::getSubgraphFromValue(
    llvm::Value* value) {
  if (pimSubgraphs.count(value)) return pimSubgraphs[value];

  // Because constructor is private, we can't use make_shared.
  std::shared_ptr<PimSubgraph> sg =
      std::shared_ptr<PimSubgraph>(new PimSubgraph());
  sg->probe(value);

  for (llvm::Value* value : sg->values) pimSubgraphs[value] = sg;
  for (llvm::Value* value : sg->rearFrontier) pimSubgraphs[value] = sg;
  for (llvm::Value* value : sg->frontier) pimSubgraphs[value] = sg;

  return std::move(sg);
}

void PimSubgraph::probe(llvm::Value* value, TRAVERSE_DIR dir) {
  // TODO should this happen?
  if (values.count(value)) return;

  if (canOffload(value)) {
    values.insert(value);
  } else if (dir == BACKWARD) {
    rearFrontier.insert(value);
    return;
  } else if (dir == FORWARD) {
    frontier.insert(value);
    return;
  } else {
    // TODO should we be able to get here?
    // It depends on how we call the function. If it's called on a node that
    // can't be offloaded, then we can get here. I don't intend to use it that
    // way, so this should be unreachable.
    assert(false);
    return;
  }

  if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(value)) {
    for (auto& use : inst->operands()) {
      probe(use, BACKWARD);
    }
  }
  for (auto user : value->users()) {
    probe(user, FORWARD);
  }
}

bool canOffload(llvm::Value* value) {
  if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(value))
    if (offloadableInstructions.count(inst->getOpcode())) return true;
  return false;
}

}  // namespace

char PimSubgraphPass::ID = 0;
static llvm::RegisterPass<PimSubgraphPass> unused("find-pim-subgraphs",
                                                  "Find PIM subgraphs", true,
                                                  true);
