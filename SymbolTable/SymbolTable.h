//
// Created by 86198 on 2024/10/30.
//

#ifndef MY_COMPILER_SYMBOLTABLE_H
#define MY_COMPILER_SYMBOLTABLE_H


#include <utility>

#include"../header_files.h"
#include"../token/token_header.h"
#include"../ErrorHandler/ErrorTable.h"


//此头文件用于 编译过程的 符号表
/*
 * 符号表
 *
 * 1. 符号表是编译器其中一种重要的数据结构，程序中所使用的标识符的类型，作用域及其存储位置都保存在其中。
 * 2. 符号表是在编译器初期就已经建立。它根据语言规则识别标识符的正确使用，以便执行更高效的代码生成和错误检测，它具有内置的词法和语法分析阶段
 * 3.
 *      * 在分析阶段收集到的信息在语法分析阶段被编译器用来生成（中间）代码。
 *      * 被编译器用来实现编译期效率
 *      * 在如下多个阶段中使用：
 *          * 词法分析：在表中创建新的表条目，例如，token 的条目
 *          * 语法分析：根据 attribute type，作用域，维度，引用行，用途等信息
 *          * 语义分析：使用表中的可用信息检查语义，即验证表达式和赋值在语义上是否正确（类型检查）并相应地更新它
 *          * 中间代码生成：参考符号表，用于了解分配了多少和那种类型的运行时，并且有助于增添临时变量信息
 *          * 代码优化：使用表中的信息进行与计算机相关的优化
 *          * 目标代码生成：使用表中存在的标识符的地址信息生成代码
 * 4. 符号表条目——符号表中的每个条目都与在不同阶段的支持编译器的属性相关联
 * 5. 存储在符号表中的信息
 *      * 变量名称和常量
 *      * 过程和函数名称
 *      * 文本常量和字符串
 *      * 编译器生成的临时变量
 *      * 源语言中的标签
 * 6. 符号表的操作
 *      * allocate()        : allocate a new symbol table
 *      * free()            : remove all entries and free storage of symbol table
 *      * lookup()          : search for a name and return pointer to its entry
 *      * insert()          : insert a name in a symbol table and return a pointer to its entry
 *      * set_attribute()   : associate an attribute with a given entry
 *      * get_attribute()   : get an attribute associated with a given entry
 * 7. 谁来创建符号表的条目
 *      符号表条目是在分析阶段的词法分析器，语法分析器和语义分析器创建并使用的。我们采用 语法分析器 来创建的方法。
 * 因为语法分析器知道一个程序的语法结构，所以相对于词法分析器来说，语法分析器更适合来创建这些条目。它更好地区分一个标识符的不同声明。
 *      在更多情况下，词法分析器只能像语法分析器返回一个词法单元，比如 identifier ，以及指向这和此法单元的指针。
 * 只有语法分析器才能够决定是使用之前已经创建的符号表条目，还是为这个标识符创建一个条目
 * 8. 块的符号表的优化
 *      块的符号表的实现可以利用 作用域的最近嵌套规则。嵌套的结构可以保证可应用的符号表形成一个栈。
 * 在栈的顶部是当前块的顶部是一个当前快的符号表。栈中这个表的下方是包含这个块的各个块的符号表。
 * 因此，符号表可以按照类似于 栈 的方式来分配和释放
 *      有些编译器维护了一个散列表来存放可访问的符号表条目。也就是说，存放那些没有被内嵌块中的某个声明掩盖起来的条目。
 * 这样的散列表的实际上支持常量时间的查询，但是在进入和离开块需要插入和删除相应的条目。在从一个块 B离开时，编译器必须撤销所有因为 B 中的声明而对此散列表做出的修改。
 * 它可以在处理 B 的时候维护一个辅助栈来跟踪对这个散列表的所做的修改。
 *      语句块的 最近嵌套 规则：一个标识符的 x 在最近的 x 声明的作用域中。也就是说，从 x 出现的块开始，从内向外检查各个块时找到的第一个对 x 的声明。
 * */



namespace my_compiler_symbol_table
{



    /*为简化起见，实现的编译器不具有 静态存储区*/

    //分别对 function 和 struct 以及 is global 做区分实现符号表

    using namespace my_compiler_error_handler;

    class SymbolTable;
    class FunctionSymbol;
    class StructSymbol;

    enum Category
    {
        NIL,    // 空
        ARRAY,  //数组
        VAR,    //变量
        FUNC,   //函数
        CONST_VAR, //常变量
        CONST_FUNC, //常函数
        CONST_ARRAY,
        STRUCT
    };



