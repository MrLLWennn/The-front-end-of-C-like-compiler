//
// Created by 86198 on 2024/11/4.
//

#ifndef MY_COMPILER_FRONT_END_H
#define MY_COMPILER_FRONT_END_H


#include"lexer/lexer.h"
#include"parser/parser.h"
#include"../SymbolTable/SymbolTable.h"
#include"../ErrorHandler/ErrorTable.h"



namespace my_compiler
{

    using namespace my_compiler_symbol_table;
    using namespace my_compiler_tree;


    //生成的临时字符串的序号
    static int seq = 0;
    static int cur_num = 0;


    //编译器的前端
    class FrontEnd
    {
        using Node = GrammaticalAnalysisTreeNode;
    public:
        FrontEnd()
        {
            FunctionSymbol func1(Category::FUNC, "input", "-");
            func1.set_property("default global function");
            func1.set_address(&func1);

            FunctionSymbol func2(Category::FUNC, "output", "-");
            func2.set_property("default global function");
            func2.set_address(&func2);

            symbol_table_.emplace_function(func1);
            symbol_table_.emplace_function(func2);
        }

    public:

        void front_end_compiling()
        {
            front_end_compiling_aux();
        }

        void generate_symbol_table()
        {
            input_file(file_in_, lex_file);
            output_file(file_out_, symbol_table_file);
            generate_symbol_table_aux();
            file_in_.close();
            file_out_.close();
        }



    private:

        void _front_end_compiling_aux();

        void front_end_compiling_aux();

        //符号表生成
        void generate_symbol_table_aux();

        //语义分析
        void semantic_analysis_aux();

        //中间代码生成
        void generate_intermediate_code_aux();

    private:

        static string temp_step()
        {
            string o = "STEP";
            std::stringstream ss;
            ss << seq++;
            o.append(ss.str());
            return o;
        }

        bool action1(const shared_ptr<Node>& node)
        {
            //1 <S> -> <program>
            return true;
        }

        bool action2(const shared_ptr<Node>& node)
        {
            //2 <program> -> <declaration-list>
            return true;
        }

        bool action3(const shared_ptr<Node>& node)
        {
            //3 <declaration-list> -> <declaration> <declaration-list>'
            return true;
        }

        //map<string, bool(*)(tree bit *cur)> func map;		//推导式编号与其对应的语义动作函数
        //map<int, string> mid code map;
        //map<string, int> func num;
        //int cur num = 99;		//三地址代码编号
        //int cur temp = 0;		//临时变量个数

        bool action4(const shared_ptr<Node>& node)
        {
            //4 <declaration-list>' -> <declaration> <declaration-list>'
            return true;
        }

        bool action5(const shared_ptr<Node>& node)
        {
            //5 <declaration-list>' -> empty
            return true;
        }

        bool action6(const shared_ptr<Node>& node)
        {
            //6 <declaration> -> <var-declaration>
            return true;
        }

        bool action7(const shared_ptr<Node>& node)
        {
            //7 <declaration> -> <const-var-declaration>
            return true;
        }

        bool action8(const shared_ptr<Node>& node)
        {
            //8 <declaration> -> <func-declaration>
            return true;
        }

        bool action9(const shared_ptr<Node>& node)
        {
            //9 <declaration> -> <struct-declaration>
            return true;
        }

        bool action10(const shared_ptr<Node>& node)
        {
            //10 <var-declaration> -> <type-specifier> identifier <var-declaration>'
            return true;
        }

        bool action11(const shared_ptr<Node>& node)
        {
            //11 <var-declaration>' -> ;
            return true;
        }

        bool action12(const shared_ptr<Node>& node)
        {
            //12 <var-declaration>' -> [ number ] ;
            return true;
        }

        bool action13(const shared_ptr<Node>& node)
        {
            //13 <const-var-declaration> -> const <type-specifier> identifier <const-var-declaration>'
            return true;
        }

        bool action14(const shared_ptr<Node>& node)
        {
            //14 <const-var-declaration>' -> ;
            return true;
        }


        bool action15(const shared_ptr<Node>& node)
        {
            //15 <const-var-declaration>' -> [ number ] ;
            return true;
        }

        bool action16(const shared_ptr<Node>& node)
        {
            //16 <type-specifier> -> <const-type>
            return true;
        }

        bool action17(const shared_ptr<Node>& node)
        {
            //17 <type-specifier> -> <type>
            return true;
        }

        bool action18(const shared_ptr<Node>& node)
        {
            //18 <const-type> -> const <const-type>'
            return true;
        }

        bool action19(const shared_ptr<Node>& node)
        {
            //19 <const-type>' -> int
            return true;
        }

        bool action20(const shared_ptr<Node>& node)
        {
            //20 <const-type>' -> double
            return true;
        }

        bool action21(const shared_ptr<Node>& node)
        {
            //21 <const-type>' -> float
            return true;
        }

        bool action22(const shared_ptr<Node>& node)
        {
            //22 <type> -> int
            return true;
        }

        bool action23(const shared_ptr<Node>& node)
        {
            //23 <type> -> float
            return true;
        }

