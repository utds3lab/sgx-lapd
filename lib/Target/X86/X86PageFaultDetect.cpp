#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "X86.h"
#include "X86InstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
using namespace llvm;
//#define EVALUATION
namespace {
  class PageFaultDetect : public MachineFunctionPass {
      static char ID;
      public:
        PageFaultDetect() : MachineFunctionPass(ID) { }
        bool runOnMachineFunction(MachineFunction &MF) override;
        const char *getPassName() const override { return "Insert code to detect page fault"; }
    };
}

FunctionPass *llvm::createX86PageFaultDetectPass() { return new PageFaultDetect(); }

char PageFaultDetect::ID = 0;
bool PageFaultDetect::runOnMachineFunction(MachineFunction &MF) 
{

  const TargetInstrInfo *TII = MF.getSubtarget<X86Subtarget>().getInstrInfo();
  for (auto MBB = MF.begin(); MBB != MF.end(); MBB++) {
        for (auto MI = MBB->begin(); MI != MBB->end(); MI++) {
            if(MI->isCall() || MI->isBranch() || MI->isReturn())
            {

                MI->setLabel();
                MachineBasicBlock::iterator  patch=BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::PUSHF64));
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::PUSH64r)).addReg(X86::RAX);
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::PUSH64r)).addReg(X86::RSI);
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::PUSH64r)).addReg(X86::RDX);
                
#ifndef EVALUATION
                MachineBasicBlock::iterator MI2 = BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::CALL64pcrel32)).addExternalSymbol("abort");
#else
                MachineBasicBlock::iterator MI2 = BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::NOOP));
#endif
                MachineBasicBlock::iterator MI_return = BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::POP64r)).addReg(X86::RDX);
                MI_return->setLabel();
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::POP64r)).addReg(X86::RSI);
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::POP64r)).addReg(X86::RAX);
                BuildMI(*MBB, MI, MI->getDebugLoc(), TII->get(X86::POPF64));
 

                switch(MI->getOpcode()){
                    case X86::RETQ:
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RSI).addReg(X86::RSP).addImm(1).addReg(0).addImm(32).addReg(0);
                        break;
                    case X86::CALL64pcrel32:
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::LEA64r)).addReg(X86::RSI).addReg(X86::RIP).addImm(1).addReg(0).addOperand(MI->getOperand(0)).addReg(0);
                        patch->setPatch();
                        patch->setLabel();
                        MI->setConFlow();
                        break;
                    case X86::CALL64r:
                    case X86::JMP64r:
                    case X86::TAILJMPr64:
                        {
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rr)).addReg(X86::RSI).addOperand(MI->getOperand(0));
#ifdef EVALUATION
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rr)).addReg(X86::RAX).addReg(X86::RSI);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::LEA64r)).addReg(X86::RDX).addReg(X86::RIP).addImm(1).addReg(0).addSym(MI->getSymbol()).addReg(0);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RDX).addReg(X86::RDX, RegState::Define).addImm(0xfffffffffffff000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RAX).addReg(X86::RAX, RegState::Define).addImm(0xfffffffffffff000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::CMP64rr)).addReg(X86::RAX).addReg(X86::RDX);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::JE_1)).addSym(MI_return->getSymbol());
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RDX).addReg(X86::RDX, RegState::Define).addImm(0xffffffffffe00000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RAX).addReg(X86::RAX, RegState::Define).addImm(0xffffffffffe00000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::CMP64rr)).addReg(X86::RAX).addReg(X86::RDX);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::JNE_1)).addSym(MI_return->getSymbol());
#endif
                        }
                        break;
                    case X86::CALL64m:
                    case X86::JMP64m:
                    case X86::TAILJMPm64:
                        {
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RSI).addOperand(MI->getOperand(0)).addOperand(MI->getOperand(1)).addOperand(MI->getOperand(2)).addOperand(MI->getOperand(3)).addOperand(MI->getOperand(4));
#ifdef EVALUATION
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rr)).addReg(X86::RAX).addReg(X86::RSI);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::LEA64r)).addReg(X86::RDX).addReg(X86::RIP).addImm(1).addReg(0).addSym(MI->getSymbol()).addReg(0);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RDX).addReg(X86::RDX, RegState::Define).addImm(0xfffffffffffff000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RAX).addReg(X86::RAX, RegState::Define).addImm(0xfffffffffffff000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::CMP64rr)).addReg(X86::RAX).addReg(X86::RDX);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::JE_1)).addSym(MI_return->getSymbol());
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RDX).addReg(X86::RDX, RegState::Define).addImm(0xffffffffffe00000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::AND64ri32)).addReg(X86::RAX).addReg(X86::RAX, RegState::Define).addImm(0xffffffffffe00000);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::CMP64rr)).addReg(X86::RAX).addReg(X86::RDX);
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::JNE_1)).addSym(MI_return->getSymbol());
#endif  
                        }
                        break;
                    case X86::CALL32m:
                    case X86::JMP32m:
                    case X86::CALL32r:
                    case X86::JMP32r:
                        dbgs() <<"@@ERROR not support" <<'\n';
                        break;
                    default:
                        BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::LEA64r)).addReg(X86::RSI).addReg(X86::RIP).addImm(1).addReg(0).addOperand(MI->getOperand(0)).addReg(0);
                        patch->setPatch();
                        patch->setLabel();
                        MI->setConFlow();
                        break;
                }
                // get SSA page
#ifndef EVALUATION
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RAX).addReg(0).addImm(1).addReg(0).addImm(0x20).addReg(X86::GS);
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV8mi)).addReg(X86::RAX).addImm(1).addReg(0).addImm(0xa0).addReg(0).addImm(0x0);
#else

                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RAX).addReg(X86::RSP).addImm(1).addReg(0).addImm(0).addReg(0);
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RAX).addReg(X86::RSP).addImm(1).addReg(0).addImm(0).addReg(0);
                //BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV8mi)).addReg(X86::RAX).addImm(1).addReg(0).addImm(0xa0).addReg(0).addImm(0x0);
#endif

                //Triger page fault
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV8rm)).addReg(X86::RDX).addReg(X86::RSI).addImm(1).addReg(0).addImm(0).addReg(0);
                 
                // get exception vector
#ifndef EVALUATION
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV8rm)).addReg(X86::RDX).addReg(X86::RAX).addImm(1).addReg(0).addImm(0xa0).addReg(0);
#else
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::MOV64rm)).addReg(X86::RAX).addReg(X86::RSP).addImm(1).addReg(0).addImm(0).addReg(0);
#endif

                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::CMP8ri)).addReg(X86::RDX).addImm(0xe);
                BuildMI(*MBB, MI2, MI->getDebugLoc(), TII->get(X86::JNE_1)).addSym(MI_return->getSymbol());
                
            }
        }
  }

    return true;
}

