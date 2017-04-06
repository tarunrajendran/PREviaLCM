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

typedef pair< pair< pair<Value*, Value*>, unsigned >, Type* > term_t;
term_t makeTerm(Value* operand1, unsigned opcode, Value* operand2, Type* type) {
  pair<Value*, Value*> operands(operand1, operand2);
  pair< pair<Value*, Value*>, unsigned > real_term(operands, opcode);
  term_t term(real_term, type);
  return term;
}

#define term_operand1(term) (term.first.first.first)
#define term_operand2(term) (term.first.first.second)
#define term_opcode(term) (term.first.second)
#define term_type(term) (term.second)

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
    std::map<Instruction*, bool> mem_isolated;

    std::set<term_t> getPartialRedundantExpressions(Function &F);
    Value* getAlloca(Value* val);
    bool Used(Instruction &inst, term_t term);
    bool Transp(Instruction &inst, term_t term);
    bool DSafe(Instruction &inst, term_t term);
    bool Earliest(Instruction &inst, term_t term);
    bool Delay(Instruction &inst, term_t term);
    bool Latest(Instruction &inst, term_t term);
    bool Isolated(Instruction &inst, term_t term);
    Instruction* getBinarySuccessor(Instruction *inst);
    std::set<Instruction*> getSuccessors(Instruction *inst);
    std::set<Instruction*> getPredecessors(Instruction *inst);
    void getDSafes(Function &F, term_t term);
    void getEarliests(Function &F, term_t term);
    void getDelays(Function &F, term_t term);
    void getLatests(Function &F, term_t term);
    void getIsolateds(Function &F, term_t term);
    std::set<Instruction*> getOCP(Function &F, term_t term);
    std::set<Instruction*> getRO(Function &F, term_t term);
    bool perform_OCP_RO_Transformation(Function &F, term_t term);

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
      Value* operand1 = inst->getOperand(0);
      Value* operand2 = inst->getOperand(1);
      Value* alloca1 = getAlloca(operand1);
      Value* alloca2 = getAlloca(operand2);
      if (alloca1 && alloca2) {
        term_t term = makeTerm(alloca1, inst->getOpcode(), alloca2, inst->getType());
        partiallyRedundant.insert(term);
      }
    } else {
    }
  }
  DEBUG(dbgs() << "#done: " << partiallyRedundant.size() << "\n");
  return partiallyRedundant;
}

Value* PRE::getAlloca(Value* val) {
  LoadInst* loadInst = dyn_cast<LoadInst>(val);
  if (loadInst) {
    return dyn_cast<Value>(loadInst->getOperand(0));
  } else {
    return NULL;
  }
}


bool PRE::Used(Instruction &inst, term_t term) {
  // compare two operands and opcode
  if (inst.isBinaryOp()) {
    Value* operand1 = getAlloca(inst.getOperand(0));
    Value* operand2 = getAlloca(inst.getOperand(1));
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

  StoreInst* storeInst = dyn_cast<StoreInst>(&inst);
  if (storeInst) {
    if (storeInst->getOperand(0) == operand1 || storeInst->getOperand(1) == operand2) {
      return false;
    } else {
      return true;
    }
  } else {
    return true;
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
  if (DSafe(inst, term) && Earliest(inst, term)) {
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

bool PRE::Isolated(Instruction &inst, term_t term) {
  if (mem_isolated.find(&inst) != mem_isolated.end()) return mem_isolated[&inst];

  bool isolated = true;
  std::set<Instruction*> successors = getSuccessors(&inst);
  for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
    Instruction *m = *I;
    if (Latest(*m, term) ||
       (!Used(*m, term) &&
        Isolated(*m, term))
    ) continue;

    isolated = false;
    break;
  }

  mem_isolated[&inst] = isolated;
  return isolated;
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

void PRE::getDelays(Function &F, term_t term) {
  mem_delay.clear();

  ReversePostOrderTraversal<Function *> RPOT(&F);
  for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                           RE = RPOT.end();
       RI != RE; ++RI) {
    BasicBlock * bb = *RI;
    for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
      Instruction * inst = &*it;
      Delay(*inst, term);
    }
  }
}

void PRE::getLatests(Function &F, term_t term) {
  mem_latest.clear();

  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock * bb = *I;
    for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
      Instruction * inst = &*it;
      Latest(*inst, term);
    }
  }
}

void PRE::getIsolateds(Function &F, term_t term) {
  mem_isolated.clear();

  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock * bb = *I;
    for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
      Instruction * inst = &*it;
      Isolated(*inst, term);
    }
  }
}

std::set<Instruction*> PRE::getOCP(Function &F, term_t term) {
  std::set<Instruction*> OCP;

  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *n = &*I;
    if (Latest(*n, term) && !Isolated(*n, term)) {
      OCP.insert(n);
    }
  }

  return OCP;
}

