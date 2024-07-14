#include <cstring>
#include <sstream>
#include <stack>
#include <set>
#include "RegAlloca.hpp"
#include "CodeGenUtil.hpp"

#define CONST_INT(num) ConstantInt::get(num, m)
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)
#define IS_Global(val) dynamic_cast<GlobalVariable *>(val)
#define IS_Instr(val) dynamic_cast<Instruction *>(val)
#define IS_Arg(val) dynamic_cast<Argument *>(val)
#define IS_Const(val) dynamic_cast<Constant *>(val)
float FP_zero = 0.;
std::set<int> instr_reg_operand;
std::set<int> instr_freg_operand;

BasicBlock *current_bb;
void CodeGen_Reg::allocate()
{

    unsigned offset = PROLOGUE_OFFSET_BASE;

    for (auto &arg : context.func->get_args())
    {
        auto size = arg.get_type()->get_size();
        offset = ALIGN(offset + size, size);
        context.offset_map[&arg] = -static_cast<int>(offset);
    }

    for (auto &bb : context.func->get_basic_blocks())
    {
        for (auto &instr : bb.get_instructions())
        {
            if (not instr.is_void())
            {
                auto size = instr.get_type()->get_size();
                offset = ALIGN(offset + size, size);
                context.offset_map[&instr] = -static_cast<int>(offset);
            }
            if (instr.is_alloca())
            {
                auto *alloca_inst = static_cast<AllocaInst *>(&instr);
                if (alloca_inst->get_alloca_type()->is_array_type())
                {
                    Type *swap_type = alloca_inst->get_alloca_type();
                    while (swap_type->is_array_type())
                        swap_type = static_cast<ArrayType *>(swap_type)->get_array_element_type();
                    auto alloc_size = swap_type->get_size();
                    offset = ALIGN(offset + alloc_size, alloc_size);
                    offset += alloca_inst->get_alloca_type()->get_size();
                }
                else if (alloca_inst->get_alloca_type()->is_float_type() || alloca_inst->get_alloca_type()->is_integer_type() || alloca_inst->get_alloca_type()->is_pointer_type())
                {
                    auto alloc_size = alloca_inst->get_alloca_type()->get_size();
                    offset = ALIGN(offset + alloc_size, alloc_size);
                }
            }
        }
    }

    context.frame_size = ALIGN(offset, PROLOGUE_ALIGN);
}

void CodeGen_Reg::load_to_greg(Value *val, const Reg &reg)
{
    assert(val->get_type()->is_integer_type() || val->get_type()->is_pointer_type());

    if (auto *constant = dynamic_cast<ConstantInt *>(val))
    {
        int64_t val = constant->get_value();
        if (IS_IMM_12(val))
        {
            append_inst(ADDI WORD, {reg.print(), "$zero", std::to_string(val)});
        }
        else if (IS_IMM_32(val))
        {
            load_large_int32((int32_t)val, reg);
        }
        else
        {
            load_large_int64(val, reg);
        }
    }
    else if (auto *global = IS_Global(val))
    {
        append_inst(LOAD_ADDR, {reg.print(), global->get_name()});
    }
    else
    {
        load_from_stack_to_greg(val, reg);
    }
}

void CodeGen_Reg::load_large_int32(int32_t val, const Reg &reg)
{
    int32_t high_20 = val >> 12;
    uint32_t low_12 = val & LOW_12_MASK;
    append_inst(LU12I_W, {reg.print(), std::to_string(high_20)});
    append_inst(ORI, {reg.print(), reg.print(), std::to_string(low_12)});
}

void CodeGen_Reg::load_large_int64(int64_t val, const Reg &reg)
{
    auto low_32 = static_cast<int32_t>(val & LOW_32_MASK);
    load_large_int32(low_32, reg);

    auto high_32 = static_cast<int32_t>(val >> 32);
    int32_t high_32_low_20 = (high_32 << 12) >> 12;
    int32_t high_32_high_12 = high_32 >> 20;
    append_inst(LU32I_D, {reg.print(), std::to_string(high_32_low_20)});
    append_inst(LU52I_D,
                {reg.print(), reg.print(), std::to_string(high_32_high_12)});
}

void CodeGen_Reg::load_from_stack_to_greg(Value *val, const Reg &reg)
{
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset))
    {
        if (type->is_int1_type())
        {
            append_inst(LOAD BYTE, {reg.print(), "$fp", offset_str});
        }
        else if (type->is_int32_type())
        {
            append_inst(LOAD WORD, {reg.print(), "$fp", offset_str});
        }
        else
        {
            append_inst(LOAD DOUBLE, {reg.print(), "$fp", offset_str});
        }
    }
    else
    {
        load_large_int64(offset, reg);
        append_inst(ADD DOUBLE, {reg.print(), "$fp", reg.print()});
        if (type->is_int1_type())
        {
            append_inst(LOAD BYTE, {reg.print(), reg.print(), "0"});
        }
        else if (type->is_int32_type())
        {
            append_inst(LOAD WORD, {reg.print(), reg.print(), "0"});
        }
        else
        {
            append_inst(LOAD DOUBLE, {reg.print(), reg.print(), "0"});
        }
    }
}

void CodeGen_Reg::store_from_greg(Value *val, const Reg &reg)
{
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset))
    {
        if (type->is_int1_type())
        {
            append_inst(STORE BYTE, {reg.print(), "$fp", offset_str});
        }
        else if (type->is_int32_type())
        {
            append_inst(STORE WORD, {reg.print(), "$fp", offset_str});
        }
        else
        {
            append_inst(STORE DOUBLE, {reg.print(), "$fp", offset_str});
        }
    }
    else
    {
        int temp_addr = 2;
        load_large_int64(offset, Reg::a(temp_addr));
        append_inst(ADD DOUBLE, {Reg::a(temp_addr).print(), "$fp", Reg::a(temp_addr).print()});
        if (type->is_int1_type())
        {
            append_inst(STORE BYTE, {reg.print(), Reg::a(temp_addr).print(), "0"});
        }
        else if (type->is_int32_type())
        {
            append_inst(STORE WORD, {reg.print(), Reg::a(temp_addr).print(), "0"});
        }
        else
        {
            append_inst(STORE DOUBLE, {reg.print(), Reg::a(temp_addr).print(), "0"});
        }
    }
}

