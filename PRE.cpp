//===- PRE.cpp - Partial Redundancy Elimination via Lazy Code Motion --------===//

#define DEBUG_TYPE "pre"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/PostOrderIterator.h"
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

    std::set<Instruction*> mem_used;
    std::map<Instruction*, bool> mem_dsafe;
    std::map<Instruction*, bool> mem_earliest;
    std::map<Instruction*, bool> mem_delay;
    std::map<Instruction*, bool> mem_latest;

    std::set<term_t> getPartialRedundantExpressions(Function &F);
    bool Used(Instruction &inst, term_t term);
    bool Transp(Instruction &inst, term_t term);
    bool DSafe(Instruction &inst, term_t term);
    bool Earliest(Instruction &inst, term_t term);
    bool Delay(Instruction &inst, term_t term);
    bool Latest(Instruction &inst, term_t term);
    Instruction* getBinarySuccessor(Instruction *inst);
    std::set<Instruction*> getSuccessors(Instruction *inst);
    std::set<Instruction*> getPredecessors(Instruction *inst);
    void performSafeEarliestTransform(Function &F, term_t term);
    void getDSafes(Function &F, term_t term);
    void getEarliests(Function &F, term_t term);

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
    DEBUG(dbgs() << "#inst: " << *inst << "\n");
    if (inst->isBinaryOp()) { //
      DEBUG(dbgs() << "#binary inst: " << *inst << "\n");
      DEBUG(dbgs() << "  #operands: " << inst->getNumOperands() << "\n");
      DEBUG(dbgs() << "  #operand: " << *(inst->getOperand(0)) << "\n");
      DEBUG(dbgs() << "  #operand: " << *(inst->getOperand(1)) << "\n");
      DEBUG(dbgs() << "  #opcode: " << *(inst->getOpcodeName()) << "\n");

      // Term *term = new Term(inst->getOperand(0), inst->getOpcode(), inst->getOperand(1));
      term_t term = makeTerm(inst->getOperand(0), inst->getOpcode(), inst->getOperand(1));
      partiallyRedundant.insert(term);
    } else {
    }
  }
  DEBUG(dbgs() << "#done: " << partiallyRedundant.size() << "\n");
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
      mem_used.insert(&inst);
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool PRE::Transp(Instruction &inst, term_t term) {
  Value* operand1 = term_operand1(term);
  Value* operand2 = term_operand2(term);
  if ((&inst != operand1) && (&inst != operand2)) {
    return true;
  } else {
    return false;
  }
}

