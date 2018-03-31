#include <iostream>
#include <memory>
#include <set>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/LLVMDependenceGraph.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/analysis/DefUse.h>
#include <llvm/analysis/PointsTo/PointsTo.h>
#include <llvm/analysis/ReachingDefinitions/ReachingDefinitions.h>

namespace {

void runOnDependenceGraph(dg::LLVMDependenceGraph* dg);

/**
 * Determine whether an LLVM value (e.g. an instruction or immediate) can be
 * offloaded to memory.
 */
bool canOffload(const dg::LLVMNode* node);
std::set<unsigned int> offloadableInstructions = {
    llvm::Instruction::Add, llvm::Instruction::Sub, llvm::Instruction::And,
    llvm::Instruction::Or,  llvm::Instruction::Xor, llvm::Instruction::Mul,
};

/**
 * Represents a connected subgraph of nodes, where each of the nodes can be
 * offloaded to memory.
 */
class PimSubgraph {
  static std::unordered_map<dg::LLVMNode*, std::shared_ptr<PimSubgraph>>
      pimSubgraphs;

 public:
  /**
   * First checks whether node is in pimSubgraphs, so as not to duplicate work.
   * If the node isn't in pimSubgraphs, this function constructs a new
   * PimSubgraph, inserts it into pimSubgraphs, and returns it.
   * Expands outward from node, consuming all offloadable instructions to create
   * a PimSubgraph.
   */
  static std::shared_ptr<PimSubgraph> getSubgraphFromNode(dg::LLVMNode* node);

  std::set<dg::LLVMNode*> nodes;

  // The nodes on the frontier of the graph.
  // Rear frontier nodes are nodes with at least one operand that cannot be
  // offloaded to memory.
  // Similarly, frontier nodes are nodes with at least one user that can't be
  // offloaded to memory.
  std::set<dg::LLVMNode*> rearFrontier, frontier;

 private:
  PimSubgraph() {}

  /**
   * dir: the direction we were traveling when this call to probe() occured. If
   * this node can't be offloaded to memory, this determines whether the node
   * gets added to the frontier or the rear frontier.
   */
  enum TRAVERSE_DIR { BACKWARD, FORWARD, INIT };
  void probe(dg::LLVMNode* node, TRAVERSE_DIR dir = INIT);
};
// See https://stackoverflow.com/questions/9282354/static-variable-link-error
std::unordered_map<dg::LLVMNode*, std::shared_ptr<PimSubgraph>>
    PimSubgraph::pimSubgraphs;

/***
 * A more complex Def-Use pass built with the dg library.
 */
class DgDefUse : public llvm::ModulePass {
 public:
  static char ID;
  DgDefUse() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module& module) {
    // We have to do pointer + reaching definition analysis before we can build
    // a graph with data dependencies. Actually, it looks like the specific
    // order is: pointer analysis, which building the graph depends on. Then
    // reaching definitions. Then def-use analysis, which depends on built
    // graph, pointer analysis, and reaching defs.

    std::unique_ptr<dg::LLVMPointerAnalysis> pta(
        new dg::LLVMPointerAnalysis(&module));
    // TODO just picked a setting at random -- which should we use?
    pta->run<dg::analysis::pta::PointsToFlowSensitive>();

    std::unique_ptr<dg::analysis::rd::LLVMReachingDefinitions> rd(
        new dg::analysis::rd::LLVMReachingDefinitions(&module, pta.get()));
    rd->run<dg::analysis::rd::ReachingDefinitionsAnalysis>();

    std::unique_ptr<dg::LLVMDependenceGraph> dg(new dg::LLVMDependenceGraph());
    dg->build(&module, pta.get());

    dg::LLVMDefUseAnalysis dua(dg.get(), rd.get(), pta.get());
    dua.run();

    llvm::errs() << "Built dependence graph.\n";

    runOnDependenceGraph(dg.get());

    return false;
  }
};

std::set<dg::LLVMDependenceGraph*> seen;

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
}

std::shared_ptr<PimSubgraph> PimSubgraph::getSubgraphFromNode(
    dg::LLVMNode* node) {
  if (pimSubgraphs.count(node)) return pimSubgraphs[node];

  // Because constructor is private, we can't use make_shared.
  std::shared_ptr<PimSubgraph> sg =
      std::shared_ptr<PimSubgraph>(new PimSubgraph());
  sg->probe(node);

  for (dg::LLVMNode* node : sg->nodes) pimSubgraphs[node] = sg;
  for (dg::LLVMNode* node : sg->rearFrontier) pimSubgraphs[node] = sg;
  for (dg::LLVMNode* node : sg->frontier) pimSubgraphs[node] = sg;

  return std::move(sg);
}

void PimSubgraph::probe(dg::LLVMNode* node, TRAVERSE_DIR dir) {
  llvm::errs() << "probing: " << *node->getValue() << "\n";

  // TODO should this happen?
  if (nodes.count(node)) return;

  if (canOffload(node)) {
    nodes.insert(node);
  } else if (dir == BACKWARD) {
    rearFrontier.insert(node);
    return;
  } else if (dir == FORWARD) {
    frontier.insert(node);
    return;
  } else {
    // TODO should we be able to get here?
    // It depends on how we call the function. If it's called on a node that
    // can't be offloaded, then we can get here. I don't intend to use it that
    // way, so this should be unreachable.
    assert(false);
    return;
  }

  for (dg::LLVMNode::data_iterator it = node->rev_data_begin();
       it != node->rev_data_end(); it++) {
    probe(*it, BACKWARD);
  }
  for (dg::LLVMNode::data_iterator it = node->data_begin();
       it != node->data_end(); it++) {
    probe(*it, FORWARD);
  }
}

bool canOffload(const dg::LLVMNode* node) {
  if (llvm::Instruction* inst =
          llvm::dyn_cast<llvm::Instruction>(node->getValue()))
    if (offloadableInstructions.count(inst->getOpcode())) return true;
  return false;
}

}  // namespace

char DgDefUse::ID = 0;
static llvm::RegisterPass<DgDefUse> unused("dg-def-use", "DG Def Use");