        bool action24(const shared_ptr<Node>& node)
        {
            //24 <type> -> double
            return true;
        }

        bool action25(const shared_ptr<Node>& node)
        {
            //25 <type> -> void
            return true;
        }

        bool action26(const shared_ptr<Node>& node)
        {
            //26 <type> -> bool
            return true;
        }

        bool action27(shared_ptr<Node> node)
        {
            //27 <func-declaration> -> action27_1 <type-specifier> identifier-function action27_2 ( <params> ) <func-declaration>'
            node = node->parent_;
            //
            string procedure = "define internal ";
            // <func-declaration> -> <type-specifier> -> <type> -> 函数的类型
            procedure.append(node->child_[0]->child_[0]->child_[0]->value_);

            intermediate_code_.insert({++cur_num, procedure});
            return true;
        }

        bool action28(const shared_ptr<Node>& node)
        {
            //28 <func-declaration>' -> ;
            return true;
        }

        bool action29(const shared_ptr<Node>& node)
        {
            //29 <func-declaration>' -> <compound-stmt>
            return true;
        }

        bool action30(shared_ptr<Node> node)
        {
            //30 <struct-declaration> -> struct identifier-struct <struct-declaration>'
            node = node->parent_;
            intermediate_code_.insert({++cur_num, "Struct" + node->child_[1]->value_});
            return true;
        }

        bool action31(const shared_ptr<Node>& node)
        {
            //31 <struct-declaration>' -> <struct-stmt> ;
            return true;
        }

        bool action32(const shared_ptr<Node>& node)
        {
            //32 <struct-declaration>' -> identifier [ number ] ;
            return true;
        }

        bool action33(const shared_ptr<Node>& node)
        {
            //33 <params> -> <params-list>
            return true;
        }

        bool action34(const shared_ptr<Node>& node)
        {
            //34 <params> -> void
            return true;
        }

        bool action35(const shared_ptr<Node>& node)
        {
            //35 <params> -> empty
            return true;
        }

        bool action36(const shared_ptr<Node>& node)
        {
            //36 <params-list> -> <param> <params-list>'
            return true;
        }

        bool action37(const shared_ptr<Node>& node)
        {
            //37 <params-list>' -> , <param> <params-list>'
            return true;
        }

        bool action38(const shared_ptr<Node>& node)
        {
            //38 <params-list>' -> empty
            return true;
        }

        bool action39(const shared_ptr<Node>& node)
        {
            //39 <param> -> <type-specifier> identifier
            return true;
        }

        bool action40(const shared_ptr<Node>& node)
        {
            //40 <param> -> <type-specifier> identifier [ ]
            return true;
        }

        bool action41(const shared_ptr<Node>& node)
        {
            //41 <param> -> const <type-specifier> identifier
            return true;
        }

        bool action42(const shared_ptr<Node>& node)
        {
            //42 <param> -> const <type-specifier> identifier [ ]
            return true;
        }

        bool action43(const shared_ptr<Node>& node)
        {
            //43 <compound-stmt> -> { <local-declarations> <statement-list> }
            return true;
        }

        bool action44(const shared_ptr<Node>& node)
        {
            //44 <struct-stmt> -> { <declaration-list> }
            return true;
        }

        bool action45(const shared_ptr<Node>& node)
        {
            //45 <local-declarations> -> empty
            return true;
        }

        bool action46(const shared_ptr<Node>& node)
        {
            //46 <local-declarations> -> <local-declarations>'
            return true;
        }

        bool action47(const shared_ptr<Node>& node)
        {
            //47 <local-declarations>' -> <var-declaration> <local-declarations>'
            return true;
        }

        bool action48(const shared_ptr<Node>& node)
        {
            //48 <local-declarations>' -> empty
            return true;
        }

        bool action49(const shared_ptr<Node>& node)
        {
            //49 <statement-list> -> empty
            return true;
        }

        bool action50(const shared_ptr<Node>& node)
        {
            //50 <statement-list> -> <statement-list>'
            return true;
        }

        bool action51(const shared_ptr<Node>& node)
        {
            //51 <statement-list>' -> <statement> <statement-list>'
            return true;
        }

        bool action52(const shared_ptr<Node>& node)
        {
            //52 <statement-list>' -> empty
            return true;
        }

        bool action53(const shared_ptr<Node>& node)
        {
            //53 <statement> -> <expression-stmt>
            return true;
        }

        bool action54(const shared_ptr<Node>& node)
        {
            ///54 <statement> -> <compound-stmt>
            return true;
        }

        bool action55(const shared_ptr<Node>& node)
        {
            //55 <statement> -> <selection-stmt>
            return true;
        }

        bool action56(const shared_ptr<Node>& node)
        {
            //56 <statement> -> <while-stmt>
            return true;
        }

        bool action57(const shared_ptr<Node>& node)
        {
            //57 <statement> -> <for-stmt>
            return true;
        }

        bool action58(const shared_ptr<Node>& node)
        {
            //58 <statement> -> <jump-stmt>
            return true;
        }

