//===- PRE.cpp - Partial Redundancy Elimination via Lazy Code Motion --------===//

#define DEBUG_TYPE "pre"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
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

STATISTIC(NumInstInserted, "Number of instructions inserted for PRE via Lazy Code Motion");
STATISTIC(NumInstReplaced, "Number of instructions replaced for PRE via Lazy Code Motion");

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

    std::map<Instruction*, bool> mem_dsafe;
    std::map<Instruction*, bool> mem_earliest;
    std::map<Instruction*, bool> mem_delay;
    std::map<Instruction*, bool> mem_latest;
    std::map<Instruction*, bool> mem_isolated;
    Instruction* startNode;
    Instruction* endNode;

    std::set<term_t> getPartiallyRedundantExpressions(Function &F);
    Value* getAlloca(Value* val);
    Instruction* getStartNode(Function &F);
    Instruction* getEndNode(Function &F);
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

std::set<term_t> PRE::getPartiallyRedundantExpressions(Function &F) {
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

Instruction* PRE::getStartNode(Function &F) {
  ReversePostOrderTraversal<Function *> RPOT(&F);
  BasicBlock *bb = *(RPOT.begin());
  Instruction *inst = &*(bb->begin());
  return inst;
}

Instruction* PRE::getEndNode(Function &F) {
  BasicBlock *bb = *(po_begin(&F.getEntryBlock()));
  Instruction *inst = &*(bb->rbegin());
  return inst;
}

Value* PRE::getAlloca(Value* val) {
  LoadInst* loadInst = dyn_cast<LoadInst>(val);
  Constant* constInst = dyn_cast<Constant>(val);
  if (loadInst) {
    return dyn_cast<Value>(loadInst->getOperand(0));
  } else if (constInst) {
    return val;
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

// return changed or not.
bool PRE::DSafe(Instruction &inst, term_t term) {
  // DEBUG(dbgs() << "DSafe: " << inst << "\n");
  bool dsafe = false;
  if (endNode == &inst) {  // if n == e
    dsafe = false;
  } else if (Used(inst, term)) {
    dsafe = true;
  } else if (Transp(inst, term)) {
    dsafe = true;
    std::set<Instruction*> successors = getSuccessors(&inst);
    for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
      Instruction *m = *I;
      if (mem_dsafe.find(m) == mem_dsafe.end()) continue; // instruction not calculated yet.
      if (!mem_dsafe[m]) {
        dsafe = false;
        break;
      }
    }
  } else {
    dsafe = false;
  }

  if (mem_dsafe.find(&inst) == mem_dsafe.end() ||
      mem_dsafe[&inst] != dsafe) {
    mem_dsafe[&inst] = dsafe;
    return true;
  } else {
    return false;
  }
}

bool PRE::Earliest(Instruction &inst, term_t term) {
  bool earliest = false;
  if (startNode == &inst) { // if n == s
    earliest = true;
  } else {
    std::set<Instruction*> predecessors = getPredecessors(&inst);
    earliest = false;
    for (auto I = predecessors.begin(), E = predecessors.end(); I != E; ++I) {
      Instruction *m = *I;
      if (mem_earliest.find(m) == mem_earliest.end()) continue;
      if (!Transp(*m, term)) {
        earliest = true;
        break;
      } else if (!mem_dsafe[m] && mem_earliest[m]) {
        earliest = true;
        break;
      }
    }
  }

  if (mem_earliest.find(&inst) == mem_earliest.end() ||
      mem_earliest[&inst] != earliest) {
    mem_earliest[&inst] = earliest;
    return true;
  } else {
    return false;
  }
}

bool PRE::Delay(Instruction &inst, term_t term) {
  bool delay = false;
  if (mem_dsafe[&inst] && mem_earliest[&inst]) {
    delay = true;
  } else {
    if (startNode == &inst) { // if n == s
      delay = false;
    } else {
      delay = true;
      std::set<Instruction*> predecessors = getPredecessors(&inst);
      for (auto I = predecessors.begin(), E = predecessors.end(); I != E; ++I) {
        Instruction *m = *I;
        if (mem_delay.find(m) == mem_delay.end()) continue;
        if (!Used(*m, term) && mem_delay[m]) continue;

        delay = false;
        break;
      }
    }
  }

  if (mem_delay.find(&inst) == mem_delay.end() ||
      mem_delay[&inst] != delay) {
    mem_delay[&inst] = delay;
    return true;
  } else {
    return false;
  }
}

bool PRE::Latest(Instruction &inst, term_t term) {
  bool latest = true;
  if (!mem_delay[&inst]) {
    latest = false;
  } else if (Used(inst, term)) {
    latest = true;
  } else {
    bool flag = true;
    std::set<Instruction*> successors = getSuccessors(&inst);
    for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
      Instruction *m = *I;
      if (mem_latest.find(m) == mem_latest.end()) continue;
      if (mem_delay[m]) continue;

      flag = false;
      break;
    }
    latest = !flag;
  }

  if (mem_latest.find(&inst) == mem_latest.end() ||
      mem_latest[&inst] != latest) {
    mem_latest[&inst] = latest;
    return true;
  } else {
    return false;
  }
}

bool PRE::Isolated(Instruction &inst, term_t term) {
  bool isolated = true;
  std::set<Instruction*> successors = getSuccessors(&inst);
  for (auto I = successors.begin(), E = successors.end(); I != E; ++I) {
    Instruction *m = *I;
    if (mem_isolated.find(m) == mem_isolated.end()) continue;
    if (mem_latest[m] ||
       (!Used(*m, term) &&
        mem_isolated[m])
    ) continue;

    isolated = false;
    break;
  }

  if (mem_isolated.find(&inst) == mem_isolated.end() ||
      mem_isolated[&inst] != isolated) {
    mem_isolated[&inst] = isolated;
    return true;
  } else {
    return false;
  }
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
  bool changed = true;
  while (changed) {
    changed = false;
    for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                  IE = po_end(&F.getEntryBlock());
                                 I != IE; ++I) {
      BasicBlock * bb = *I;
      for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
        Instruction * inst = &*it;
        changed = DSafe(*inst, term) || changed;
      }
    }
  }
}