std::set<Instruction*> PRE::getRO(Function &F, term_t term) {
  std::set<Instruction*> RO;

  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *n = &*I;
    if (Used(*n, term) && !(Latest(*n, term) && Isolated(*n, term))) {
      RO.insert(n);
    }
  }

  return RO;
}

bool PRE::perform_OCP_RO_Transformation(Function &F, term_t term) {
  bool Changed = false;
  DEBUG(dbgs() << "#perform_OCP_RO_Transformation\n");
  DEBUG(dbgs() << "    term: " << *(term_operand1(term)) << " " << term_opcode(term) << " " << *(term_operand2(term)) << "\n");
  getDSafes(F, term);
  getEarliests(F, term);
  getDelays(F, term);
  getLatests(F, term);
  getIsolateds(F, term);

  DEBUG(dbgs() << "#Stats\n");
  for (auto &dsafe_pair : mem_dsafe) {
    Instruction* inst = dsafe_pair.first;
    bool dsafe = dsafe_pair.second;
    bool earliest = mem_earliest[inst];
    bool delay = mem_delay[inst];
    bool latest = mem_latest[inst];
    bool isolated = mem_isolated[inst];
    DEBUG(dbgs() << "    " << *inst << " | dsafe: " << dsafe << ", earliest: " << earliest << ", delay: " << delay << ", latest: " << latest << ", isolated: " << isolated << "\n");
  }

  std::set<Instruction*> OCP = getOCP(F, term);
  std::set<Instruction*> RO = getRO(F, term);

  DEBUG(dbgs() << "#OCP\n");
  for (auto I : OCP) {
    DEBUG(dbgs() << *I << "\n");
  }

  DEBUG(dbgs() << "\n#RO\n");
  for (auto I : RO) {
    DEBUG(dbgs() << *I << "\n");
  }

  // insert instruction that is both earliest and
  // and update term to load inst
  if (OCP.size() > 0 && RO.size() > 0) {
    Changed = true;
    Instruction *firstInst = &(F.front().front());
    Value* allocaInst = dyn_cast<Value>(new AllocaInst(term_type(term), Twine(), firstInst));  // alloca inst for term.

    ReversePostOrderTraversal<Function *> RPOT(&F);
    for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                             RE = RPOT.end();
         RI != RE; ++RI) {
      BasicBlock * bb = *RI;
      for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
        Instruction * inst = &*it;
        if (OCP.find(inst) != OCP.end()) {
          // insert instruction
          auto loadInst1 = (new LoadInst(term_operand1(term), Twine(), inst));
          auto loadInst2 = (new LoadInst(term_operand2(term), Twine(), inst));
          Value* binaryOperator = dyn_cast<Value>(BinaryOperator::Create((Instruction::BinaryOps)(term_opcode(term)), loadInst1, loadInst2, Twine(), inst));
          (void)dyn_cast<Value>(new StoreInst(binaryOperator, allocaInst, inst));
        }
        if (RO.find(inst) != RO.end()) {
          auto nextIt = ++it;
          LLVMContext & C = inst->getModule()->getContext();
          IRBuilder<> IRB(C);
          DEBUG(dbgs() << "@@ " << *allocaInst << "\n");
          auto loadInst = IRB.CreateLoad(allocaInst, Twine());
          DEBUG(dbgs() << "    replace to: " << *loadInst << "\n");

          ReplaceInstWithInst(inst, loadInst); // replace with load instruction.

          // erase all operands of inst
          unsigned numOperands = inst->getNumOperands();
          // DEBUG(dbgs() << "remove operands " << numOperands << "\n");
          for (unsigned i = 0; i < numOperands; i++) {
            Value *operandToBeRemoved = inst->getOperand(i);
            dyn_cast<Instruction>(operandToBeRemoved)->eraseFromParent();
            // DEBUG(dbgs() << *operandToBeRemoved << "\n");
          }

          // DEBUG(dbgs() << "    done replacing" << *loadInst << "\n");
          it = --nextIt;
        }
      }
    }
  }

  DEBUG(dbgs() << "\n#Done perform_OCP_RO_Transformation\n");
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    DEBUG(dbgs() << *inst << "\n");
  }

  return Changed;
}

bool PRE::runOnFunction(Function &F) {

  bool Changed = false;

  DEBUG(dbgs() << "#### PRE ####\n");

  // for test
  std::set<term_t> terms = getPartialRedundantExpressions(F);
  for (auto term : terms) {
    if(perform_OCP_RO_Transformation(F, term)) {
      Changed = true;
    }
  }

  /*
  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    BasicBlock * block = &*b;
    Instruction *inst = &*(--(--(block->end())));
    DEBUG(dbgs() << block->back() << "\n");
    DEBUG(dbgs() << *inst << "\n");
  }
  */

  DEBUG(dbgs() << "#### Done PRE ####\n");
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    DEBUG(dbgs() << *inst << "\n");
  }
  return Changed;
}
