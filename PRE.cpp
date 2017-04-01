//===- PRE.cpp - Partial Redundancy Elimination via Lazy Code Motion --------===//

#define DEBUG_TYPE "pre"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
using namespace llvm;
using namespace std;

// STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
// STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

class Term {
public:
  Value* operand1;
  unsigned opcode;
  Value* operand2;
  Term(Value* v1, unsigned opcode, Value* v2) {
    this->operand1 = v1;
    this->opcode = opcode;
    this->operand2 = v2;
  }

  bool operator==(const Term &R) {
    DEBUG(errs() << "#comparison: " << "\n");
    return (this->operand1 == R.operand1 &&
          this->opcode == R.opcode &&
          this->operand1 == R.operand2);
  }

  bool operator<(const Term &R) {
    DEBUG(errs() << "#comparison<: " << "\n");
    return (this->operand1 == R.operand1 &&
          this->opcode == R.opcode &&
          this->operand1 == R.operand2);
  }
};

namespace {
  struct PRE : public FunctionPass {
    static char ID; // Pass identification
    PRE() : FunctionPass(ID) { }

    // Entry point for the overall pre pass
    bool runOnFunction(Function &F);

    std::unordered_set<Term*> getPartialRedundantExpressions(Function &F);

    // getAnalysisUsage - List passes required by this pass.  We also know it
    // will not alter the CFG, so say so.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
    }

  private:
    // Add fields and helper functions for this pass here.
  };
}

char PRE::ID = 0;
static RegisterPass<PRE> X("pre",
			    "Partial Redundancy Elimination via LCM (CS526)",
			    false /* does not modify the CFG */,
			    false /* transformation, not just analysis */);


// Public interface to create the PartialRedundancyElimination pass.
// This function is provided to you.
FunctionPass *createPartialRedundancyEliminationPass() { return new PRE(); }


//===----------------------------------------------------------------------===//
//                      SKELETON FUNCTION TO BE IMPLEMENTED
//===----------------------------------------------------------------------===//
//
// Function runOnFunction:
// Entry point for the overall PartialRedundancyElimination function pass.
// This function is provided to you.

std::unordered_set<Term*> PRE::getPartialRedundantExpressions(Function &F) {
  std::unordered_set<Term*> partiallyRedundant;

  // http://llvm.org/docs/ProgrammersManual.html#iterating-over-the-instruction-in-a-function
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    DEBUG(errs() << "#inst: " << *inst << "\n");
    if (inst->isBinaryOp()) { //
      DEBUG(errs() << "#binary inst: " << *inst << "\n");
      DEBUG(errs() << "  #operands: " << inst->getNumOperands() << "\n");
      DEBUG(errs() << "  #operand: " << *(inst->getOperand(0)) << "\n");
      DEBUG(errs() << "  #operand: " << *(inst->getOperand(1)) << "\n");
      DEBUG(errs() << "  #opcode: " << *(inst->getOpcodeName()) << "\n");

      Term *term = new Term(inst->getOperand(0), inst->getOpcode(), inst->getOperand(1));
      partiallyRedundant.insert(term);
    } else {
    }
  }

  DEBUG(errs() << "#done: " << partiallyRedundant.size() << "\n");
  if (*(partiallyRedundant.begin()) == *(++(partiallyRedundant.begin()))) {
    DEBUG(errs() << "#equal\n");
  } else {
    DEBUG(errs() << "#notequal\n");
    DEBUG(errs() << *((*(partiallyRedundant.begin()))->operand1) << "\n" );
    DEBUG(errs() << *((*(++(partiallyRedundant.begin())))->operand1) << "\n" );
    if ( (((*(partiallyRedundant.begin()))->operand1)) == (((*(++(partiallyRedundant.begin())))->operand1)) ) {
      DEBUG(errs() << "operands equal\n");
    }
  }

  return partiallyRedundant;
}

bool PRE::runOnFunction(Function &F) {

  bool Changed = false;

  // for test
  getPartialRedundantExpressions(F);

  return Changed;

}
