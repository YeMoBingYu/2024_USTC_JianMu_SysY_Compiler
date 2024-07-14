#pragma once
#include "PassManager.hpp"
#include "FindLoop.hpp"
#include "Function.hpp"

class LoopHoist : public Pass
{
public:
    LoopHoist(Module *m) : Pass(m) {}
    void LoopExtract(Function *);
    void Extract(LoopTreeNode *);
    void CopyInstr(BasicBlock *, Instruction *);
    void run();

private:
    std::unique_ptr<FindLoop> findloop_;
};