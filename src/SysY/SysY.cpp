#include "CodeGen.hpp"
#include "RegAlloca.hpp"
#include "DeadCode.hpp"
#include "Mem2Reg.hpp"
#include "FindLoop.hpp"
#include "LoopHoist.hpp"
#include "ConstantSpread.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "SysY_Builder.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using std::string;
using std::operator""s;

struct Config
{
    string exe_name; // compiler exe name
    std::filesystem::path input_file;
    std::filesystem::path output_file;

    bool emitllvm{false};
    bool emitasm{false};
    bool regalloca{false};
    bool irhoist{false};

    Config(int argc, char **argv) : argc(argc), argv(argv)
    {
        parse_cmd_line();
        check();
    }

private:
    int argc{-1};
    char **argv{nullptr};

    void parse_cmd_line();
    void check();
    // print helper infomation and exit
    void print_help() const;
    void print_err(const string &msg) const;
};

int main(int argc, char **argv)
{
    Config config(argc, argv);

    std::unique_ptr<Module> m;
    {
        auto syntax_tree = parse(config.input_file.c_str());
        auto ast = AST(syntax_tree);
        SysYBuilder builder;
        ast.run_visitor(builder);
        m = builder.getModule();
    }

    PassManager PM(m.get());

    if (config.irhoist)
    {
        PM.add_pass<Mem2Reg>();        // SSA
        PM.add_pass<DeadCode>();       // 死代码
        PM.add_pass<ConstantSpread>(); // 常量传播
        PM.add_pass<LoopHoist>();      // 循环不变量外提
    }
    PM.run();

    std::ofstream output_stream(config.output_file);
    if (config.emitllvm)
    {
        auto abs_path = std::filesystem::canonical(config.input_file);
        output_stream << "; ModuleID = 'sy'\n";
        output_stream << "source_filename = " << abs_path << "\n\n";
        output_stream << m->print();
    }
    else if (config.emitasm)
    {
        if (config.regalloca)
        {
            CodeGen_Reg codegen_reg(m.get());
            codegen_reg.run();
            output_stream << codegen_reg.print();
        }
        else
        {
            CodeGen codegen(m.get());
            codegen.run();
            output_stream << codegen.print();
        }
    }

    return 0;
}

void Config::parse_cmd_line()
{
    exe_name = argv[0];
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == "-h"s || argv[i] == "--help"s)
        {
            print_help();
        }
        else if (argv[i] == "-o"s)
        {
            if (output_file.empty() && i + 1 < argc)
            {
                output_file = argv[i + 1];
                i += 1;
            }
            else
            {
                print_err("bad output file");
            }
        }
        else if (argv[i] == "-emit-llvm"s)
        {
            emitllvm = true;
        }
        else if (argv[i] == "-S"s)
        {
            emitasm = true;
        }
        else if (argv[i] == "-o1"s)
        {
            irhoist = true;
            regalloca=false;
        }
        else if (argv[i] == "-o2"s)
        {
            irhoist = true;
            regalloca = true;
        }
        else
        {
            if (input_file.empty())
            {
                input_file = argv[i];
            }
            else
            {
                string err =
                    "unrecognized command-line option \'"s + argv[i] + "\'"s;
                print_err(err);
            }
        }
    }
}

void Config::check()
{
    if (input_file.empty())
    {
        print_err("no input file");
    }
    if (input_file.extension() != ".sy")
    {
        print_err("file format not recognized");
    }
    if (emitllvm and emitasm)
    {
        print_err("emit llvm and emit asm both set");
    }
    if (not emitllvm and not emitasm)
    {
        print_err("not supported: generate executable file directly");
    }
    if (output_file.empty())
    {
        output_file = input_file.stem();
        if (emitllvm)
        {
            output_file.replace_extension(".ll");
        }
        else if (emitasm)
        {
            output_file.replace_extension(".s");
        }
    }
}

void Config::print_help() const
{
    std::cout << "Usage: " << exe_name
              << " [-h|--help] [-o <target-file>] [-mem2reg] [-emit-llvm] [-S] "
                 "<input-file>"
              << std::endl;
    exit(0);
}

void Config::print_err(const string &msg) const
{
    std::cout << exe_name << ": " << msg << std::endl;
    exit(-1);
}
