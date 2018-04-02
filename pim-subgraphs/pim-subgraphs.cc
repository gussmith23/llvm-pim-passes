#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "pim-instructions-flag.h"

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

  // The offloadable instructions interior to the graph.
  std::set<llvm::Value*> values;

  // The nodes on the frontier of the graph.
  // Rear frontier nodes are nodes with at least one operand that cannot be
  // offloaded to memory.
  // Similarly, frontier nodes are nodes with at least one user that can't be
  // offloaded to memory.
  std::set<llvm::Value*> rearFrontier, frontier;

  friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                       const PimSubgraph& ps) {
    os << "Rear frontier:\n";
    for (auto i : ps.rearFrontier) {
      os << *i << "\n";
    }
    os << "Subgraph:\n";
    for (auto i : ps.values) {
      os << *i << "\n";
    }
    os << "Frontier:\n";
    for (auto i : ps.frontier) {
      os << *i << "\n";
    }
    return os;
  }

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
    std::set<unsigned int> pimInstrsFromFlags = getPimInstructions();
    if (pimInstrsFromFlags.size())
      offloadableInstructions = std::move(pimInstrsFromFlags);

    for (auto& f : m) {
      for (auto& bb : f) {
        for (auto& i : bb) {
          if (canOffload(&i)) sgs.insert(PimSubgraph::getSubgraphFromValue(&i));
        }
      }
    }

    return false;
  }

  void print(llvm::raw_ostream& os, const llvm::Module* m) const override;

 private:
  std::set<std::shared_ptr<PimSubgraph>> sgs;
};

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

/**
 * For a group of llvm::Value*, generate and print two histograms:
 * - The opcode histogram, for those values which are instructions.
 * - the immediate type histogram, for the rest (which we assume are immediates)
 * Does not clear the histograms beforehand.
 */
void updateHists(const std::set<llvm::Value*>& values,
                 std::map<unsigned int, unsigned long>& opcodeHist,
                 std::map<llvm::Type*, unsigned long>& immedateTypeHist) {
  for (auto i : values) {
    if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(i))
      opcodeHist[inst->getOpcode()]++;
    else
      immedateTypeHist[i->getType()]++;
  }
}

void printHists(const std::map<unsigned int, unsigned long>& opcodeHist,
                const std::map<llvm::Type*, unsigned long>& immedateTypeHist,
                const std::string& opcodeHistHeading,
                const std::string& immedateTypeHistHeading,
                llvm::raw_ostream& os) {
  // Print
  os << opcodeHistHeading << "\n";
  for (auto hist_it : opcodeHist)
    os << llvm::Instruction::getOpcodeName(hist_it.first) << "\t"
       << hist_it.second << "\n";
  os << "\n";
  os << immedateTypeHistHeading << "\n";
  for (auto hist_it : immedateTypeHist) {
    hist_it.first->print(os);
    os << "\t" << hist_it.second << "\n";
  }
}

void PimSubgraphPass::print(llvm::raw_ostream& os,
                            const llvm::Module* m) const {
  os << "TOTAL NUMBER OF SUBGRAPHS\n" << sgs.size() << "\n";

  std::map<unsigned int, unsigned long> rearFrontierOpcodeHist, opcodeHist,
      frontierOpcodeHist;
  std::map<llvm::Type*, unsigned long> rearFrontierImmedateTypeHist,
      immedateTypeHist, frontierImmediateTypeHist;

  // This histogram tracks the number of instructions actually offloaded (i.e.
  // sg->values) per subgraph.
  std::map<unsigned long, unsigned long> subgraphSizeHist;

  // Maps size of sg->values to PimSubgraphs, so that we can easily find the
  // largest/smallest subgraphs.
  std::multimap<unsigned long, std::shared_ptr<PimSubgraph>>
      offloadSizeToSubgraph;

  for (auto sg : sgs) {
    subgraphSizeHist[sg->values.size()]++;

    // TODO could be updated to newer C++14 syntax with make_pair
    std::pair<unsigned long, std::shared_ptr<PimSubgraph>> p(sg->values.size(),
                                                             sg);
    offloadSizeToSubgraph.insert(p);

    updateHists(sg->rearFrontier, rearFrontierOpcodeHist,
                rearFrontierImmedateTypeHist);
    updateHists(sg->values, opcodeHist, immedateTypeHist);
    updateHists(sg->frontier, frontierOpcodeHist, frontierImmediateTypeHist);
  }

  os << "SUBGRAPH SIZE HISTOGRAM\n";
  for (auto hist_it : subgraphSizeHist)
    os << hist_it.first << "\t" << hist_it.second << "\n";

  os << "\n";

  os << "LARGEST SUBGRAPHS:\n";
  constexpr int numSgToPrint = 10;
  auto it = offloadSizeToSubgraph.rbegin();
  for (int i = 0; it != offloadSizeToSubgraph.rend() && i < numSgToPrint;
       it++, i++)
    os << *it->second << "\n";

  os << "\n";

  printHists(rearFrontierOpcodeHist, rearFrontierImmedateTypeHist,
             "REAR FRONTIER OPERAND HISTOGRAM",
             "REAR FRONTIER IMMEDIATE TYPE HISTOGRAM", os);
  os << "\n";
  printHists(opcodeHist, immedateTypeHist, "SUBGRAPH OPERAND HISTOGRAM",
             "SUBGRAPH IMMEDIATE TYPE HISTOGRAM", os);
  os << "\n";
  // Note that there shouldn't be any immediates on the frontier (because
  // immediates aren't instructions, and don't take any operands)
  printHists(frontierOpcodeHist, frontierImmediateTypeHist,
             "FRONTIER OPERAND HISTOGRAM", "FRONTIER IMMEDIATE TYPE HISTOGRAM",
             os);
  os << "\n";
}

}  // namespace

char PimSubgraphPass::ID = 0;
static llvm::RegisterPass<PimSubgraphPass> unused("find-pim-subgraphs",
                                                  "Find PIM subgraphs", true,
                                                  true);
