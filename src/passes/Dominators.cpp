#include "Dominators.hpp"
#include <queue>
#include <cstring>
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void Dominators::run()
{
    for (auto &f1 : m_->get_functions())
    {
        auto f = &f1;
        if (f->get_num_basic_blocks() < 2)
            continue;
        create_dom(f);
        create_idom(f);
        create_dominance_frontier(f);
    }
}

void Dominators::create_dom(Function *f)
{
    // 初始化
    std::set<BasicBlock *> all_bb;
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        all_bb.insert(bb);
    }
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        if (_STR_EQ(bb->get_name().c_str(), "label_entry"))
        {
            std::set<BasicBlock *> bb_set;
            bb_set.insert(bb);
            dom_.insert({bb, bb_set});
        }
        else
        {
            dom_.insert({bb, all_bb});
        }
    }
    // 迭代求解 Dom
    bool if_continue = false;
    do
    {
        if_continue = false;
        for (auto &bb1 : f->get_basic_blocks())
        {
            auto bb = &bb1;
            auto &pre_bb_list = bb->get_pre_basic_blocks();
            std::set<BasicBlock *> new_set;
            if (!pre_bb_list.empty())
            {
                auto first_bb = pre_bb_list.front();
                for (auto &first_bb_dom_bb : dom_[first_bb])
                {
                    bool if_real = true;
                    for (auto &other_bb : pre_bb_list)
                    {
                        if (other_bb != first_bb && dom_[other_bb].find(first_bb_dom_bb) == dom_[other_bb].end())
                            if_real = false;
                    }
                    if (if_real)
                        new_set.insert(first_bb_dom_bb);
                }
            }
            if (new_set.find(bb) == new_set.end())
                new_set.insert(bb);
            bool if_need_change = false;
            if (dom_[bb].size() != new_set.size())
                if_need_change = true;
            else
            {
                for (auto &old_dom_bb : dom_[bb])
                    if (new_set.find(old_dom_bb) == new_set.end())
                        if_need_change = true;
            }
            if (if_need_change)
            {
                dom_[bb] = new_set;
                if_continue = true;
            }
        }
    } while (if_continue);
}

void Dominators::create_idom(Function *f)
{
    // 初始化
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        BasicBlock *idom_bb = nullptr;
        idom_.insert({bb, idom_bb});
    }
    // 迭代计算
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        std::queue<BasicBlock *> q;
        std::set<BasicBlock *> visit;
        for (auto &pre_bb : bb->get_pre_basic_blocks())
            q.push(pre_bb);
        visit.insert(bb);
        while (!q.empty())
        {
            BasicBlock *first_bb = q.front();
            if (dom_[bb].find(first_bb) != dom_[bb].end())
            {
                idom_[bb] = first_bb;
                break;
            }
            for (auto &first_bb_pre_bb : first_bb->get_pre_basic_blocks())
            {
                if (visit.find(first_bb_pre_bb) == visit.end())
                {
                    visit.insert(first_bb_pre_bb);
                    q.push(first_bb_pre_bb);
                }
            }
            q.pop();
        }
    }
}

void Dominators::create_dominance_frontier(Function *f)
{
    // 初始化
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        std::set<BasicBlock *> bb_set;
        dom_frontier_.insert({bb, bb_set});
    }
    // 迭代求解支配边界
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        auto succ_bb_list = bb->get_succ_basic_blocks();
        for (auto &succ_bb : succ_bb_list)
        {
            auto x = bb;
            while (x != nullptr)
            {
                if (dom_[succ_bb].find(x) != dom_[succ_bb].end())
                    break;
                else
                {
                    dom_frontier_[x].insert(succ_bb);
                    x = idom_[x];
                }
            }
        }
    }
}
