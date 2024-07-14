#pragma once
#include "PassManager.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include <set>
#include <map>

struct LoopTreeNode
{
    std::set<BasicBlock *> Loop;
    BasicBlock *entry = nullptr;
    std::set<std::shared_ptr<LoopTreeNode>> inner_loop;
};

class FindLoop : public Pass
{
public:
    FindLoop(Module *m) : Pass(m) {}
    ~FindLoop() = default;
    void GetLoop(Function *);
    void MergeNode(Function *);
    LoopTreeNode *MinNumNode(std::set<LoopTreeNode *> &);
    BasicBlock *get_loop_entry(LoopTreeNode *);
    LoopTreeNode *get_entry_loop(BasicBlock *);
    std::set<std::shared_ptr<LoopTreeNode>> &get_function_loop(Function *f) { return func_loop_[f]; };
    void run();

private:
    std::map<Function *, std::set<std::shared_ptr<LoopTreeNode>>> func_loop_; // 每个循环的循环森林
    std::map<LoopTreeNode *, BasicBlock *> loop_entry_;                       // 每个循环的入口节点
    std::map<BasicBlock *, LoopTreeNode *> entry_loop_;                       // 每个入口节点对应的循环
    std::unique_ptr<Dominators> dominators_;
};

