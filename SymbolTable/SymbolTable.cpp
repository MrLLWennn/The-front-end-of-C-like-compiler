//
// 创建者：86198；日期：2024/10/30。
//

#include "SymbolTable.h"


namespace my_compiler_symbol_table
{

    size_t Symbol::symbol_count_ = 0;

    void Symbol::set(Category cat, const string &name, const string &type, size_t level, bool formal)
    {
        category_ = cat;
        name_ = name;
        type_ = type;
        level_ = level;
        is_formal_ = formal;
    }

    void SymbolTable::emplace_function(const FunctionSymbol &func)
    {
        functions_.emplace_back(func);
    }

    bool SymbolTable::function_exists(const string &name) const
    {
        for (const auto &f: functions_)
        {
            if(f.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    FunctionSymbol& SymbolTable::get_function(const string &name)
    {
        for (auto &f: functions_)
        {
            if(f.name_ == name)
            {
                return f;
            }
        }
        throw std::exception();
    }

    bool SymbolTable::global_symbol_exists(const string &name) const
    {
        for(const auto& g_s : global_symbols_)
        {
            if(g_s.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    bool SymbolTable::local_symbol_exists(const string &func_name, const string& symbol_name) const
    {
        for(const auto& f : functions_)
        {
            if(f.name_ == func_name)
            {
               if(f.local_symbol_exists(symbol_name))
               {
                   return true;
               }
            }
        }
        return false;
    }

    void SymbolTable::emplace_global_symbol(const Symbol &sym)
    {
        global_symbols_.emplace_back(sym);
    }

    bool SymbolTable::function_parameter_exists(const string &func_name, const string &symbol_name) const
    {
        for(const auto& f : functions_)
        {
            if(f.name_ == func_name)
            {
                if(f.parameter_exists(symbol_name))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void SymbolTable::print_global_functions() const
    {
        for(const auto& f : functions_)
        {
            f.print_function();
        }
    }

    void SymbolTable::emplace_struct(const StructSymbol &st)
    {
        structs_.emplace_back(st);
    }

    StructSymbol &SymbolTable::get_struct(const string &name)
    {
        for (auto & s : structs_)
        {
            if(s.name_ == name)
            {
                return s;
            }
        }
        throw std::exception();
    }

    [[maybe_unused]] bool SymbolTable::struct_symbol_exists(const string &struct_name, const string &symbol_name) const
    {
        for(const auto&stu : structs_)
        {
            if(stu.name_ == struct_name)
            {
                if(stu.symbol_exists(symbol_name))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool SymbolTable::struct_function_exists(const string &struct_name, const string &symbol_name) const
    {
        for(const auto& s : structs_)
        {
            if(s.name_ == struct_name)
            {
                if(s.function_exists(symbol_name))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void SymbolTable::display(fstream &out) const
    {
        out << "my_compiler -- 符号表" << endl << endl;
        out << std::setiosflags(ios::left) << std::setw(40) << "符号名"
            << std::setw(60) << "符号属性"
            << std::setw(60) << "符号地址"
            << endl;

        for(const auto& f : functions_)
        {
            out << std::setiosflags(ios::left) << std::setw(40) << f.name_
                << std::setw(60) << f.property_
                << std::setw(60) << f.address_
                << endl;
        }

        for(const auto& s : structs_)
        {
            out << std::setiosflags(ios::left) << std::setw(40) << s.name_
                << std::setw(60) << s.property_
                << std::setw(60) << s.address_
                << endl;
        }


        out << "\n\n        全局函数-子符号表" << endl << endl;
        for(const auto& f : functions_)
        {
            out << "            全局函数名：" << std::setw(15) << f.name_ << "         " << f.type_  << endl;
            out << "            " << std::setiosflags(ios::left)
                << std::setw(20) << "参数名"
                << std::setw(40) << "参数属性"
                << std::setw(20) << "参数类型"
                << std::setw(20) << "参数地址" << endl;
            for(const auto& p : f.parameters_)
            {
                p.display_symbol(out);
            }

            out << "            " << std::setiosflags(ios::left)
                << std::setw(20) << "局部变量名"
                << std::setw(40) << "局部变量属性"
                << std::setw(20) << "局部变量类型"
                << std::setw(20) << "局部变量地址" << endl;

            for(const auto& s : f.local_symbols_)
            {
                s.display_symbol(out);
            }

            out << "            " << std::setiosflags(ios::left)
                << std::setw(20) << "调用函数名"
                << std::setw(40) << "调用函数属性"
                << std::setw(20) << "调用函数类型"
                << std::setw(20) << "调用函数地址" << endl;

            for(const auto& f_c : f.function_call_)
            {
                for(const auto& ff : functions_)
                {
                    if(ff.name_ == f_c)
                    {
                        out << "            " << std::setiosflags(ios::left)
                            << std::setw(20) << ff.name_
                            << std::setw(40) << ff.property_
                            << std::setw(20) << ff.type_
                            << std::setw(20) << ff.address_ << endl;
                    }
                }
            }

            out << endl;
        }

        out << "\n\n        结构体-子符号表" << endl << endl;
        for(const auto& s : structs_)
        {
            out << "            结构体名：" << s.name_ << endl;
            out << "                成员变量表" << endl;
            out << "                " << std::setiosflags(ios::left)
                << std::setw(20) << "成员变量名"
                << std::setw(40) << "成员变量属性"
                << std::setw(20) << "成员变量类型"
                << std::setw(20) << "成员变量地址" << endl;
            for(const auto& s_s : s.struct_symbols_)
            {
                out << "    ";
                s_s.display_symbol(out);
            }

            out << "                成员函数表" << endl;
            for(const auto& s_f : s.struct_functions_)
            {
                out << "                    成员函数名：" << std::setw(15) << s_f.name_ << "         " << s_f.type_ << endl;
                out << "                    " << std::setiosflags(ios::left)
                    << std::setw(20) << "参数名"
                    << std::setw(40) << "参数属性"
                    << std::setw(20) << "参数类型"
                    << std::setw(20) << "参数地址" << endl;
                for(const auto& p : s_f.parameters_)
                {
                    out << "        ";
                    p.display_symbol(out);
                }

                out << "                    " << std::setiosflags(ios::left)
                    << std::setw(20) << "局部变量名"
                    << std::setw(40) << "局部变量属性"
                    << std::setw(20) << "局部变量类型"
                    << std::setw(20) << "局部变量地址" << endl;

                for(const auto& ss : s_f.local_symbols_)
                {
                    out << "        ";
                    ss.display_symbol(out);
                }

                out << "                    " << std::setiosflags(ios::left)
                    << std::setw(20) << "调用函数名"
                    << std::setw(40) << "调用函数属性"
                    << std::setw(20) << "调用函数类型"
                    << std::setw(20) << "调用函数地址" << endl;

                for(const auto& f_c : s_f.function_call_)
                {
                    for(const auto& ff : functions_)
                    {
                        if(ff.name_ == f_c)
                        {
                            out << "                    " << std::setiosflags(ios::left)
                                << std::setw(20) << ff.name_
                                << std::setw(40) << ff.property_
                                << std::setw(20) << ff.type_
                                << std::setw(20) << ff.address_ << endl;
                        }
                    }
                }

                out << endl;
            }
        }
    }


    void SymbolTable::emplace_circulation_symbol(const string& func_name , const Symbol &sym)
    {
        auto iter = for_symbols_.find(func_name);
        if(iter != for_symbols_.end())
        {
            iter->second = sym;
        }
        else
        {
            for_symbols_[func_name] = sym;
        }
    }


    bool SymbolTable::for_symbol_exists(const string &func_name, const string &symbol_name) const
    {
        for(const auto& f_s : for_symbols_)
        {
            if(f_s.first == func_name && f_s.second.name_ == symbol_name)
            {
                return true;
            }
        }
        return false;
    }


    void FunctionSymbol::emplace_parameter(const Symbol &para)
    {
        parameters_.emplace_back(para);
    }

    bool FunctionSymbol::parameter_exists(const string& name) const
    {
        for(const auto& p : parameters_)
        {
            if(p.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    void FunctionSymbol::emplace_local_symbol(const Symbol &local)
    {
        local_symbols_.push_back(local);
    }

    bool FunctionSymbol::local_symbol_exists(const string &name) const
    {
        for(const auto& s : local_symbols_)
        {
            if(s.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    void FunctionSymbol::emplace_function_call(const string &name)
    {
        if(std::find(function_call_.begin(), function_call_.end(), name) == function_call_.end())
        {
            function_call_.emplace_back(name);
        }
    }


    void StructSymbol::emplace_struct_function(const FunctionSymbol& func)
    {
        struct_functions_.emplace_back(func);
    }

    void StructSymbol::emplace_struct_symbols(const Symbol &sym)
    {
        struct_symbols_.emplace_back(sym);
    }

    bool StructSymbol::symbol_exists(const string &name) const
    {
        for(const auto& s : struct_symbols_)
        {
            if(s.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    bool StructSymbol::function_exists(const string &name) const
    {
        for(const auto& f : struct_functions_)
        {
            if(f.name_ == name)
            {
                return true;
            }
        }
        return false;
    }

    FunctionSymbol &StructSymbol::get_struct_function(const string &name)
    {
        for(auto& f : struct_functions_)
        {
            if(f.name_ == name)
            {
                return f;
            }
        }
        throw std::exception();
    }

}