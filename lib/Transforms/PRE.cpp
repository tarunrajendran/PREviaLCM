//===- PRE.cpp - Partial Redundancy Elimination via Lazy Code Motion --------===//

#define DEBUG_TYPE "pre"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <algorithm>
using namespace llvm;
using namespace std;

STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

namespace {
  struct PRE : public FunctionPass {
    static char ID; // Pass identification
    PRE() : FunctionPass(ID) { }

    // Entry point for the overall pre pass
    bool runOnFunction(Function &F);
    
    std::vector<Instruction*> getPartialRedundantExpressions(Function &F);

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

std::vector<Instruction*> PRE::getPartialRedundantExpressions(Function &F) {
  std::vector<Instruction*> partiallyRedundant;

  // http://llvm.org/docs/ProgrammersManual.html#iterating-over-the-instruction-in-a-function
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (I->isBinaryOp()) { // 
      DEBUG(errs() << "        " << I << "\n");
      partiallyRedundant.push_back(&*I);
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