void CodeGen_Reg::load_to_freg(Value *val, const FReg &freg)
{
    assert(val->get_type()->is_float_type());
    if (auto *constant = dynamic_cast<ConstantFP *>(val))
    {
        float val = constant->get_value();
        load_float_imm(val, freg);
    }
    else
    {
        auto offset = context.offset_map.at(val);
        auto offset_str = std::to_string(offset);
        if (IS_IMM_12(offset))
        {
            append_inst(FLOAD SINGLE, {freg.print(), "$fp", offset_str});
        }
        else
        {
            int addr_num = get_value_reg_freg(ConstantInt::get(0, m));
            load_large_int64(offset, Reg::t(addr_num));
            append_inst(ADD DOUBLE, {Reg::t(addr_num).print(), "$fp", Reg::t(addr_num).print()});
            append_inst(FLOAD SINGLE, {freg.print(), Reg::t(addr_num).print(), "0"});
        }
    }
}

void CodeGen_Reg::load_float_imm(float val, const FReg &r)
{
    int32_t bytes = *reinterpret_cast<int32_t *>(&val);
    unsigned reg_num = get_value_reg_freg(ConstantInt::get(0, m));
    load_large_int32(bytes, Reg::t(reg_num));
    append_inst(GR2FR WORD, {r.print(), Reg::t(reg_num).print()});
}

void CodeGen_Reg::store_from_freg(Value *val, const FReg &r)
{
    auto offset = context.offset_map.at(val);
    if (IS_IMM_12(offset))
    {
        auto offset_str = std::to_string(offset);
        append_inst(FSTORE SINGLE, {r.print(), "$fp", offset_str});
    }
    else
    {
        unsigned addr_num = get_value_reg_freg(ConstantInt::get(0, m));
        load_large_int64(offset, Reg::t(addr_num));
        append_inst(ADD DOUBLE, {Reg::t(addr_num).print(), "$fp", Reg::t(addr_num).print()});
        append_inst(FSTORE SINGLE, {r.print(), Reg::t(addr_num).print(), "0"});
    }
}

void CodeGen_Reg::gen_prologue()
{
    if (IS_IMM_12(-static_cast<int>(context.frame_size)))
    {
        append_inst("st.d $ra, $sp, -8");
        append_inst("st.d $fp, $sp, -16");
        append_inst("addi.d $fp, $sp, 0");
        append_inst("addi.d $sp, $sp, " + std::to_string(-static_cast<int>(context.frame_size)));
    }
    else
    {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("st.d $ra, $sp, -8");
        append_inst("st.d $fp, $sp, -16");
        append_inst("addi.d $fp, $sp, 0");
        append_inst("sub.d $sp, $sp, $t0");
    }

    if (context.func->get_num_of_args() <= 8)
    {
        int garg_cnt = 0;
        int farg_cnt = 0;
        for (auto &arg : context.func->get_args())
        {
            if (arg.get_type()->is_float_type())
            {
                store_from_freg(&arg, FReg::fa(farg_cnt++));
            }
            else
            {
                store_from_greg(&arg, Reg::a(garg_cnt++));
            }
        }
    }
}

void CodeGen_Reg::gen_epilogue()
{
    if (IS_IMM_12(-static_cast<int>(context.frame_size)))
    {
        append_inst("addi.d $sp, $sp, " + std::to_string(static_cast<int>(context.frame_size)));
        append_inst("ld.d $ra, $sp, -8");
        append_inst("ld.d $fp, $sp, -16");
        append_inst("jr $ra");
    }
    else
    {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add.d $sp, $sp, $t0");
        append_inst("ld.d $ra, $sp, -8");
        append_inst("ld.d $fp, $sp, -16");
        append_inst("jr $ra");
    }
}

void CodeGen_Reg::gen_ret()
{
    int count = context.inst->get_num_operand();
    if (count)
    {
        auto *ptr = context.inst->get_operand(0);
        int ret_num = get_value_reg_freg(ptr);
        if (ptr->get_type()->is_float_type())
        {
            //load_to_freg(ConstantFP::get(FP_zero, m), FReg::fa(0));
            append_inst("movgr2fr.w", {FReg::fa(0).print(), "$zero"});
            append_inst("fadd.s", {FReg::fa(0).print(), FReg::ft(ret_num).print(), FReg::fa(0).print()});
        }
        else
        {
            if (ptr->get_type()->is_pointer_type())
            {
                append_inst("addi.d", {Reg::a(0).print(), Reg::t(ret_num).print(), "0"});
            }
            else
            {
                append_inst("addi.w", {Reg::a(0).print(), Reg::t(ret_num).print(), "0"});
            }
        }
    }
    else
    {
        append_inst("addi.w $a0, $zero, 0");
    }
    gen_epilogue();
}

void CodeGen_Reg::gen_br()
{
    gen_phi();
    Set_livevar_out(current_bb);
    auto *branchInst = static_cast<BranchInst *>(context.inst);
    if (branchInst->is_cond_br())
    {
        int reg_num = get_value_reg_freg(branchInst->get_operand(0));
        auto *branchbb1 = static_cast<BasicBlock *>(branchInst->get_operand(1));
        auto *branchbb2 = static_cast<BasicBlock *>(branchInst->get_operand(2));
        append_inst("beqz", {Reg::t(reg_num).print(), label_name(branchbb2)});
        append_inst("bnez", {Reg::t(reg_num).print(), label_name(branchbb1)});
    }
    else
    {
        auto *branchbb = static_cast<BasicBlock *>(branchInst->get_operand(0));
        append_inst("b " + label_name(branchbb));
    }
}

