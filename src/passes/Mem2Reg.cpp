#include "Mem2Reg.hpp"
#include "IRBuilder.hpp"
#include "Value.hpp"
#include <memory>

#define IS_GLOBAL_VARIABLE(val) dynamic_cast<GlobalVariable *>(val) // 判断变量是否为全局变量

#define IS_GEP_INSTR(val) dynamic_cast<GetElementPtrInst *>(val) // 判断变量是否为指针

std::map<Value *, std::vector<Value *>> val_stack; // 全局变量栈图

std::map<Instruction *, Value *> phi_val_map; // phi指令对应的val值

std::set<BasicBlock *> bb_renamed; // 记录renamed过的bb

void Mem2Reg::run()
{
    // 创建支配树分析 Pass 的实例
    dominators_ = std::make_unique<Dominators>(m_);
    // 建立支配树
    dominators_->run();
    // 以函数为单元遍历实现 Mem2Reg 算法
    for (auto &f : m_->get_functions())
    {
        if (f.is_declaration())
            continue;
        func_ = &f;
        if (func_->get_basic_blocks().size() > 1)
        {
            // phi 指令插入的阶段
            generate_phi();
        }
        // 重命名
        rename(func_->get_entry_block());
        // 后续 DeadCode 将移除冗余的局部变量的分配空间以及load指令
    }
}

void Mem2Reg::generate_phi()
{
    // 找到活跃在多个 bb的全局名字集合，以及它们所属的 bb 块
    std::set<Value *> global_live_name;
    std::map<Value *, std::set<BasicBlock *>> global_live_name_bb;
    for (auto &bb1 : func_->get_basic_blocks())
    {
        auto bb = &bb1;
        for (auto &instr1 : bb->get_instructions())
        {
            auto instr = &instr1;
            if (instr->is_store())
            {
                auto store_rval = instr->get_operand(1);
                if (!IS_GLOBAL_VARIABLE(store_rval) && !IS_GEP_INSTR(store_rval))
                {
                    if (global_live_name.find(store_rval) == global_live_name.end())
                    {
                        global_live_name.insert(store_rval);
                        std::set<BasicBlock *> store_rval_set;
                        global_live_name_bb.insert({store_rval, store_rval_set});
                    }
                    if (global_live_name_bb.find(store_rval) != global_live_name_bb.end())
                    {
                        if (global_live_name_bb[store_rval].find(bb) == global_live_name_bb[store_rval].end())
                        {
                            global_live_name_bb[store_rval].insert(bb);
                        }
                    }
                }
            }
        }
    }
    std::vector<Value *> global_live_name_to_delete;

    for (auto &val : global_live_name)
    {
        if (global_live_name_bb.find(val) != global_live_name_bb.end())
        {
            if (global_live_name_bb[val].size() < 2)
            {
                global_live_name_to_delete.push_back(val);
            }
        }
    }
    for (auto &val : global_live_name_to_delete)
    {
        global_live_name.erase(val);
        global_live_name_bb.erase(val);
    }

    // 通过支配边界信息，在对应位置插入phi指令
    std::map<std::pair<BasicBlock *, Value *>, bool> bb_has_val_phi;
    std::vector<BasicBlock *> work_list;
    for (auto &val : global_live_name)
    {
        work_list.assign(global_live_name_bb[val].begin(), global_live_name_bb[val].end());
        int len = work_list.size();
        for (int i = 0; i < len; i++)
        {
            auto bb = work_list[i];
            if (dominators_->get_dominance_frontier(bb).size() != 0)
            {
                for (auto bb_dominance_frontier : dominators_->get_dominance_frontier(bb))
                {
                    if (bb_has_val_phi.find(std::make_pair(bb_dominance_frontier, val)) == bb_has_val_phi.end())
                    {
                        std::vector<Value *> vals;
                        std::vector<BasicBlock *> vals_bbs;
                        auto phi1 = PhiInst::create_phi(val->get_type()->get_pointer_element_type(), bb_dominance_frontier, vals, vals_bbs);
                        auto phi = static_cast<Instruction *>(phi1);
                        bb_dominance_frontier->add_instr_begin(phi);
                        bb_has_val_phi.insert(std::make_pair(std::make_pair(bb_dominance_frontier, val), true));
                        bool if_bb_dominance_frontier = true;
                        for (int j = 0; j < len; j++)
                        {
                            if (work_list[j] == bb_dominance_frontier)
                            {
                                if_bb_dominance_frontier = false;
                                break;
                            }
                        }
                        if (if_bb_dominance_frontier)
                        {
                            work_list.push_back(bb_dominance_frontier);
                            len++;
                        }
                        phi_val_map.insert({phi, val});
                    }
                }
            }
        }
        work_list.clear();
    }
}

