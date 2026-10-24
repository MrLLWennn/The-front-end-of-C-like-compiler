#ifndef MY_COMPILER_COMPILATION_PATHS_HPP
#define MY_COMPILER_COMPILATION_PATHS_HPP

#include <array>
#include <filesystem>
#include <string>

namespace my_compiler
{

    // 用一个值描述本轮路径；默认值兼容旧探针与测试目录。
    struct CompilationPaths
    {
        std::string source_file = "../files/00-source.txt";
        std::string uncomment_file = "../files/01-uncomment.txt";
        std::string pretreatment_file = "../files/02-pretreatment.txt";
        std::string lex_file = "../files/03-lexer.txt";
        std::string grammar_file = "../files/04-grammar.txt";
        std::string LL1grammar_file = "../files/05-LL(1)_grammar.txt";
        std::string processed_grammar_file = "../files/06-processed_grammar.txt";
        std::string Vn_Vt_file = "../files/07-Vn_Vt.txt";
        std::string first_follow_file = "../files/08-first_follow.txt";
        std::string pred_table_file = "../files/09-predictive_analysis_table.txt";
        std::string stack_file = "../files/10-stack_file.txt";
        std::string tree_file = "../files/11-grammatical_analysis_tree.txt";
        std::string tree_dot_file = "../files/12-grammatical_analysis_tree.dot";
        std::string symbol_table_file = "../files/13-symbol_table.txt";
        std::string semantic_analysis_file = "../files/14-semantic_analysis.txt";

        std::string ast_file = "../files/15-ast.txt";
        std::string llvm_before_file = "../files/16-before.ll";
        std::string llvm_after_file = "../files/17-after.ll";
        std::string object_file = "../files/18-program.o";

        static CompilationPaths for_run(const std::filesystem::path& source,
            const std::filesystem::path& grammar,
            const std::filesystem::path& output)
        {
            CompilationPaths p;
            p.source_file = source.string();
            p.grammar_file = grammar.string();
            p.uncomment_file = (output / "01-uncomment.txt").string();
            p.pretreatment_file = (output / "02-pretreatment.txt").string();
            p.lex_file = (output / "03-lexer.txt").string();
            p.LL1grammar_file = (output / "05-LL(1)_grammar.txt").string();
            p.processed_grammar_file = (output / "06-processed_grammar.txt").string();
            p.Vn_Vt_file = (output / "07-Vn_Vt.txt").string();
            p.first_follow_file = (output / "08-first_follow.txt").string();
            p.pred_table_file = (output / "09-predictive_analysis_table.txt").string();
            p.stack_file = (output / "10-stack_file.txt").string();
            p.tree_file = (output / "11-grammatical_analysis_tree.txt").string();
            p.tree_dot_file = (output / "12-grammatical_analysis_tree.dot").string();
            p.symbol_table_file = (output / "13-symbol_table.txt").string();
            p.semantic_analysis_file = (output / "14-semantic_analysis.txt").string();
            p.ast_file = (output / "15-ast.txt").string();
            p.llvm_before_file = (output / "16-before.ll").string();
            p.llvm_after_file = (output / "17-after.ll").string();
            p.object_file = (output / "18-program.o").string();
            return p;
        }

        std::array<std::string, 17> output_files() const
        {
            return {uncomment_file, pretreatment_file, lex_file, LL1grammar_file,
                processed_grammar_file, Vn_Vt_file, first_follow_file,
                pred_table_file, stack_file, tree_file, tree_dot_file,
                symbol_table_file, semantic_analysis_file, ast_file, llvm_before_file,
                llvm_after_file, object_file};
        }
    };

    // 过渡适配：旧模块仍通过 header_files.h 的共享名字访问路径。
    // 必须在构造和运行前端前配置，运行中不能重新绑定。
    // 支持独立进程隔离，不支持同一进程内并发运行多个编译实例。
    inline CompilationPaths& legacy_compilation_paths()
    {
        static CompilationPaths paths;
        return paths;
    }

    inline void configure_compilation_paths(const CompilationPaths& paths)
    {
        legacy_compilation_paths() = paths;
    }

} // namespace my_compiler
#endif