        bool action59(const shared_ptr<Node>& node)
        {
            //59 <statement> -> <break-stmt>
            return true;
        }

        bool action60(const shared_ptr<Node>& node)
        {
            //60 <statement> -> <continue-stmt>
            return true;
        }

        bool action61(const shared_ptr<Node>& node)
        {
            //61 <statement> -> <switch-case-default-stmt>
            return true;
        }

        bool action62(const shared_ptr<Node>& node)
        {
            //62 <expression-stmt> -> ;
            return true;
        }

        bool action63(const shared_ptr<Node>& node)
        {
            //63 <expression-stmt> -> <expression> ;
            return true;
        }

        bool action64_1(shared_ptr<Node> node)
        {
            //64 <selection-stmt> -> if ( <expression> action64_1 ) <statement> action64_2 <selection-stmt>' action64_3
            node = node->parent_;

            return true;
        }
//
//        bool action31_1(const shared_ptr<Node>& node);
//
//        bool action31_2(const shared_ptr<Node>& node);
//
//        bool action32(const shared_ptr<Node>& node);
//
//        bool action33(const shared_ptr<Node>& node);
//
//        bool action34(const shared_ptr<Node>& node);
//
//        bool action35_1(const shared_ptr<Node>& node);
//
//        bool action35_2(const shared_ptr<Node>& node);
//
//        bool action36(const shared_ptr<Node>& node);
//
//        bool action37(const shared_ptr<Node>& node);
//
//        bool action38(const shared_ptr<Node>& node);
//
//        bool action39(const shared_ptr<Node>& node);
//
//        bool action40_1(const shared_ptr<Node>& node);
//
//        bool action40_2(const shared_ptr<Node>& node);
//
//        bool action41(const shared_ptr<Node>& node);
//
//        bool action42(const shared_ptr<Node>& node);
//
//        bool action43_1(const shared_ptr<Node>& node);
//
//        bool action43_2(const shared_ptr<Node>& node);
//
//        bool action44_1(const shared_ptr<Node>& node);
//
//        bool action44_2(const shared_ptr<Node>& node);
//
//        bool action44_3(const shared_ptr<Node>& node);
//
//        bool action45_1(const shared_ptr<Node>& node);
//
//        bool action45_2(const shared_ptr<Node>& node);
//
//        bool action46_1(const shared_ptr<Node>& node);
//
//        bool action46_2(const shared_ptr<Node>& node);
//
//        bool action46_3(const shared_ptr<Node>& node);
//
//        bool action46_4(const shared_ptr<Node>& node);
//
//        bool action47(const shared_ptr<Node>& node);
//
//        bool action48_1(const shared_ptr<Node>& node);
//
//        bool action48_2(const shared_ptr<Node>& node);
//
//        bool action49(const shared_ptr<Node>& node);
//
//        bool action50_1(const shared_ptr<Node>& node);
//
//        bool action50_2(const shared_ptr<Node>& node);
//
//        bool action51(const shared_ptr<Node>& node);
//
//        bool action52_1(const shared_ptr<Node>& node);
//
//        bool action52_2(const shared_ptr<Node>& node);
//
//        bool action53_1(const shared_ptr<Node>& node);
//
//        bool action53_2(const shared_ptr<Node>& node);
//
//        bool action53_3(const shared_ptr<Node>& node);
//
//        bool action53_4(const shared_ptr<Node>& node);
//
//        bool action53_5(const shared_ptr<Node>& node);
//
//        bool action53_6(const shared_ptr<Node>& node);
//
//        bool action54(const shared_ptr<Node>& node);
//
//        bool action55_1(const shared_ptr<Node>& node);
//
//        bool action55_2(const shared_ptr<Node>& node);
//
//        bool action56(const shared_ptr<Node>& node);
//
//        bool action57_1(const shared_ptr<Node>& node);
//
//        bool action57_2(const shared_ptr<Node>& node);
//
//        bool action158_1(const shared_ptr<Node>& node);
//
//        bool action58_2(const shared_ptr<Node>& node);
//
//        bool action59_1(const shared_ptr<Node>& node);
//
//        bool action59_2(const shared_ptr<Node>& node);
//
//        bool action59_3(const shared_ptr<Node>& node);
//
//        bool action59_4(const shared_ptr<Node>& node);
//
//        bool action59_5(const shared_ptr<Node>& node);
//
//        bool action60(const shared_ptr<Node>& node);
//
//        bool action61_1(const shared_ptr<Node>& node);
//
//        bool action61_2(const shared_ptr<Node>& node);
//
//        bool action62(const shared_ptr<Node>& node);
//
//        bool action63_1(const shared_ptr<Node>& node);
//
//        bool action63_2(const shared_ptr<Node>& node);





    private:
        //文件对象
        fstream file_in_;
        fstream file_out_;
        //词法分析器
        Lexer lexer_;
        vector<Token> tokens_;
        //语法分析器
        Parser parser_;
        //local作用域 变量符号表
        SymbolTable symbol_table_;
        //中间表示代码
        map<int, string> intermediate_code_;

    };


}



#endif //MY_COMPILER_FRONT_END_H