void Mem2Reg::rename(BasicBlock *bb)
{
    std::vector<Instruction *> wait_delete;

    // 用phi指令值替换进入栈图中对应的val栈
    for (auto &instr1 : bb->get_instructions())
    {
        auto instr = &instr1;
        if (instr->is_phi())
        {
            if (phi_val_map.find(instr) != phi_val_map.end())
            {
                auto phi_val = phi_val_map[instr];
                val_stack[phi_val].push_back(instr);
            }
        }
    }

    for (auto &instr1 : bb->get_instructions())
    {
        auto instr = &instr1;
        // 将load的值替换
        if (instr->is_load())
        {
            auto load_val = static_cast<LoadInst *>(instr)->get_operand(0);
            if (!IS_GLOBAL_VARIABLE(load_val) && !IS_GEP_INSTR(load_val))
            {
                if (val_stack.find(load_val) != val_stack.end())
                {
                    if (val_stack[load_val].empty())
                        assert(false && "load_val be empty");
                    else
                    {
                        instr->replace_all_use_with(val_stack[load_val].back());
                    }
                }
            }
        }
        // 将store值入栈，同时删除store指令
        if (instr->is_store())
        {
            auto store_lval = static_cast<StoreInst *>(instr)->get_operand(0);
            auto store_rval = static_cast<StoreInst *>(instr)->get_operand(1);
            if (!IS_GLOBAL_VARIABLE(store_rval) && !IS_GEP_INSTR(store_rval))
            {
                if (val_stack.find(store_rval) == val_stack.end())
                {
                    std::vector<Value *> store_rval_value;
                    val_stack.insert({store_rval, store_rval_value});
                }
                val_stack[store_rval].push_back(store_lval);
                wait_delete.push_back(instr);
            }
        }
    }

    // 补全后续bb的phi指令
    for (auto succ_bb : bb->get_succ_basic_blocks())
    {
        for (auto &instr1 : succ_bb->get_instructions())
        {
            auto instr = &instr1;
            if (instr->is_phi())
            {
                auto phi_instr = static_cast<PhiInst *>(instr);
                if (phi_val_map.find(instr) != phi_val_map.end())
                {
                    auto phi_val = phi_val_map[instr];
                    if (val_stack.find(phi_val) != val_stack.end() && !val_stack[phi_val].empty())
                        phi_instr->add_phi_pair_operand(val_stack[phi_val].back(), bb);
                }
            }
        }
    }

    // 对后续bb块递归进行重命名。
    bb_renamed.insert(bb);

    for (auto succ_bbs : bb->get_succ_basic_blocks())
    {
        if (bb_renamed.find(succ_bbs) == bb_renamed.end())
            rename(succ_bbs);
    }

    // 结束当前bb块重命名，弹出栈值。
    for (auto &instr1 : bb->get_instructions())
    {
        auto instr = &instr1;
        if (instr->is_store())
        {
            auto store_rval = static_cast<StoreInst *>(instr)->get_operand(1);
            if (!IS_GLOBAL_VARIABLE(store_rval) && !IS_GEP_INSTR(store_rval))
            {
                if (val_stack.find(store_rval) != val_stack.end())
                {
                    if (!val_stack[store_rval].empty())
                        val_stack[store_rval].pop_back();
                    else
                        assert(false && "val_stack push error");
                }
            }
        }
        else if (instr->is_phi())
        {
            if (phi_val_map.find(instr) != phi_val_map.end())
            {
                auto phi_val = phi_val_map[instr];
                if (val_stack.find(phi_val) != val_stack.end() && !val_stack[phi_val].empty())
                    val_stack[phi_val].pop_back();
                else
                    assert(false && "val_stack phi error");
            }
        }
    }

    // 删除store指令
    for (auto &instr : wait_delete)
    {
        bb->erase_instr(instr);
    }
    return;
}