    class Symbol
    {
        friend class SymbolTable;
        friend class FunctionSymbol;
        friend class StructSymbol;
        static size_t symbol_count_;
    protected:
        //符号属性
        enum Category category_;
        //符号名
        string name_;
        //符号类型
        string type_;
        string property_;
        //符号地址
        Symbol* address_;
        //表征层级
        size_t level_;
        //是否为 形参
        bool is_formal_;

    public:

        Symbol() = default;

        Symbol(string name, string type) :
        category_(Category::VAR), name_(std::move(name)), type_(std::move(type)), address_(nullptr), level_(0), is_formal_(false)
        {
            symbol_count_++;
        }


        Symbol(Category cat,string name, string type, bool formal) :
        category_(cat), name_(std::move(name)), type_(std::move(type)), address_(nullptr), level_(0), is_formal_(formal)
        {
            symbol_count_++;
            if(!is_formal_)
            {
                switch(category_)
                {
                    case ARRAY:
                        property_ = "variable array";
                        break;
                    case VAR:
                        property_ = "variable";
                        break;
                    case FUNC:
                        property_ = "function";
                        break;
                    case CONST_FUNC:
                        property_ = "const function";
                        break;
                    case CONST_VAR:
                        property_ = "const variable";
                        break;
                    case CONST_ARRAY:
                        property_ = "const variable array";
                        break;
                    case STRUCT:
                        property_ = "struct declaration";
                        break;
                    default:
                        property_ = "NULL";
                        break;
                }
            }
            else
            {
                switch(category_)
                {
                    case ARRAY:
                        property_ = "formal variable array";
                        break;
                    case VAR:
                        property_ = "formal variable";
                        break;
                    case CONST_VAR:
                        property_ = "formal const variable";
                        break;
                    case CONST_ARRAY:
                        property_ = "formal const variable array";
                        break;
                    default:
                        property_ = "NULL";
                        break;
                }
            }

        }


        Symbol(Category cat,string name, string type, size_t level, bool formal) :
        category_(cat), name_(std::move(name)), type_(std::move(type)), address_(nullptr), level_(level), is_formal_(formal)
        {
            symbol_count_++;
            if(!is_formal_)
            {
                switch(category_)
                {
                    case ARRAY:
                        property_ = "variable array";
                        break;
                    case VAR:
                        property_ = "variable";
                        break;
                    case FUNC:
                        property_ = "function";
                        break;
                    case CONST_FUNC:
                        property_ = "const function";
                        break;
                    case CONST_VAR:
                        property_ = "const variable";
                        break;
                    case CONST_ARRAY:
                        property_ = "const variable array";
                        break;
                    case STRUCT:
                        property_ = "struct declaration";
                        break;
                    default:
                        property_ = "NULL";
                        break;
                }
            }
            else
            {
                switch(category_)
                {
                    case ARRAY:
                        property_ = "formal variable array";
                        break;
                    case VAR:
                        property_ = "formal variable";
                        break;
                    case CONST_VAR:
                        property_ = "formal const variable";
                        break;
                    case CONST_ARRAY:
                        property_ = "formal const variable array";
                        break;
                    default:
                        property_ = "NULL";
                        break;
                }
            }

        }

        void set_address(Symbol* address)
        {
            address_ = address;
        }


        void set(Category cat, const string& name, const string& type, size_t level, bool formal);

        void set_property(const string& property)
        {
            property_ = property;
        }


        void print_symbol() const
        {
            cout << "符号" << symbol_count_ << ": " << name_ << "(" << type_ << ")";
        }

        void display_symbol(fstream& out) const
        {
            out << "            " << std::setiosflags(ios::left)
                              << std::setw(20) << name_
                              << std::setw(40) << property_
                              << std::setw(20) << type_
                              << std::setw(20) << address_ << endl;
        }



    };


    class FunctionSymbol : public Symbol
    {
        friend class SymbolTable;
    protected:
        //形参
        vector<Symbol> parameters_;
        //函数局部符号
        vector<Symbol> local_symbols_;
        //函数调用
        vector<string> function_call_;


    public:


        FunctionSymbol(Category cat, string name, string type) : Symbol(cat, std::move(name), std::move(type), false)
        {
        }

        FunctionSymbol() = default;


    public:

        [[nodiscard]] bool parameter_exists(const string& name) const;

        [[nodiscard]] bool local_symbol_exists(const string& name) const;

