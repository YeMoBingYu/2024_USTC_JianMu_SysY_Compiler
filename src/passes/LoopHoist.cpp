#include "LoopHoist.hpp"
#include <map>
#include <set>

#define IS_Arg(val) dynamic_cast<Argument *>(val)
#define IS_Const(val) dynamic_cast<Constant *>(val)
#define IS_Instr(val) dynamic_cast<Instruction *>(val)

std::set<Instruction *> unchange;

void LoopHoist::run()
{
    findloop_ = std::make_unique<FindLoop>(m_);
    findloop_->run();
    for (auto &f1 : m_->get_functions())
    {
        auto f = &f1;
        if (!f->is_declaration())
            LoopExtract(f);
    }
}

void LoopHoist::CopyInstr(BasicBlock *pre_BB, Instruction *instr)
{
    Instruction *new_instr;
    if (instr->is_alloca())
    {
        new_instr = AllocaInst::create_alloca(instr->get_type()->get_pointer_element_type(), pre_BB);
    }
    else if (instr->is_gep())
    {
        if (instr->get_num_operand() == 2)
        {
            new_instr = GetElementPtrInst::create_gep(instr->get_operand(0), {instr->get_operand(1)}, pre_BB);
        }
        else if (instr->get_num_operand() == 3)
        {
            new_instr = GetElementPtrInst::create_gep(instr->get_operand(0), {instr->get_operand(1), instr->get_operand(2)}, pre_BB);
        }
    }
    else if (instr->isBinary())
    {
        if (instr->is_add())
        {
            new_instr = IBinaryInst::create_add(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_sub())
        {
            new_instr = IBinaryInst::create_sub(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_mul())
        {
            new_instr = IBinaryInst::create_mul(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_div())
        {
            new_instr = IBinaryInst::create_sdiv(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_srem())
        {
            new_instr = IBinaryInst::create_srem(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_fadd())
        {
            new_instr = FBinaryInst::create_fadd(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_fsub())
        {
            new_instr = FBinaryInst::create_fsub(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_fmul())
        {
            new_instr = FBinaryInst::create_fmul(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
        else if (instr->is_fdiv())
        {
            new_instr = FBinaryInst::create_fdiv(instr->get_operand(0), instr->get_operand(1), pre_BB);
        }
    }
    else if (instr->is_fp2si() || instr->is_si2fp() || instr->is_zext())
    {
        if (instr->is_fp2si())
        {
            new_instr = FpToSiInst::create_fptosi(instr->get_operand(0), instr->get_type(), pre_BB);
        }
        else if (instr->is_si2fp())
        {
            new_instr = SiToFpInst::create_sitofp(instr->get_operand(0), pre_BB);
        }
        else if (instr->is_zext())
        {
            new_instr = ZextInst::create_zext(instr->get_operand(0), instr->get_type(), pre_BB);
        }
    }
    instr->replace_all_use_with(new_instr);
}

void LoopHoist::Extract(LoopTreeNode *node)
{
    if (node->inner_loop.empty())
    {
        for (auto &loop_bb : node->Loop)
        {
            if (loop_bb != node->entry)
            {
                for (auto &instr1 : loop_bb->get_instructions())
                {
                    auto instr = &instr1;
                    if (instr->is_alloca())
                    {
                        unchange.insert(instr);
                    }
                    else if (instr->is_gep())
                    {
                        int i;
                        for (i = 0; i < instr->get_num_operand(); i++)
                        {
                            Instruction *operand;
                            if (IS_Instr(instr->get_operand(i)))
                                operand = static_cast<Instruction *>(instr->get_operand(i));
                            if (!((operand != nullptr && unchange.find(operand) != unchange.end()) || IS_Const(instr->get_operand(i)) || IS_Arg(instr->get_operand(i))))
                                break;
                        }
                        if (i == instr->get_num_operand())
                        {
                            unchange.insert(instr);
                        }
                    }
                    else if (instr->isBinary())
                    {
                        Instruction *operand1;
                        Instruction *operand2;
                        if (IS_Instr(instr->get_operand(0)))
                            operand1 = static_cast<Instruction *>(instr->get_operand(0));
                        if (IS_Instr(instr->get_operand(1)))
                            operand2 = static_cast<Instruction *>(instr->get_operand(1));
                        if ((operand1 != nullptr && unchange.find(operand1) != unchange.end()) || IS_Const(instr->get_operand(0)) || IS_Arg(instr->get_operand(0)))
                        {
                            if ((operand2 != nullptr && unchange.find(operand2) != unchange.end()) || IS_Const(instr->get_operand(1)) || IS_Arg(instr->get_operand(1)))
                            {
                                unchange.insert(instr);
                            }
                        }
                    }
                    else if (instr->is_fp2si() || instr->is_si2fp() || instr->is_zext())
                    {
                        Instruction *operand;
                        if (IS_Instr(instr->get_operand(0)))
                            operand = static_cast<Instruction *>(instr->get_operand(0));
                        if ((operand != nullptr && unchange.find(operand) != unchange.end()) || IS_Const(instr->get_operand(0)) || IS_Arg(instr->get_operand(0)))
                        {
                            unchange.insert(instr);
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (auto &inner_node : node->inner_loop)
            Extract(inner_node.get());
    }
}

void LoopHoist::LoopExtract(Function *f)
{
    std::map<BasicBlock *, BasicBlock *> add_pre_BB;
    for (auto &loop : findloop_->get_function_loop(f))
    {
        Extract(loop.get());
        BasicBlock *bb = loop->entry;
        if (!unchange.empty())
        {
            BasicBlock *pre_BB;
            if (add_pre_BB.find(bb) == add_pre_BB.end())
            {
                auto func = bb->get_parent();
                std::set<BasicBlock *> wait_to_change;
                pre_BB = BasicBlock::create(m_, "", func);
                for (auto &pre_bb : bb->get_pre_basic_blocks())
                {
                    if (loop->Loop.find(pre_bb) == loop->Loop.end())
                    {
                        wait_to_change.insert(pre_bb);
                    }
                }
                for (auto &pre_bb : wait_to_change)
                {
                    bb->remove_pre_basic_block(pre_bb);
                    pre_bb->remove_succ_basic_block(bb);
                    pre_bb->add_succ_basic_block(pre_BB);
                    pre_BB->add_pre_basic_block(pre_bb);
                }
                for (auto &pre_bb : pre_BB->get_pre_basic_blocks())
                {
                    for (auto &instr1 : pre_bb->get_instructions())
                    {
                        auto instr = &instr1;
                        if (instr->is_br())
                        {
                            if (instr->get_num_operand() == 1)
                            {
                                if (instr->get_operand(0) == bb)
                                    instr->set_operand(0, pre_BB);
                            }
                            else if (instr->get_num_operand() == 3)
                            {
                                if (instr->get_operand(1) == bb)
                                    instr->set_operand(1, pre_BB);
                                else if (instr->get_operand(2) == bb)
                                    instr->set_operand(2, pre_BB);
                            }
                        }
                    }
                }
                for (auto &instr1 : bb->get_instructions())
                {
                    std::vector<Value *> vals;
                    std::vector<BasicBlock *> bbs;
                    auto instr = &instr1;
                    if (instr->is_phi())
                    {
                        int num = instr->get_num_operand();
                        for (int i = 1; i < num; i = i + 2)
                        {
                            if (wait_to_change.find(static_cast<BasicBlock *>(instr->get_operand(i))) != wait_to_change.end())
                            {
                                vals.push_back(instr->get_operand(i - 1));
                                bbs.push_back(static_cast<BasicBlock *>(instr->get_operand(i)));
                            }
                        }
                    }
                    if (bbs.size() == 1)
                    {
                        instr->set_operand(instr->get_operand_num(bbs[0]), pre_BB);
                    }
                    else if (bbs.size() > 1)
                    {
                        auto new_instr = PhiInst::create_phi(instr->get_type(), pre_BB, vals, bbs);
                        pre_BB->add_instr_begin(new_instr);
                        for (auto &val : vals)
                        {
                            instr->remove_operand(instr->get_operand_num(val));
                        }
                        for (auto &bb_val : bbs)
                        {
                            instr->remove_operand(instr->get_operand_num(bb_val));
                        }
                        if (instr->get_num_operand() == 0)
                        {
                            instr->replace_all_use_with(new_instr);
                            instr->get_parent()->erase_instr(instr);
                        }
                        else
                        {
                            instr->add_operand(new_instr);
                            instr->add_operand(pre_BB);
                        }
                    }
                }
                add_pre_BB.insert({bb, pre_BB});
            }
            else
                pre_BB = add_pre_BB[bb];
            for (auto &instr : unchange)
            {
                CopyInstr(pre_BB, instr);
            }
            auto br_instr = BranchInst::create_br(bb, pre_BB);
            for (auto inst : unchange)
                inst->remove_all_operands();
            for (auto inst : unchange)
                inst->get_parent()->erase_instr(inst);
        }
        unchange.clear();
    }
    return;
}