// forwards
void PRE::getEarliests(Function &F, term_t term) {
  mem_earliest.clear();
  bool changed = true;
  while (changed) {
    changed = false;
    ReversePostOrderTraversal<Function *> RPOT(&F);
    for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                             RE = RPOT.end();
         RI != RE; ++RI) {
      BasicBlock * bb = *RI;
      for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
        Instruction * inst = &*it;
        changed = Earliest(*inst, term) || changed;
      }
    }
  }
}

void PRE::getDelays(Function &F, term_t term) {
  mem_delay.clear();
  bool changed = true;
  while (changed) {
    changed = false;
    ReversePostOrderTraversal<Function *> RPOT(&F);
    for (ReversePostOrderTraversal<Function *>::rpo_iterator RI = RPOT.begin(),
                                                             RE = RPOT.end();
         RI != RE; ++RI) {
      BasicBlock * bb = *RI;
      for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
        Instruction * inst = &*it;
        changed = Delay(*inst, term) || changed;
      }
    }
  }
}

void PRE::getLatests(Function &F, term_t term) {
  mem_latest.clear();
  bool changed = true;
  while (changed) {
    changed = false;
    for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                  IE = po_end(&F.getEntryBlock());
                                 I != IE; ++I) {
      BasicBlock * bb = *I;
      for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
        Instruction * inst = &*it;
        changed = Latest(*inst, term) || changed;
      }
    }
  }
}

void PRE::getIsolateds(Function &F, term_t term) {
  mem_isolated.clear();
  bool changed = true;
  while (changed) {
    changed = false;
    for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                                  IE = po_end(&F.getEntryBlock());
                                 I != IE; ++I) {
      BasicBlock * bb = *I;
      for (auto it = bb->rbegin(), ite = bb->rend(); it != ite; ++it) {
        Instruction * inst = &*it;
        changed = Isolated(*inst, term) || changed;
      }
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
  startNode = getStartNode(F);
  endNode = getEndNode(F);
  DEBUG(dbgs() << "end node: " << *endNode << "\n");
  DEBUG(dbgs() << "Begin getDSafes\n");
  getDSafes(F, term);
  DEBUG(dbgs() << "Begin getEarliests\n");
  getEarliests(F, term);
  DEBUG(dbgs() << "Begin getDelays\n");
  getDelays(F, term);
  DEBUG(dbgs() << "Begin getLatests\n");
  getLatests(F, term);
  DEBUG(dbgs() << "Begin getIsolateds\n");
  getIsolateds(F, term);
  DEBUG(dbgs() << "Done gettings sets\n");

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
          Value* loadInst1 = term_operand1(term);
          Value* loadInst2 = term_operand2(term);
          if (isa<AllocaInst>(loadInst1)) {
            loadInst1 = dyn_cast<Value>(new LoadInst(loadInst1, Twine(), inst));
          }
          if (isa<AllocaInst>(loadInst2)) {
            loadInst2 = dyn_cast<Value>(new LoadInst(loadInst2, Twine(), inst));
          }
          DEBUG(dbgs() << "#insert h\n");
          DEBUG(dbgs() << *loadInst1 << "\n");
          DEBUG(dbgs() << *loadInst2 << "\n");

          Value* binaryOperator = dyn_cast<Value>(BinaryOperator::Create((Instruction::BinaryOps)(term_opcode(term)), loadInst1, loadInst2, Twine(), inst));
          (void)dyn_cast<Value>(new StoreInst(binaryOperator, allocaInst, inst));
          NumInstInserted++;
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
            if (!isa<Constant>(operandToBeRemoved)) {
              dyn_cast<Instruction>(operandToBeRemoved)->eraseFromParent();
            }
            // DEBUG(dbgs() << *operandToBeRemoved << "\n");
          }

          // DEBUG(dbgs() << "    done replacing" << *loadInst << "\n");
          it = --nextIt;
          NumInstReplaced++;
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
  std::set<term_t> terms = getPartiallyRedundantExpressions(F);
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
