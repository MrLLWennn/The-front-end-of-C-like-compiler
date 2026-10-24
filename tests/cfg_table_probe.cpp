#include "front_end/parser/context_free_grammar.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
    try
    {
        const bool raw_grammar = argc > 1 && std::string(argv[1]) == "--raw";
        const int row_argument = raw_grammar ? 2 : 1;
        if(argc < row_argument + 2)
            throw std::invalid_argument("usage: cfg_table_probe [--raw] NONTERMINAL LOOKAHEAD...");
        CFG::ContextFreeGrammar grammar;
        if(raw_grammar)
        {
            // 原有夹具从06进入；此模式覆盖04到05/06的真实文法预处理。
            grammar.remove_left_recursion_and_factor();
            grammar.split_grammar();
        }
        grammar.cal_Vn_Vt();
        grammar.cal_first_and_follow();
        grammar.generate_predictive_analysis_table();
        // 两个集合必须互斥；避免误把派生非终结符初始化为终结符 FIRST。
        const auto& vocabulary = grammar.predictive_analysis_table_;
        for(const auto& symbol : vocabulary.Vn_)
        {
            if(std::find(vocabulary.Vt_.begin(), vocabulary.Vt_.end(), symbol) != vocabulary.Vt_.end())
                throw std::logic_error("symbol is both terminal and nonterminal: " + symbol);
        }
        const std::string row_name = argv[row_argument];
        const auto& table = grammar.predictive_analysis_table_.table_;
        const auto row = table.find(row_name);
        for(int i = row_argument + 1; i < argc; ++i)
        {
            const std::string terminal = argv[i];
            bool emitted = false;
            if(row != table.end())
            {
                const auto cell = row->second.find(terminal);
                if(cell != row->second.end())
                {
                    for(const auto& production : cell->second)
                    {
                        std::cout << "TABLE_ENTRY\t" << row_name << '\t' << terminal
                            << '\t' << production << '\n';
                        emitted = true;
                    }
                }
            }
            if(!emitted)
                std::cout << "TABLE_EMPTY\t" << row_name << '\t' << terminal << '\n';
        }
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
