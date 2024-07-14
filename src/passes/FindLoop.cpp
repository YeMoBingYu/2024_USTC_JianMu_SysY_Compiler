#include "FindLoop.hpp"
#include <set>
#include <vector>
#include <queue>
#define MAX 2e30

std::set<LoopTreeNode *> bb_loop_set;
BasicBlock *wrong_bb;

void FindLoop::run()
{
    dominators_ = std::make_unique<Dominators>(m_);
    dominators_->run();
    for (auto &f1 : m_->get_functions())
    {
        auto f = &f1;
        if (!f->is_declaration())
        {
            std::set<std::shared_ptr<LoopTreeNode>> LoopWood;
            func_loop_.insert({f, LoopWood});
            GetLoop(f);
        }
    }
}

void FindLoop::GetLoop(Function *f)
{
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        auto dom_bb = dominators_->get_dom(bb);
        for (auto &succ_bb : bb->get_succ_basic_blocks())
        {
            if (dom_bb.find(succ_bb) != dom_bb.end())
            {
                std::queue<BasicBlock *> q;
                std::set<BasicBlock *> loop;
                std::set<BasicBlock *> visit;
                q.push(bb);
                while (!q.empty())
                {
                    auto first_bb = q.front();
                    for (auto &pre_bb : first_bb->get_pre_basic_blocks())
                    {
                        if (pre_bb != succ_bb && visit.find(pre_bb) == visit.end())
                        {
                            q.push(pre_bb);
                            visit.insert(pre_bb);
                        }
                    }
                    loop.insert(first_bb);
                    q.pop();
                }
                loop.insert(succ_bb);
                auto node = new LoopTreeNode();
                node->entry = succ_bb;
                node->Loop = loop;
                bb_loop_set.insert(node);
            }
        }
    }
    MergeNode(f);
    bb_loop_set.clear();
}

void FindLoop::MergeNode(Function *f)
{
    std::set<LoopTreeNode *> loop_set = bb_loop_set;
    for (auto &loop_node : bb_loop_set)
    {
        LoopTreeNode *min_node = MinNumNode(loop_set);
        std::set<LoopTreeNode *> min_node_outer;
        for (auto &loop_node : bb_loop_set)
        {
            if (loop_node != min_node)
            {
                int num = 0;
                for (auto &bb : min_node->Loop)
                {
                    if (loop_node->Loop.find(bb) != loop_node->Loop.end())
                        num++;
                }
                if (num == min_node->Loop.size())
                    min_node_outer.insert(loop_node);
            }
        }
        if (min_node_outer.empty())
        {
            loop_entry_.insert({min_node, min_node->entry});
            if (entry_loop_.find(min_node->entry) == entry_loop_.end())
                entry_loop_.insert({min_node->entry, min_node});
            else
                entry_loop_[min_node->entry] = min_node;
            func_loop_[f].insert(std::shared_ptr<LoopTreeNode>(min_node));
        }
        else
        {
            LoopTreeNode *outer_node = MinNumNode(min_node_outer);
            outer_node->inner_loop.insert(std::shared_ptr<LoopTreeNode>(min_node));
            loop_entry_.insert({min_node, min_node->entry});
            entry_loop_.insert({min_node->entry,min_node});
        }
        loop_set.erase(min_node);
    }
}

LoopTreeNode *FindLoop::MinNumNode(std::set<LoopTreeNode *> &loop_set)
{
    int min = MAX;
    LoopTreeNode *node;
    for (auto &loop_node : loop_set)
    {
        if (loop_node->Loop.size() < min)
        {
            node = loop_node;
            min = node->Loop.size();
        }
    }
    return node;
}

BasicBlock *FindLoop::get_loop_entry(LoopTreeNode *loop)
{
    if (loop_entry_.find(loop) == loop_entry_.end())
        return nullptr;
    else
        return loop_entry_[loop];
    return nullptr;
}

LoopTreeNode *FindLoop::get_entry_loop(BasicBlock *bb)
{
    if (bb == nullptr && entry_loop_.find(bb) == entry_loop_.end())
        assert(false && "entry : cannot find this loop");
    else
        return entry_loop_[bb];
}