        void emplace_parameter(const Symbol& para);

        void emplace_local_symbol(const Symbol& local);

        void emplace_function_call(const string& name);

        [[nodiscard]] size_t parameters_size() const
        {
            return parameters_.size();
        }

        string get_function_name()
        {
            return name_;
        }


        void print_function() const
        {
            cout << "函数 （" << name_ << "），参数列表 [";
            for(const auto& p :parameters_)
            {
                cout << p.name_ << "(" << p.type_ << ") ";
            }
            cout << "]，函数局部变量：";
            if(local_symbols_.empty())
            {
                cout << "空" << endl;
            }
            else
            {
                for(const auto& p : local_symbols_)
                {
                    p.print_symbol();
                    cout << " ";
                }
                cout << endl;
            }
            cout << "函数调用的函数名：";
            for(const auto& f : function_call_)
            {
                cout << f << " ";
            }
            cout << endl;
        }





    };


    class StructSymbol : public Symbol
    {
        friend class SymbolTable;
    protected:
        //结构体内函数
        vector<FunctionSymbol> struct_functions_;
        //结构体内变量
        vector<Symbol> struct_symbols_;

    public:


        StructSymbol(Category cat,string name, string type, size_t level, bool formal)
        : Symbol(cat, std::move(name), std::move(type), level, formal)
        {

        }


        FunctionSymbol& get_struct_function(const string& name);

        void emplace_struct_function(const FunctionSymbol& func);

        void emplace_struct_symbols(const Symbol& sym);

        [[nodiscard]] bool symbol_exists(const string& name) const ;

        [[nodiscard]] bool function_exists(const string& name) const ;

        string get_struct_name()
        {
            return name_;
        }

        void print_struct() const
        {
            cout << "struct 内的成员变量: ";
            if(struct_symbols_.empty())
            {
                cout << "空" << endl;
            }
            else
            {
                for(const auto& s : struct_symbols_)
                {
                    s.print_symbol();
                    cout << endl;
                }
            }
            cout << "struct 内的函数: ";
            if(struct_functions_.empty())
            {
                cout << "空" << endl;
            }
            else
            {
                for(const auto& f : struct_functions_)
                {
                    f.print_function();
                }
            }
            cout << endl;
        }

    };



    class SymbolTable
    {
    private:
        //表名
        string name_;
        //全局函数
        vector<FunctionSymbol> functions_;
        vector<StructSymbol> structs_;
        //全局变量
        vector<Symbol> global_symbols_;
        //临时变量
        std::map<string, Symbol> for_symbols_;



    public:


        SymbolTable() = default;

        void emplace_function(const FunctionSymbol& func);

        [[nodiscard]] bool function_exists(const string& name) const;

        FunctionSymbol& get_function(const string& name);

        StructSymbol& get_struct(const string& name);

        void print_global_functions() const;

        void emplace_struct(const StructSymbol& st);

        void emplace_global_symbol(const Symbol& sym);

        void emplace_circulation_symbol(const string& func_name, const Symbol& sym);

        //void emplace_function_call(const FunctionSymbol& name1, const FunctionSymbol& name2);

        [[nodiscard]] bool global_symbol_exists(const string& name) const;

        [[nodiscard]] bool local_symbol_exists(const string &func_name, const string& symbol_name) const;

        [[nodiscard]] bool function_parameter_exists(const string &func_name, const string& symbol_name) const;

        [[nodiscard]] bool struct_symbol_exists(const string &struct_name, const string& symbol_name) const;

        [[nodiscard]] bool struct_function_exists(const string &struct_name, const string& symbol_name) const;

        [[nodiscard]] bool for_symbol_exists(const string &func_name, const string& symbol_name) const;


        void print_global_symbols() const
        {
            for(const auto& s : global_symbols_)
            {
                s.print_symbol();
                cout << endl;
            }
        }

        void print_global_struct() const
        {
            for(const auto& s : structs_)
            {
                s.print_struct();
            }
        }

        void print_for_circulation() const
        {
            for(const auto& f_s : for_symbols_)
            {
                cout << "函数" << f_s.first << " for循环迭代变量";
                f_s.second.print_symbol();
                cout << endl;
            }
        }

        void print_table() const
        {
            print_global_symbols();
            print_global_functions();
            print_global_struct();
        }


        void display(fstream& out) const;


    };





}// namespace my_compiler_symbol_table



#endif //MY_COMPILER_SYMBOLTABLE_H
