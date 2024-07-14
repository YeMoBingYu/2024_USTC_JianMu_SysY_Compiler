#pragma once

#include "ASMInstruction.hpp"
#include "Module.hpp"
#include "Register.hpp"
#include "FindLoop.hpp"
#include "LiveVar.hpp"
#include <map>
#include <unordered_map>

class CodeGen_Reg
{
public:
    explicit CodeGen_Reg(Module *module) : m(module) {}

    std::string print() const;

    void run();

    template <class... Args>
    void append_inst(Args... arg)
    {
        output.emplace_back(arg...);
    }

    void
    append_inst(const char *inst, std::initializer_list<std::string> args,
                ASMInstruction::InstType ty = ASMInstruction::Instruction)
    {
        auto content = std::string(inst) + " ";
        for (const auto &arg : args)
        {
            content += arg + ", ";
        }
        while (content.back() == ',' || content.back() == ' ')
            content.pop_back();
        output.emplace_back(content, ty);
    }

private:
    void allocate();

    // 向寄存器中装载数据
    void load_to_greg(Value *, const Reg &);
    void load_to_freg(Value *, const FReg &);
    void load_from_stack_to_greg(Value *, const Reg &);

    // 向寄存器中加载立即数
    void load_large_int32(int32_t, const Reg &);
    void load_large_int64(int64_t, const Reg &);
    void load_float_imm(float, const FReg &);

    // 将寄存器中的数据保存回栈上
    void store_from_greg(Value *, const Reg &);
    void store_from_freg(Value *, const FReg &);

    void gen_prologue();
    void gen_ret();
    void gen_br();
    void gen_binary();
    void gen_float_binary();
    void gen_alloca();
    void gen_load();
    void gen_store();
    void gen_icmp();
    void gen_fcmp();
    void gen_zext();
    void gen_call();
    void gen_gep();
    void gen_sitofp();
    void gen_fptosi();
    void gen_epilogue();
    void gen_phi();

    void gen_global_array_init(Constant *, Type *, Type *);

    void set_instr_number(BasicBlock *);   // 给每个instr编号，同时分析其起始活跃端点
    int get_register(Value *);             // 申请一个reg
    int get_fregister(Value *);            // 申请一个freg
    int get_value_reg_freg(Value *);       // 得到val对应的寄存器
    void refresh_reg(int, Value *);        // 更新reg_map
    void refresh_freg(int, Value *);       // 更新freg_map
    void Set_reg_freg_enter(BasicBlock *); // 初始化map对应的reg和freg
    void set_instr_max_min_num();          // 设置instr的开始结束端点
    void Set_livevar_out(BasicBlock *);

    static std::string label_name(BasicBlock *bb)
    {
        return "." + bb->get_parent()->get_name() + "_" + bb->get_name();
    }

    struct
    {
        Function *func{nullptr};    // 当前函数
        Instruction *inst{nullptr}; // 当前指令
        
        unsigned frame_size{0};                        // 当前函数的栈帧大小
        std::unordered_map<Value *, int> offset_map{}; // 指针相对 fp 的偏移

        std::map<int, Value *> reg_map{};    // 各reg的分配情况
        std::map<int, Value *> freg_map{};   // 各freg的分配情况
        std::map<Value *, int> value_reg{};  // value的分配tmp_reg
        std::map<Value *, int> value_freg{}; // value的分配tmp_freg
        std::vector<BasicBlock *> bb_order;  // 遍历bb的顺序
        std::set<BasicBlock *> visit;        // 已经访问过的bb

        unsigned num = 0;
        int current_bb_num;

        void init_reg_map() // 初始化map
        {
            for (int i = 0; i <= 8; i++)
                reg_map.insert({i, nullptr});
            for (int i = 0; i <= 15; i++)
                freg_map.insert({i, nullptr});
        }

        void init_value_reg() // 插入value
        {
            for (auto &arg1 : func->get_args())
            {
                auto arg = &arg1;
                if (arg->get_type()->is_integer_type() || arg->get_type()->is_pointer_type())
                    value_reg.insert({arg, -1});
                else if (arg->get_type()->is_float_type())
                    value_freg.insert({arg, -1});
            }
            for (auto &bb1 : func->get_basic_blocks())
            {
                auto bb = &bb1;
                for (auto &instr1 : bb->get_instructions())
                {
                    auto instr = &instr1;
                    if (!instr->is_void())
                    {
                        if (instr->get_type()->is_integer_type() || instr->get_type()->is_pointer_type())
                            value_reg.insert({instr, -1});
                        else if (instr->get_type()->is_float_type())
                            value_freg.insert({instr, -1});
                    }
                }
            }
        }

        void clear()
        {
            func = nullptr;
            inst = nullptr;
            num = 0;
            frame_size = 0;
            offset_map.clear();
            value_reg.clear();
            value_freg.clear();
            visit.clear();
            bb_order.clear();
            for (int i = 0; i <= 8; i++)
                reg_map[i] = nullptr;
            for (int i = 0; i <= 15; i++)
                freg_map[i] = nullptr;
        }

    } context;

    std::unique_ptr<FindLoop> findloop_;
    std::unique_ptr<LiveVar> livevar_;
    Module *m;
    std::list<ASMInstruction> output;
};
