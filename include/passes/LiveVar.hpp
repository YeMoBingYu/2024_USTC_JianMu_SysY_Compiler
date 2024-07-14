#pragma once
#include "PassManager.hpp"
#include "Function.hpp"
#include <map>
#include <set>
class LiveVar : public Pass
{
public:
    LiveVar(Module *m) : Pass(m){};
    ~LiveVar() = default;
    void create_BB_use(Function *);
    void create_BB_def(Function *);
    void create_BB_IN_OUT(Function *);
    const std::set<Value *> &get_bb_in(BasicBlock *bb) { return BB_IN[bb]; };
    const std::set<Value *> &get_bb_out(BasicBlock *bb) { return BB_OUT[bb]; };
    void run();

private:
    std::map<BasicBlock *, std::set<Value *>> BB_use;
    std::map<BasicBlock *, std::set<Value *>> BB_def;
    std::map<BasicBlock *, std::set<Value *>> BB_IN;
    std::map<BasicBlock *, std::set<Value *>> BB_OUT;
};
