//===- PRE.cpp - Partial Redundancy Elimination via Lazy Code Motion --------===//

#define DEBUG_TYPE "pre"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
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
#include <map>
#include <algorithm>
using namespace llvm;
using namespace std;

// STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
// STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

typedef pair< pair<Value*, Value*>, unsigned > term_t;
term_t makeTerm(Value* operand1, unsigned opcode, Value* operand2) {
  pair<Value*, Value*> operands(operand1, operand2);
  term_t term(operands, opcode);
  return term;
}

#define term_operand1(term) (term.first.first)
#define term_operand2(term) (term.first.second)
#define term_opcode(term) (term.second)

namespace {
  struct PRE : public FunctionPass {
    static char ID; // Pass identification
    PRE() : FunctionPass(ID) { }

    // Entry point for the overall pre pass
    bool runOnFunction(Function &F);

    std::map<Instruction*, bool> mem_dsafe;
    std::map<Instruction*, bool> mem_earliest;

    std::set<term_t> getPartialRedundantExpressions(Function &F);
    bool Used(Instruction &inst, term_t term);
    bool Transp(Instruction &inst, term_t term);
    bool DSafe(Instruction &inst, term_t term);
    bool Earliest(Instruction &inst, term_t term);
    Instruction* getBinarySuccessor(Instruction *inst);
    std::set<Instruction*> getSuccessors(Instruction *inst);
    std::set<Instruction*> getPredecessors(Instruction *inst);

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

std::set<term_t> PRE::getPartialRedundantExpressions(Function &F) {
  std::set<term_t> partiallyRedundant;

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

      // Term *term = new Term(inst->getOperand(0), inst->getOpcode(), inst->getOperand(1));
      term_t term = makeTerm(inst->getOperand(0), inst->getOpcode(), inst->getOperand(1));
      partiallyRedundant.insert(term);
    } else {
    }
  }
  DEBUG(errs() << "#done: " << partiallyRedundant.size() << "\n");
  return partiallyRedundant;
}

bool PRE::Used(Instruction &inst, term_t term) {
  // compare two operands and opcode
  if (inst.isBinaryOp()) {
    Value* operand1 = inst.getOperand(0);
    Value* operand2 = inst.getOperand(1);
    unsigned opcode = inst.getOpcode();
    if (operand1 == term_operand1(term) &&
        operand2 == term_operand2(term) &&
        opcode == term_opcode(term)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool PRE::Transp(Instruction &inst, term_t term) {
  return true;
}

bool PRE::DSafe(Instruction &inst, term_t term) {
  if (mem_dsafe[&inst]) return mem_dsafe[&inst];

  BasicBlock *bb = inst.getParent();
  Function *f = bb->getParent();

  bool dsafe = false;
  if (bb == &(f->back()) && (&inst) == &(bb->back()))  {  // if n == e
    dsafe = false;
  } else if (Used(inst, term)) {
    dsafe = true;
  } else if (Transp(inst, term)) {
    dsafe = true;
    std::set<Instruction*> successors = getSuccessors(&inst);
    for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
      if (!DSafe(**I, term)) {
        dsafe = false;
        break;
      }
    }
  } else {
    dsafe = false;
  }

  mem_dsafe[&inst] = dsafe;
  return dsafe;
}

bool PRE::Earliest(Instruction &inst, term_t term) {
  if (mem_earliest[&inst]) return mem_earliest[&inst];

  BasicBlock *bb = inst.getParent();
  Function *f = bb->getParent();

  bool earliest = false;
  if (&(f->front()) == bb && &(bb->front()) == &inst) { // if n == s
    earliest = true;
  } else {
    std::set<Instruction*> predecessors = getPredecessors(&inst);
    earliest = false;
    for (auto I = predecessors.begin(), E = predecessors.end(); I != E; ++I) {
      Instruction *m = *I;
      if (!Transp(*m, term)) {
        earliest = true;
        break;
      } else if (!DSafe(*m, term) && Earliest(*m, term)) {
        earliest = true;
        break;
      }
    }
  }

  mem_earliest[&inst] = earliest;
  return earliest;
}

Instruction* PRE::getBinarySuccessor(Instruction *inst) {
  BasicBlock *parent = inst->getParent();
  bool findSelf = false;
  for (auto I = parent->begin(), E = parent->end(); I != E; ++I) {
    Instruction *i = &*I;
    if (i == inst) {
      findSelf = true;
      continue;
    }
    if (findSelf && i->isBinaryOp()) {
      return i;
    }
  }
  return NULL; // no successor found
}


std::set<Instruction*> PRE::getSuccessors(Instruction *inst) {
  std::set<Instruction*> successorsSet;

  BasicBlock *parent = inst->getParent();
  if (&(parent->back()) == inst) {   // the end of basic block
    /*
    TerminatorInst *termInst = parent->getTerminator();
    unsigned numSuccessors = termInst->getNumSuccessors();
    for (unsigned i = 0; i < numSuccessors; i++) {
      BasicBlock *successorBB = termInst->getSuccessor(i);
      Instruction *firstInst = &*(successorBB->begin());
      successorsSet.insert(firstInst);
    }
    */
    for (auto it : successors(parent)) {
      BasicBlock* bb = &*it;
      successorsSet.insert(&(bb->front()));
    }

  } else { // has single successor
    for (auto I = parent->begin(), E = parent->end(); I != E; ++I) {
      if (inst == &*I) {
        I++;
        successorsSet.insert(&*I);
        break;
      }
    }
  }

  return successorsSet;
}

std::set<Instruction*> PRE::getPredecessors(Instruction *inst) {
  std::set<Instruction*> predecessorsSet;

  BasicBlock *parent = inst->getParent();
  if (&(parent->front()) == inst) {   // the begin of basic block
    for (auto it : predecessors(parent)) {
      BasicBlock* bb = &*it;
      predecessorsSet.insert(&(bb->back()));
    }
  } else { // has single successor
    for (auto I = parent->begin(), E = parent->end(); I != E; ++I) {
      if (inst == &*I) {
        I--;
        predecessorsSet.insert(&*I);
        break;
      }
    }
  }

  return predecessorsSet;
}


bool PRE::runOnFunction(Function &F) {

  bool Changed = false;

  DEBUG(errs() << "#### PRE ####\n");
  // for test
  getPartialRedundantExpressions(F);

  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    BasicBlock * block = &*b;
    Instruction *inst = &*(--(--(block->end())));
    DEBUG(errs() << block->back() << "\n");
    DEBUG(errs() << *inst << "\n");
  }

  return Changed;

}
