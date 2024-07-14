#include "LiveVar.hpp"
#define IS_Global(val) dynamic_cast<GlobalVariable *>(val)
#define IS_Instr(val) dynamic_cast<Instruction *>(val)
#define IS_Arg(val) dynamic_cast<Argument *>(val)

void LiveVar::run()
{
    for (auto &f1 : m_->get_functions())
    {
        auto f = &f1;
        if (!f->is_declaration())
        {
            for (auto &bb1 : f->get_basic_blocks())
            {
                auto bb = &bb1;
                BB_def.insert({bb, {}});
                BB_use.insert({bb, {}});
                BB_IN.insert({bb, {}});
                BB_OUT.insert({bb, {}});
            }
            create_BB_def(f);
            create_BB_use(f);
            create_BB_IN_OUT(f);
        }
    }
}

void LiveVar::create_BB_def(Function *f)
{
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        for (auto &instr1 : bb->get_instructions())
        {
            Value *instr = &instr1;
            if (!static_cast<Instruction *>(instr)->is_void())
                BB_def[bb].insert(instr);
        }
    }
}

void LiveVar::create_BB_use(Function *f)
{
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        for (auto &instr1 : bb->get_instructions())
        {
            auto instr = &instr1;
            int num = instr->get_num_operand();
            for (int i = 0; i < num; i++)
            {
                Value *val = instr->get_operand(i);
                if (IS_Global(val) || IS_Instr(val) || IS_Arg(val))
                {
                    if (BB_def[bb].find(val) == BB_def[bb].end())
                    {
                        if (BB_use[bb].find(val) == BB_use[bb].end())
                            BB_use[bb].insert(val);
                    }
                }
            }
        }
    }
}

void LiveVar::create_BB_IN_OUT(Function *f)
{
    bool if_continue = true;
    while (if_continue)
    {
        if_continue = false;
        for (auto &bb1 : f->get_basic_blocks())
        {
            BasicBlock *bb = &bb1;
            std::set<Value *> new_bb_in;
            std::set<Value *> new_bb_out;
            for (auto &next_bb : bb->get_succ_basic_blocks())
            {
                for (auto &next_bb_in : BB_IN[next_bb])
                {
                    if (new_bb_out.find(next_bb_in) == new_bb_out.end())
                        new_bb_out.insert(next_bb_in);
                }
            }
            std::set<Value *> wait_to_remove;
            for (auto &bb_def : BB_def[bb])
            {
                if (new_bb_out.find(bb_def) != new_bb_out.end())
                    wait_to_remove.insert(bb_def);
            }
            new_bb_in = new_bb_out;
            for (auto &remove_bb : wait_to_remove)
            {
                new_bb_in.erase(remove_bb);
            }
            for (auto &use_bb : BB_use[bb])
            {
                if (new_bb_in.find(use_bb) == new_bb_in.end())
                    new_bb_in.insert(use_bb);
            }
            if (!(new_bb_in == BB_IN[bb] && new_bb_out == BB_OUT[bb]))
            {
                if_continue = true;
                BB_IN[bb] = new_bb_in;
                BB_OUT[bb] = new_bb_out;
            }
        }
    }
}