void CodeGen_Reg::gen_binary()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *ptr_1 = context.inst->get_operand(1);
    unsigned operand0_num = get_value_reg_freg(ptr_0);
    instr_reg_operand.insert(operand0_num);
    unsigned operand1_num = get_value_reg_freg(ptr_1);
    instr_reg_operand.insert(operand1_num);
    unsigned reg_num = get_value_reg_freg(context.inst);
    switch (context.inst->get_instr_type())
    {
    case Instruction::add:
        append_inst("add.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        break;
    case Instruction::sub:
        append_inst("sub.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        break;
    case Instruction::mul:
        append_inst("mul.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        break;
    case Instruction::sdiv:
        append_inst("div.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        break;
    case Instruction::srem:
        append_inst("div.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        append_inst("mul.w", {Reg::t(reg_num).print(), Reg::t(reg_num).print(), Reg::t(operand1_num).print()});
        append_inst("sub.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(reg_num).print()});
        break;
    default:
        assert(false);
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_float_binary()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *ptr_1 = context.inst->get_operand(1);
    unsigned operand0_num = get_value_reg_freg(ptr_0);
    instr_freg_operand.insert(operand0_num);
    unsigned operand1_num = get_value_reg_freg(ptr_1);
    instr_freg_operand.insert(operand1_num);
    unsigned freg_num = get_fregister(context.inst);
    switch (context.inst->get_instr_type())
    {
    case Instruction::fadd:
        append_inst("fadd.s", {FReg::ft(freg_num).print(), FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    case Instruction::fsub:
        append_inst("fsub.s", {FReg::ft(freg_num).print(), FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    case Instruction::fmul:
        append_inst("fmul.s", {FReg::ft(freg_num).print(), FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    case Instruction::fdiv:
        append_inst("fdiv.s", {FReg::ft(freg_num).print(), FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    default:
        assert(false);
    }
    instr_freg_operand.clear();
}

void CodeGen_Reg::gen_alloca()
{
    auto offset = context.offset_map[context.inst];
    auto *alloca_inst = static_cast<AllocaInst *>(context.inst);
    offset = -offset;
    if (alloca_inst->get_alloca_type()->is_array_type())
    {
        Type *swap_type = alloca_inst->get_alloca_type();
        while (swap_type->is_array_type())
            swap_type = static_cast<ArrayType *>(swap_type)->get_array_element_type();
        auto alloc_size = swap_type->get_size();
        offset = ALIGN(offset + alloc_size, alloc_size);
        offset += alloca_inst->get_alloca_type()->get_size();
    }
    else if (alloca_inst->get_alloca_type()->is_float_type() || alloca_inst->get_alloca_type()->is_integer_type() || alloca_inst->get_alloca_type()->is_pointer_type())
    {
        auto alloc_size = alloca_inst->get_alloca_type()->get_size();
        offset = ALIGN(offset + alloc_size, alloc_size);
    }
    offset = -offset;
    unsigned alloc_num = get_value_reg_freg(context.inst);
    if (IS_IMM_12(offset))
    {
        append_inst("addi.d", {Reg::t(alloc_num).print(), "$fp", std::to_string(offset)});
    }
    else
    {
        int64_t offset_64 = static_cast<int64_t>(offset);
        load_large_int64(offset_64, Reg::t(alloc_num));
        append_inst("add.d", {Reg::t(alloc_num).print(), "$fp", Reg::t(alloc_num).print()});
    }
}

void CodeGen_Reg::gen_load()
{
    auto *ptr = context.inst->get_operand(0);
    auto *type = context.inst->get_type();
    int ptr_reg_num = get_value_reg_freg(ptr);
    instr_reg_operand.insert(ptr_reg_num);
    if (type->is_float_type())
    {
        int freg_num = get_value_reg_freg(context.inst);
        append_inst("fld.s", {FReg::ft(freg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
    }
    else
    {
        int reg_num = get_value_reg_freg(context.inst);
        if (type->is_int1_type())
        {
            append_inst("ld.b", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
        else if (type->is_int32_type())
        {
            append_inst("ld.w", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
        else
        {
            append_inst("ld.d", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_store()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *ptr_1 = context.inst->get_operand(1);
    int ptr_reg_num = get_value_reg_freg(ptr_1);
    instr_reg_operand.insert(ptr_reg_num);
    auto *type = ptr_0->get_type();
    if (type->is_float_type())
    {
        int freg_num = get_value_reg_freg(ptr_0);
        append_inst("fst.s", {FReg::ft(freg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
    }
    else
    {
        int reg_num = get_value_reg_freg(ptr_0);
        if (type->is_int1_type())
        {
            append_inst("st.b", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
        else if (type->is_int32_type())
        {
            append_inst("st.w", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
        else
        {
            append_inst("st.d", {Reg::t(reg_num).print(), Reg::t(ptr_reg_num).print(), "0"});
        }
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_icmp()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *ptr_1 = context.inst->get_operand(1);
    unsigned operand0_num = get_value_reg_freg(ptr_0);
    instr_reg_operand.insert(operand0_num);
    unsigned operand1_num = get_value_reg_freg(ptr_1);
    instr_reg_operand.insert(operand1_num);
    unsigned reg_num = get_value_reg_freg(context.inst);
    switch (context.inst->get_instr_type())
    {
    case Instruction::ge:
    {
        append_inst("slt", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        append_inst("xori", {Reg::t(reg_num).print(), Reg::t(reg_num).print(), "1"});
        break;
    }
    case Instruction::gt:
    {
        append_inst("slt", {Reg::t(reg_num).print(), Reg::t(operand1_num).print(), Reg::t(operand0_num).print()});
        break;
    }
    case Instruction::le:
    {
        append_inst("slt", {Reg::t(reg_num).print(), Reg::t(operand1_num).print(), Reg::t(operand0_num).print()});
        append_inst("xori", {Reg::t(reg_num).print(), Reg::t(reg_num).print(), "1"});
        break;
    }
    case Instruction::lt:
    {
        append_inst("slt", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        break;
    }
    case Instruction::eq:
    {
        append_inst("xor", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        append_inst("sltu", {Reg::t(reg_num).print(), "$zero", Reg::t(reg_num).print()});
        append_inst("xori", {Reg::t(reg_num).print(), Reg::t(reg_num).print(), "1"});
        break;
    }
    case Instruction::ne:
    {
        append_inst("xor", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), Reg::t(operand1_num).print()});
        append_inst("sltu", {Reg::t(reg_num).print(), "$zero", Reg::t(reg_num).print()});
        break;
    }
    default:
        break;
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_fcmp()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *ptr_1 = context.inst->get_operand(1);
    unsigned operand0_num = get_value_reg_freg(ptr_0);
    instr_freg_operand.insert(operand0_num);
    unsigned operand1_num = get_value_reg_freg(ptr_1);
    unsigned reg_num = get_value_reg_freg(context.inst);
    switch (context.inst->get_instr_type())
    {
    case Instruction::fge:
    {
        append_inst("fcmp.sle.s", {"$fcc0", FReg::ft(operand1_num).print(), FReg::ft(operand0_num).print()});
        break;
    }
    case Instruction::fgt:
    {
        append_inst("fcmp.slt.s", {"$fcc0", FReg::ft(operand1_num).print(), FReg::ft(operand0_num).print()});
        break;
    }
    case Instruction::fle:
    {
        append_inst("fcmp.sle.s", {"$fcc0", FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    }
    case Instruction::flt:
    {
        append_inst("fcmp.slt.s", {"$fcc0", FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    }
    case Instruction::feq:
    {
        append_inst("fcmp.seq.s", {"$fcc0", FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    }
    case Instruction::fne:
    {
        append_inst("fcmp.sne.s", {"$fcc0", FReg::ft(operand0_num).print(), FReg::ft(operand1_num).print()});
        break;
    }
    default:
        break;
    }
    append_inst("bcnez $fcc0, 0xC");
    append_inst("addi.w", {Reg::t(reg_num).print(), "$zero", "0"});
    append_inst("b 0x8");
    append_inst("addi.w", {Reg::t(reg_num).print(), "$zero", "1"});
    instr_freg_operand.clear();
}

void CodeGen_Reg::gen_zext()
{
    auto *ptr_0 = context.inst->get_operand(0);
    auto *type_0 = ptr_0->get_type();
    int operand0_num = get_value_reg_freg(ptr_0);
    instr_reg_operand.insert(operand0_num);
    int reg_num = get_value_reg_freg(context.inst);
    if (context.inst->get_type()->is_int32_type())
    {
        append_inst("bstrpick.w", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), "0", "0"});
    }
    else
    {
        if (type_0->is_int1_type())
        {
            append_inst("bstrpick.d", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), "0", "0"});
        }
        else if (type_0->is_int32_type())
        {
            append_inst("bstrpick.d", {Reg::t(reg_num).print(), Reg::t(operand0_num).print(), "31", "0"});
        }
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_call()
{
    for (auto &it : context.reg_map)
    {
        if (it.second != nullptr)
        {
            Value *val = it.second;
            if (IS_Instr(val))
                store_from_greg(val, Reg::t(it.first));
        }
    }
    for (auto &it : context.freg_map)
    {
        if (it.second != nullptr)
        {
            Value *val = it.second;
            if (IS_Instr(val))
                store_from_freg(val, FReg::ft(it.first));
        }
    }
    int num = context.inst->get_num_operand();
    auto *func = context.inst->get_operand(0);
    Function *func_;
    if (dynamic_cast<Function *>(func))
        func_ = static_cast<Function *>(func);
    if (num - 1 <= 8)
    {
        int garg_cnt = 0;
        int farg_cnt = 0;
        int i = 1;
        while (i < num)
        {
            auto *ptr = context.inst->get_operand(i);
            if (ptr->get_type()->is_float_type())
            {
                int freg_num = get_value_reg_freg(ptr);
                //load_to_freg(ConstantFP::get(FP_zero, m), FReg::fa(farg_cnt));
                append_inst("movgr2fr.w", {FReg::fa(farg_cnt).print(), "$zero"});
                append_inst("fadd.s", {FReg::fa(farg_cnt).print(), FReg::ft(freg_num).print(), FReg::fa(farg_cnt).print()});
                farg_cnt++;
            }
            else
            {
                int reg_num = get_value_reg_freg(ptr);
                if (ptr->get_type()->is_pointer_type())
                    append_inst("addi.d", {Reg::a(garg_cnt++).print(), Reg::t(reg_num).print(), "0"});
                else
                    append_inst("addi.w", {Reg::a(garg_cnt++).print(), Reg::t(reg_num).print(), "0"});
            }
            i++;
        }
    }
    else
    {
        unsigned init_offset = PROLOGUE_ALIGN;
        int i = 1;
        unsigned reg_num = get_value_reg_freg(ConstantInt::get(0, m));
        instr_reg_operand.insert(reg_num);
        for (auto &arg : func_->get_args())
        {
            auto size = arg.get_type()->get_size();
            init_offset = ALIGN(init_offset + size, size);
            load_to_greg(CONST_INT((int)-init_offset), Reg::t(reg_num));
            append_inst("add.d", {Reg::t(reg_num).print(), "$sp", Reg::t(reg_num).print()});
            auto *ptr = context.inst->get_operand(i++);
            if (ptr->get_type()->is_float_type())
            {
                unsigned ptr_num = get_value_reg_freg(ptr);
                append_inst("fst.s", {FReg::ft(ptr_num).print(), Reg::t(reg_num).print(), "0"});
            }
            else
            {
                unsigned ptr_num = get_value_reg_freg(ptr);
                if (ptr->get_type()->is_pointer_type())
                    append_inst("st.d", {Reg::t(ptr_num).print(), Reg::t(reg_num).print(), "0"});
                else
                    append_inst("st.w", {Reg::t(ptr_num).print(), Reg::t(reg_num).print(), "0"});
            }
        }
        instr_reg_operand.clear();
    }
    append_inst("bl " + func->get_name());
    for (auto &it : context.reg_map)
    {
        if (it.second != nullptr)
        {
            Value *val = it.second;
            if (IS_Instr(val) || IS_Arg(val) || IS_Global(val))
                load_to_greg(val, Reg::t(it.first));
        }
    }
    for (auto &it : context.freg_map)
    {
        if (it.second != nullptr)
        {
            Value *val = it.second;
            if (IS_Instr(val) || IS_Arg(val) || IS_Global(val))
                load_to_freg(val, FReg::ft(it.first));
        }
    }
    if (!context.inst->is_void())
    {
        int instr_reg_num = get_value_reg_freg(context.inst);
        if (context.inst->get_type()->is_float_type())
        {
            // load_to_freg(ConstantFP::get(FP_zero, m), FReg::ft(instr_reg_num));
            append_inst("movgr2fr.w", {FReg::ft(instr_reg_num).print(), "$zero"});
            append_inst("fadd.s", {FReg::ft(instr_reg_num).print(), FReg::fa(0).print(), FReg::ft(instr_reg_num).print()});
        }
        else
        {
            if (context.inst->get_type()->is_pointer_type())
            {
                append_inst("addi.d", {Reg::t(instr_reg_num).print(), Reg::a(0).print(), "0"});
            }
            else
            {
                append_inst("addi.w", {Reg::t(instr_reg_num).print(), Reg::a(0).print(), "0"});
            }
        }
    }
}

void CodeGen_Reg::gen_gep()
{
    int count = context.inst->get_num_operand();
    unsigned instr_num = get_value_reg_freg(context.inst);
    instr_reg_operand.insert(instr_num);
    if (count == 3)
    {
        auto *ptr = context.inst->get_operand(0);
        unsigned operand0_num = get_value_reg_freg(ptr);
        instr_reg_operand.insert(operand0_num);
        Type *content_type = ptr->get_type();
        if (content_type->is_pointer_type())
            content_type = content_type->get_pointer_element_type();
        if (content_type->is_array_type())
            content_type = content_type->get_array_element_type();
        int size = content_type->get_size();
        load_to_greg(CONST_INT(size), Reg::t(instr_num));
        auto *num = context.inst->get_operand(2);
        unsigned operand2_num = get_value_reg_freg(num);
        append_inst("mul.d", {Reg::t(instr_num).print(), Reg::t(operand2_num).print(), Reg::t(instr_num).print()});
        append_inst("add.d", {Reg::t(instr_num).print(), Reg::t(instr_num).print(), Reg::t(operand0_num).print()});
    }
    else if (count == 2)
    {
        auto *ptr = context.inst->get_operand(0);
        unsigned operand0_num = get_value_reg_freg(ptr);
        instr_reg_operand.insert(operand0_num);
        Type *content_type = ptr->get_type();
        if (content_type->is_pointer_type())
            content_type = content_type->get_pointer_element_type();
        int size = content_type->get_size();
        // load_large_int32(size, Reg::t(instr_num));
        load_to_greg(CONST_INT(size), Reg::t(instr_num));
        auto *num = context.inst->get_operand(1);
        unsigned operand1_num = get_value_reg_freg(num);
        append_inst("mul.d", {Reg::t(instr_num).print(), Reg::t(operand1_num).print(), Reg::t(instr_num).print()});
        append_inst("add.d", {Reg::t(instr_num).print(), Reg::t(instr_num).print(), Reg::t(operand0_num).print()});
    }
    instr_reg_operand.clear();
}

void CodeGen_Reg::gen_sitofp()
{
    auto *ptr = context.inst->get_operand(0);
    int reg_num = get_value_reg_freg(ptr);
    int freg_num = get_value_reg_freg(context.inst);
    append_inst("movgr2fr.w", {FReg::ft(freg_num).print(), Reg::t(reg_num).print()});
    append_inst("ffint.s.w", {FReg::ft(freg_num).print(), FReg::ft(freg_num).print()});
}

void CodeGen_Reg::gen_fptosi()
{
    auto *ptr = context.inst->get_operand(0);
    int freg_num = get_value_reg_freg(ptr);
    int reg_num = get_value_reg_freg(context.inst);
    append_inst("ftintrz.w.s", {FReg::ft(freg_num).print(), FReg::ft(freg_num).print()});
    append_inst("movfr2gr.s", {Reg::t(reg_num).print(), FReg::ft(freg_num).print()});
}

void CodeGen_Reg::gen_phi()
{
    append_inst("phi gen : ", ASMInstruction::Comment);
    auto bb = context.inst->get_parent();
    auto &bb_succ_list = bb->get_succ_basic_blocks();
    for (auto bb_next : bb_succ_list)
    {
        for (auto &instr1 : bb_next->get_instructions())
        {
            auto instr = &instr1;
            if (instr->is_phi())
            {
                int num = instr->get_num_operand();
                for (int i = 0; i + 1 < num; i = i + 2)
                {
                    if (instr->get_operand(i + 1) == bb)
                    {
                        if (instr->get_operand(i)->get_type()->is_float_type())
                        {
                            int freg_num = get_value_reg_freg(instr->get_operand(i));
                            if (context.value_freg[instr] != -1)
                            {
                                int num = context.value_freg[instr];
                               // load_to_freg(ConstantFP::get(FP_zero, m), FReg::ft(num));
                                append_inst("movgr2fr.w", {FReg::ft(num).print(), "$zero"});
                                append_inst("fadd.s", {FReg::ft(num).print(), FReg::ft(freg_num).print(), FReg::ft(num).print()});
                            }
                            store_from_freg(instr, FReg::ft(freg_num));
                        }
                        else
                        {
                            int reg_num = get_value_reg_freg(instr->get_operand(i));
                            if (context.value_reg[instr] != -1)
                            {
                                int num = context.value_reg[instr];
                                if (instr->get_type()->is_pointer_type())
                                    append_inst("add.d", {Reg::t(num).print(), Reg::t(reg_num).print(), "$zero"});
                                else
                                    append_inst("add.w", {Reg::t(num).print(), Reg::t(reg_num).print(), "$zero"});
                            }
                            store_from_greg(instr, Reg::t(reg_num));
                        }
                    }
                }
            }
        }
    }
}

void CodeGen_Reg::gen_global_array_init(Constant *init, Type *array_type, Type *val_type)
{
    if (dynamic_cast<ConstantArray *>(init))
    {
        auto init_array = static_cast<ConstantArray *>(init);
        int count = init_array->get_size_of_array();
        if (count == 1)
        {
            unsigned count = array_type->get_size();
            append_inst(".space", {std::to_string(count)});
        }
        else
        {
            for (int i = 0; i < count; i++)
                gen_global_array_init(init_array->get_element_value(i), init_array->get_element_value(i)->get_type(), val_type);
        }
    }
    else if (dynamic_cast<ConstantZero *>(init))
    {
        unsigned count = array_type->get_size();
        append_inst(".space", {std::to_string(count)});
    }
    else
    {
        if (dynamic_cast<ConstantInt *>(init) && val_type->is_int32_type())
            append_inst(".word", {std::to_string(static_cast<ConstantInt *>(init)->get_value())});
        else if (dynamic_cast<ConstantFP *>(init) && val_type->is_float_type())
        {
            float val = static_cast<ConstantFP *>(init)->get_value();
            std::stringstream fp_val_ss;
            std::string fp_val;
            fp_val_ss << std::hex << *(uint32_t *)&val << std::endl;
            fp_val_ss >> fp_val;
            char *stop;
            int ans = strtol(fp_val.c_str(), &stop, 16);
            append_inst(".word", {std::to_string(ans)});
        }
    }
    return;
}

void CodeGen_Reg::set_instr_number(BasicBlock *bb)
{
    context.bb_order.push_back(bb);
    bb->begin_num_ = context.num;
    for (auto &instr1 : bb->get_instructions())
    {
        auto instr = &instr1;
        instr->line_num_ = context.num + 1;
        if (!static_cast<Instruction *>(instr)->name_num_)
        {
            instr->begin_num_ = context.num;
            static_cast<Instruction *>(instr)->name_num_ = true;
        }
        if (instr->end_num_ == 0)
            instr->end_num_ = context.num + 1;
        int num = instr->get_num_operand();
        for (int i = 0; i < num; i++)
        {
            if (IS_Instr(instr->get_operand(i)))
            {
                if (!static_cast<Instruction *>(instr->get_operand(i))->name_num_)
                {
                    static_cast<Instruction *>(instr->get_operand(i))->name_num_ = true;
                    static_cast<Instruction *>(instr->get_operand(i))->begin_num_ = instr->begin_num_;
                }
                static_cast<Instruction *>(instr->get_operand(i))->end_num_ = instr->end_num_;
            }
            else if (IS_Arg(instr->get_operand(i)))
            {
                if (!static_cast<Argument *>(instr->get_operand(i))->name_num_)
                {
                    static_cast<Argument *>(instr->get_operand(i))->name_num_ = true;
                    static_cast<Argument *>(instr->get_operand(i))->begin_num_ = instr->begin_num_;
                }
                static_cast<Argument *>(instr->get_operand(i))->end_num_ = instr->end_num_;
            }
            else if (IS_Global(instr->get_operand(i)))
            {
                if (!static_cast<GlobalVariable *>(instr->get_operand(i))->name_num_)
                {
                    static_cast<GlobalVariable *>(instr->get_operand(i))->name_num_ = true;
                    static_cast<GlobalVariable *>(instr->get_operand(i))->begin_num_ = instr->begin_num_;
                }
                static_cast<GlobalVariable *>(instr->get_operand(i))->end_num_ = instr->end_num_;
            }
        }
        context.num = context.num + 2;
    }
    bb->end_num_ = context.num - 1;
    std::set<BasicBlock *> entry;
    for (auto &succ_bb : bb->get_succ_basic_blocks())
    {
        if (findloop_->get_entry_loop(succ_bb) != nullptr)
            entry.insert(succ_bb);
    }
    for (auto &entry_bb : entry)
    {
        if (context.visit.find(entry_bb) == context.visit.end())
        {
            context.visit.insert(entry_bb);
            set_instr_number(entry_bb);
        }
    }
    for (auto &succ_bb : bb->get_succ_basic_blocks())
    {
        if (context.visit.find(succ_bb) == context.visit.end())
        {
            context.visit.insert(succ_bb);
            set_instr_number(succ_bb);
        }
    }
}

int CodeGen_Reg::get_value_reg_freg(Value *val)
{
    if (val->get_type()->is_integer_type() || val->get_type()->is_pointer_type())
    {
        if (IS_Const(val) || context.value_reg[val] == -1)
        {
            int reg_num = get_register(val);
            if (val != context.inst)
                load_to_greg(val, Reg::t(reg_num));
            return reg_num;
        }
        else
            return context.value_reg[val];
    }
    else if (val->get_type()->is_float_type())
    {
        if (IS_Const(val) || context.value_freg[val] == -1)
        {
            int freg_num = get_fregister(val);
            if (val != context.inst)
                load_to_freg(val, FReg::ft(freg_num));
            return freg_num;
        }
        else
            return context.value_freg[val];
    }
}

int CodeGen_Reg::get_fregister(Value *val)
{
    int i;
    for (i = 0; i <= 15; i++)
    {
        if (context.freg_map[i] == nullptr)
        {
            refresh_freg(i, val);
            return i;
        }
    }
    for (i = 0; i <= 15; i++)
    {
        if (instr_freg_operand.find(i) != instr_freg_operand.end())
            continue;
        else if (IS_Const(context.freg_map[i]))
        {
            refresh_freg(i, val);
            return i;
        }
    }
    for (i = 0; i <= 15; i++)
    {
        if (instr_freg_operand.find(i) != instr_freg_operand.end())
            continue;
        else if (IS_Instr(context.freg_map[i]) && static_cast<Instruction *>(context.freg_map[i])->max_num_ <= context.inst->max_num_)
        {
            refresh_freg(i, val);
            return i;
        }
        else if (IS_Arg(context.freg_map[i]) && static_cast<Argument *>(context.freg_map[i])->end_num_ <= context.inst->max_num_)
        {
            refresh_freg(i, val);
            return i;
        }
        else if (IS_Global(context.freg_map[i]) && static_cast<GlobalVariable *>(context.freg_map[i])->end_num_ <= context.inst->max_num_)
        {
            refresh_freg(i, val);
            return i;
        }
    }
    unsigned target_freg;
    int max = 0;
    for (i = 0; i <= 15; i++)
    {
        if (instr_freg_operand.find(i) != instr_freg_operand.end())
            continue;
        else if (IS_Instr(context.freg_map[i]) && static_cast<Instruction *>(context.freg_map[i])->max_num_ >= max)
        {
            target_freg = i;
            max = static_cast<Instruction *>(context.freg_map[i])->max_num_;
        }
        else if (IS_Arg(context.freg_map[i]) && static_cast<Argument *>(context.freg_map[i])->end_num_ >= max)
        {
            target_freg = i;
            max = static_cast<Argument *>(context.freg_map[i])->end_num_;
        }
        else if (IS_Global(context.freg_map[i]) && static_cast<GlobalVariable *>(context.freg_map[i])->end_num_ >= max)
        {
            target_freg = i;
            max = static_cast<GlobalVariable *>(context.freg_map[i])->end_num_;
        }
    }
    if (!(0 <= target_freg && target_freg <= 15))
        assert(false && "get_fregister error");
    refresh_freg(target_freg, val);
    return target_freg;
}

int CodeGen_Reg::get_register(Value *val)
{
    int i;
    for (i = 0; i <= 8; i++)
    {
        if (context.reg_map[i] == nullptr)
        {
            refresh_reg(i, val);
            return i;
        }
    }
    for (i = 0; i <= 8; i++)
    {
        if (instr_reg_operand.find(i) != instr_reg_operand.end())
            continue;
        else if (IS_Const(context.reg_map[i]))
        {
            refresh_reg(i, val);
            return i;
        }
    }
    for (i = 0; i <= 8; i++)
    {
        if (instr_reg_operand.find(i) != instr_reg_operand.end())
            continue;
        else if (IS_Instr(context.reg_map[i]) && static_cast<Instruction *>(context.reg_map[i])->max_num_ <= context.inst->max_num_)
        {
            refresh_reg(i, val);
            return i;
        }
        else if (IS_Arg(context.reg_map[i]) && static_cast<Argument *>(context.reg_map[i])->end_num_ <= context.inst->max_num_)
        {
            refresh_reg(i, val);
            return i;
        }
        else if (IS_Global(context.reg_map[i]) && static_cast<GlobalVariable *>(context.reg_map[i])->end_num_ <= context.inst->max_num_)
        {
            refresh_reg(i, val);
            return i;
        }
    }
    int target_reg;
    unsigned max = 0;
    for (i = 0; i <= 8; i++)
    {
        if (instr_reg_operand.find(i) != instr_reg_operand.end())
            continue;
        else if (IS_Instr(context.reg_map[i]) && static_cast<Instruction *>(context.reg_map[i])->max_num_ >= max)
        {
            target_reg = i;
            max = static_cast<Instruction *>(context.reg_map[i])->max_num_;
        }
        else if (IS_Arg(context.reg_map[i]) && static_cast<Argument *>(context.reg_map[i])->end_num_ >= max)
        {
            target_reg = i;
            max = static_cast<Argument *>(context.reg_map[i])->end_num_;
        }
        else if (IS_Global(context.reg_map[i]) && static_cast<GlobalVariable *>(context.reg_map[i])->end_num_ >= max)
        {
            target_reg = i;
            max = static_cast<GlobalVariable *>(context.reg_map[i])->end_num_;
        }
    }
    if (!(0 <= target_reg && target_reg <= 8))
        assert(false && "get_register error");
    refresh_reg(target_reg, val);
    return target_reg;
}

void CodeGen_Reg::refresh_reg(int num, Value *val)
{
    if (context.reg_map[num] != nullptr)
    {
        Value *old = context.reg_map[num];
        context.reg_map[num] = val;
        if (!IS_Const(val))
            context.value_reg[val] = num;
        if (IS_Instr(old))
        {
            auto bb_out = livevar_->get_bb_out(current_bb);
            store_from_greg(old, Reg::t(num));
            context.value_reg[old] = -1;
        }
        else if (!IS_Const(old))
        {
            context.value_reg[old] = -1;
        }
    }
    else
    {
        context.reg_map[num] = val;
        if (!IS_Const(val))
            context.value_reg[val] = num;
    }
    return;
}

void CodeGen_Reg::refresh_freg(int num, Value *val)
{
    if (context.freg_map[num] != nullptr)
    {
        Value *old = context.freg_map[num];
        context.freg_map[num] = val;
        if (!IS_Const(val))
            context.value_freg[val] = num;
        if (IS_Instr(old))
        {
            store_from_freg(old, FReg::ft(num));
            context.value_freg[old] = -1;
        }
        else if (!IS_Const(old))
        {
            context.value_freg[old] = -1;
        }
    }
    else
    {
        context.freg_map[num] = val;
        if (!IS_Const(val))
            context.value_freg[val] = num;
    }
    return;
}

void CodeGen_Reg::set_instr_max_min_num()
{
    for (auto &bb : context.bb_order)
    {
        for (auto &instr1 : bb->get_instructions())
        {
            auto instr = &instr1;
            instr->max_num_ = (instr->begin_num_ > instr->end_num_) ? instr->begin_num_ : instr->end_num_;
            instr->min_num_ = (instr->begin_num_ < instr->end_num_) ? instr->begin_num_ : instr->end_num_;
        }
    }
}

void CodeGen_Reg::Set_reg_freg_enter(BasicBlock *bb)
{
    append_inst("init reg freg enter : ", ASMInstruction::Comment);
    bool if_init = false;
    if (bb->get_pre_basic_blocks().size() > 1)
        if_init = true;
    else
    {
        if (context.current_bb_num != 0)
        {
            if (context.bb_order[context.current_bb_num - 1] != bb->get_pre_basic_blocks().front())
                if_init = true;
        }
    }
    if (if_init)
    {
        auto bb_in = livevar_->get_bb_in(bb);
        for (auto &it : context.reg_map)
        {
            if (it.second != nullptr)
                if (IS_Arg(it.second) || IS_Global(it.second) || IS_Instr(it.second))
                    if (bb_in.find(it.second) != bb_in.end())
                        load_to_greg(it.second, Reg::t(it.first));
        }
        for (auto &it : context.freg_map)
        {
            if (it.second != nullptr)
                if (IS_Arg(it.second) || IS_Global(it.second) || IS_Instr(it.second))
                    if (bb_in.find(it.second) != bb_in.end())
                        load_to_freg(it.second, FReg::ft(it.first));
        }
    }
    return;
}

void CodeGen_Reg::Set_livevar_out(BasicBlock *bb)
{
    append_inst("init live var out : ", ASMInstruction::Comment);
    bool if_init = false;
    if (bb->get_succ_basic_blocks().size() > 1)
        if_init = true;
    else
    {
        for (auto &succ_bb : bb->get_succ_basic_blocks())
        {
            if (succ_bb->get_pre_basic_blocks().size() > 1)
            {
                if_init = true;
                break;
            }
        }
    }
    if (if_init)
    {
        auto bb_out = livevar_->get_bb_out(bb);
        for (auto &it : context.reg_map)
        {
            if (bb_out.find(it.second) != bb_out.end() && IS_Instr(it.second))
                store_from_greg(it.second, Reg::t(it.first));
        }
        for (auto &it : context.freg_map)
        {
            if (bb_out.find(it.second) != bb_out.end() && IS_Instr(it.second))
                store_from_freg(it.second, FReg::ft(it.first));
        }
    }
    return;
}

void CodeGen_Reg::run()
{
    // 确保每个函数中基本块的名字都被设置好
    m->set_print_name();

    findloop_ = std::make_unique<FindLoop>(m);
    findloop_->run();
    livevar_ = std::make_unique<LiveVar>(m);
    livevar_->run();
  
    if (!m->get_global_variable().empty())
    {
        append_inst("Global variables", ASMInstruction::Comment);
        append_inst(".data", ASMInstruction::Atrribute);
        for (auto &global : m->get_global_variable())
        {
            auto size =
                global.get_type()->get_pointer_element_type()->get_size();
            append_inst(".globl", {global.get_name()},
                        ASMInstruction::Atrribute);
            append_inst(".type", {global.get_name(), "@object"},
                        ASMInstruction::Atrribute);
            append_inst(".size", {global.get_name(), std::to_string(size)},
                        ASMInstruction::Atrribute);
            append_inst(global.get_name(), ASMInstruction::Label);
            if (global.get_type()->get_pointer_element_type()->is_int32_type())
                append_inst(".word", {std::to_string(static_cast<ConstantInt *>(global.get_init())->get_value())});
            else if (global.get_type()->get_pointer_element_type()->is_float_type())
            {
                float val = static_cast<ConstantFP *>(global.get_init())->get_value();
                std::stringstream fp_val_ss;
                std::string fp_val;
                fp_val_ss << std::hex << *(uint32_t *)&val << std::endl;
                fp_val_ss >> fp_val;
                char *stop;
                int ans = strtol(fp_val.c_str(), &stop, 16);
                append_inst(".word", {std::to_string(ans)});
            }
            else if (global.get_type()->get_pointer_element_type()->is_array_type())
            {
                Type *init_type = global.get_type()->get_pointer_element_type();
                while (init_type->is_array_type())
                    init_type = static_cast<ArrayType *>(init_type)->get_element_type();
                Constant *init_val = global.get_init();
                gen_global_array_init(init_val, global.get_type()->get_pointer_element_type(), init_type);
            }
        }
    }

    // 函数代码段
    output.emplace_back(".text", ASMInstruction::Atrribute);

    context.init_reg_map();

    for (auto &func : m->get_functions())
    {
        if (not func.is_declaration())
        {
            // 更新 context
            context.clear();
            context.func = &func;
            context.init_value_reg();
            for (auto &global1 : m->get_global_variable())
            {
                auto global = &global1;
                if (global->get_type()->is_integer_type() || global->get_type()->is_pointer_type())
                    context.value_reg.insert({global, -1});
                else if (global->get_type()->is_float_type())
                    context.value_freg.insert({global, -1});
            }
            set_instr_number(context.func->get_entry_block());
            set_instr_max_min_num();
            // 函数信息
            append_inst(".globl", {func.get_name()}, ASMInstruction::Atrribute);
            append_inst(".type", {func.get_name(), "@function"},
                        ASMInstruction::Atrribute);
            append_inst(func.get_name(), ASMInstruction::Label);
            // 分配函数栈帧
            allocate();
            // 生成 prologue
            gen_prologue();
            int bb_num = context.bb_order.size();
            for (int i = 0; i < bb_num; i++)
            {
                auto bb = context.bb_order[i];
                context.current_bb_num = i;
                append_inst(label_name(bb), ASMInstruction::Label);
                current_bb = bb;
                Set_reg_freg_enter(bb);
                for (auto &instr : bb->get_instructions())
                {
                    // For debug
                    append_inst(instr.print() + " num:" + " " + std::to_string(instr.min_num_) + " " + std::to_string(instr.max_num_), ASMInstruction::Comment);
                    context.inst = &instr; // 更新 context
                    switch (instr.get_instr_type())
                    {
                    case Instruction::ret:
                        gen_ret();
                        break;
                    case Instruction::br:
                        gen_br();
                        break;
                    case Instruction::add:
                    case Instruction::sub:
                    case Instruction::mul:
                    case Instruction::sdiv:
                    case Instruction::srem:
                        gen_binary();
                        break;
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv:
                        gen_float_binary();
                        break;
                    case Instruction::alloca:
                        gen_alloca();
                        break;
                    case Instruction::load:
                        gen_load();
                        break;
                    case Instruction::store:
                        gen_store();
                        break;
                    case Instruction::ge:
                    case Instruction::gt:
                    case Instruction::le:
                    case Instruction::lt:
                    case Instruction::eq:
                    case Instruction::ne:
                        gen_icmp();
                        break;
                    case Instruction::fge:
                    case Instruction::fgt:
                    case Instruction::fle:
                    case Instruction::flt:
                    case Instruction::feq:
                    case Instruction::fne:
                        gen_fcmp();
                        break;
                    case Instruction::phi:
                        break;
                    case Instruction::call:
                        gen_call();
                        break;
                    case Instruction::getelementptr:
                        gen_gep();
                        break;
                    case Instruction::zext:
                        gen_zext();
                        break;
                    case Instruction::fptosi:
                        gen_fptosi();
                        break;
                    case Instruction::sitofp:
                        gen_sitofp();
                        break;
                    }
                }
            }
            // 生成 epilogue
            gen_epilogue();
        }
    }
}

std::string CodeGen_Reg::print() const
{
    std::string result;
    for (const auto &inst : output)
    {
        result += inst.format();
    }
    return result;
}
