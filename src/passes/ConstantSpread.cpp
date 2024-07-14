#include "ConstantSpread.hpp"
#include "Instruction.hpp"

#define IS_Const(val) dynamic_cast<Constant *>(val)
#define IS_Int(val) dynamic_cast<ConstantInt *>(val)
#define IS_FP(val) dynamic_cast<ConstantFP *>(val)

void ConstantSpread::run()
{
    for (auto &f1 : m_->get_functions())
    {
        auto f = &f1;
        if (!f->is_declaration())
            SpreadConstant(f);
    }
}

void ConstantSpread::SpreadConstant(Function *f)
{
    std::vector<Instruction *> wait_delete;
    for (auto &bb1 : f->get_basic_blocks())
    {
        auto bb = &bb1;
        for (auto &instr1 : bb->get_instructions())
        {
            auto instr = &instr1;
            if (instr->isBinary())
            {
                if (IS_Const(instr->get_operand(0)) && IS_Const(instr->get_operand(1)))
                {
                    auto value_0 = IS_Int(instr->get_operand(0)) ? static_cast<ConstantInt *>(instr->get_operand(0))->get_value() : static_cast<ConstantFP *>(instr->get_operand(0))->get_value();
                    auto value_1 = IS_Int(instr->get_operand(1)) ? static_cast<ConstantInt *>(instr->get_operand(1))->get_value() : static_cast<ConstantFP *>(instr->get_operand(1))->get_value();
                    if (instr->is_add())
                    {
                        int value_3 = value_0 + value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_sub())
                    {
                        int value_3 = value_0 - value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_mul())
                    {
                        int value_3 = value_0 * value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_div())
                    {
                        int value_3 = value_0 / value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_srem())
                    {
                        int value_3 = (int)value_0 % (int)value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_fadd())
                    {
                        float value_3 = value_0 + value_1;
                        instr->replace_all_use_with(ConstantFP::get(value_3, m_));
                    }
                    else if (instr->is_fsub())
                    {
                        float value_3 = value_0 - value_1;
                        instr->replace_all_use_with(ConstantFP::get(value_3, m_));
                    }
                    else if (instr->is_fmul())
                    {
                        float value_3 = value_0 * value_1;
                        instr->replace_all_use_with(ConstantFP::get(value_3, m_));
                    }
                    else if (instr->is_fdiv())
                    {
                        float value_3 = value_0 / value_1;
                        instr->replace_all_use_with(ConstantFP::get(value_3, m_));
                    }
                    wait_delete.push_back(instr);
                }
            }
            else if (instr->is_cmp() || instr->is_fcmp())
            {
                if (IS_Const(instr->get_operand(0)) && IS_Const(instr->get_operand(1)))
                {
                    auto value_0 = IS_Int(instr->get_operand(0)) ? static_cast<ConstantInt *>(instr->get_operand(0))->get_value() : static_cast<ConstantFP *>(instr->get_operand(0))->get_value();
                    auto value_1 = IS_Int(instr->get_operand(1)) ? static_cast<ConstantInt *>(instr->get_operand(1))->get_value() : static_cast<ConstantFP *>(instr->get_operand(1))->get_value();
                    if (instr->is_ge() || instr->is_fge())
                    {
                        bool value_3 = value_0 >= value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_gt() || instr->is_fgt())
                    {
                        bool value_3 = value_0 > value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_le() || instr->is_fle())
                    {
                        bool value_3 = value_0 <= value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_lt() || instr->is_flt())
                    {
                        bool value_3 = value_0 < value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_eq() || instr->is_feq())
                    {
                        bool value_3 = value_0 == value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_ne() || instr->is_fne())
                    {
                        bool value_3 = value_0 != value_1;
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    wait_delete.push_back(instr);
                }
            }
            else if (instr->is_zext() || instr->is_fp2si() || instr->is_si2fp())
            {
                if (IS_Const(instr->get_operand(0)))
                {
                    auto value_0 = IS_Int(instr->get_operand(0)) ? static_cast<ConstantInt *>(instr->get_operand(0))->get_value() : static_cast<ConstantFP *>(instr->get_operand(0))->get_value();
                    if (instr->is_fp2si())
                    {
                        int value_3 = (int)(value_0);
                        instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                    }
                    else if (instr->is_si2fp())
                    {
                        float value_3 = (float)(value_0);
                        instr->replace_all_use_with(ConstantFP::get(value_3, m_));
                    }
                    else if (instr->is_zext())
                    {
                        if (instr->get_type() == m_->get_int1_type())
                        {
                            bool value_3 = value_0;
                            instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                        }
                        else if (instr->get_type() == m_->get_int32_type())
                        {
                            int value_3 = (int)(value_0);
                            instr->replace_all_use_with(ConstantInt::get(value_3, m_));
                        }
                    }
                    wait_delete.push_back(instr);
                }
            }
        }
    }
    for (auto &inst : wait_delete)
        inst->remove_all_operands();
    for (auto &inst : wait_delete)
        inst->get_parent()->erase_instr(inst);
}