bool PRE::DSafe(Instruction &inst, term_t term) {
  if (mem_dsafe.find(&inst) != mem_dsafe.end()) return mem_dsafe[&inst];

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
  if (mem_earliest.find(&inst) != mem_earliest.end()) return mem_earliest[&inst];

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

bool PRE::Delay(Instruction &inst, term_t term) {
  if (mem_delay.find(&inst) != mem_delay.end()) return mem_delay[&inst];

  bool delay = false;
  if (mem_dsafe[&inst] && mem_earliest[&inst]) {
    delay = true;
  } else {
    BasicBlock *bb = inst.getParent();
    Function *f = bb->getParent();

    if (&(f->front()) == bb && &(bb->front()) == &inst) { // if n == s
      delay = false;
    } else {
      delay = true;
      std::set<Instruction*> predecessors = getPredecessors(&inst);
      for (auto I = predecessors.begin(), E = predecessors.end(); I != E; ++I) {
        Instruction *m = *I;
        if (!Used(*m, term) && Delay(*m, term)) continue;

        delay = false;
        break;
      }
    }
  }

  mem_delay[&inst] = delay;
  return delay;
}

bool PRE::Latest(Instruction &inst, term_t term) {
  if (mem_latest.find(&inst) != mem_latest.end()) return mem_latest[&inst];

  bool latest = true;
  if (!Delay(inst, term)) {
    latest = false;
  } else if (Used(inst, term)) {
    latest = true;
  } else {
    bool flag = true;
    std::set<Instruction*> successors = getSuccessors(&inst);
    for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
      Instruction *m = *I;
      if (Delay(*m, term)) continue;

      flag = false;
      break;
    }
    latest = !flag;
  }

  mem_latest[&inst] = latest;
  return latest;
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

void PRE::performSafeEarliestTransform(Function &F, term_t term) {
  DEBUG(dbgs() << "#performSafeEarliestTransform\n");
  DEBUG(dbgs() << "    term: " << *(term_operand1(term)) << " " << term_opcode(term) << " " << *(term_operand2(term)) << "\n");
  mem_used.clear();
  getDSafes(F, term);
  getEarliests(F, term);

  // print out mem_dsafe and mem_earliest for testing

  DEBUG(dbgs() << "    #DSafe & Earliest:\n");
  for (auto &dsafe_pair : mem_dsafe) {
    Instruction* inst = dsafe_pair.first;
    bool dsafe = dsafe_pair.second;
    bool earliest = mem_earliest[inst];
    DEBUG(dbgs() << "    " << *inst << " | dsafe: " << dsafe << ", earliest: " << earliest << "\n");
  }

  // insert instruction that is both earliest and
  // and update term to load inst
  Value *val;
  ReversePostOrderTraversal<Function *> RPOT(&F);
  for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                           RE = RPOT.end();
       RI != RE; ++RI) {
    BasicBlock * bb = *RI;
    for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
      Instruction * inst = &*it;
      if (mem_dsafe[inst] && mem_earliest[inst]) {
        // insert instruction
        Value* binaryOperator = dyn_cast<Value>(BinaryOperator::Create((Instruction::BinaryOps)(term_opcode(term)), term_operand1(term), term_operand2(term), Twine(), inst));
        Type *binaryOperatorType = binaryOperator->getType();

        Value* allocaInst = dyn_cast<Value>(new AllocaInst(binaryOperatorType, Twine(), inst));

        (void)dyn_cast<Value>(new StoreInst(binaryOperator, allocaInst, inst));

        val = allocaInst;

      }
      if (mem_used.find(inst) != mem_used.end()) {
        auto nextIt = ++it;
        LLVMContext & C = inst->getModule()->getContext();
        IRBuilder<> IRB(C);
        DEBUG(dbgs() << "@@ " << *val << "\n");
        auto loadInst = IRB.CreateLoad(val, Twine());
        DEBUG(dbgs() << "    replace to: " << *loadInst << "\n");
        ReplaceInstWithInst(inst, loadInst); // replace with load instruction.
        DEBUG(dbgs() << "    done replacing" << *loadInst << "\n");
        nextIt = --nextIt;
      }
    }
  }

  DEBUG(dbgs() << "#Done performScalarPREInsertion\n");
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    DEBUG(dbgs() << *inst << "\n");
  }
}

// backwards
void PRE::getDSafes(Function &F, term_t term) {
  mem_dsafe.clear();
  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock * bb = *I;
    for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
      Instruction * inst = &*it;
      DSafe(*inst, term);
    }
  }
}

// forwards
void PRE::getEarliests(Function &F, term_t term) {
  mem_earliest.clear();

  ReversePostOrderTraversal<Function *> RPOT(&F);
  for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                           RE = RPOT.end();
       RI != RE; ++RI) {
    BasicBlock * bb = *RI;
    for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
      Instruction * inst = &*it;
      Earliest(*inst, term);
    }
  }
}


bool PRE::runOnFunction(Function &F) {

  bool Changed = false;

  DEBUG(dbgs() << "#### PRE ####\n");

  // for test
  std::set<term_t> terms = getPartialRedundantExpressions(F);
  for (auto term : terms) {
    performSafeEarliestTransform(F, term);
  }

  /*
  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    BasicBlock * block = &*b;
    Instruction *inst = &*(--(--(block->end())));
    DEBUG(dbgs() << block->back() << "\n");
    DEBUG(dbgs() << *inst << "\n");
  }
  */

  return Changed;

}
