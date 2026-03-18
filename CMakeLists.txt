cmake_minimum_required(VERSION 3.22)
project(my_compiler)

set(CMAKE_CXX_STANDARD 17)

add_executable(my_compiler main.cpp
        header_files.cpp
        header_files.h
        compiler.cpp
        compiler.h
        front_end/lexer/lexer.h
        front_end/parser/parser.cpp
        front_end/parser/parser.h
        token/token_header.h
        token/keyword/keyword_token.cpp
        token/keyword/keyword_token.h
        token/identifier/identifier_token.cpp
        token/identifier/identifier_token.h
        token/operator/operator_token.cpp
        token/operator/operator_token.h
        token/delimiter/delimiter_token.cpp
        token/delimiter/delimiter_token.h
        token/number/number_token.cpp
        token/number/number_token.h
        token/token.h
        token/token.cpp
        front_end/lexer/state_transition/state_transition.h
        SymbolTable/SymbolTable.cpp
        SymbolTable/SymbolTable.h
        ErrorHandler/ErrorTable.cpp
        ErrorHandler/ErrorTable.h
        ErrorHandler/ErrorDefines.h
        front_end/parser/context_free_grammar.cpp
        front_end/parser/context_free_grammar.h
        front_end/front_end.cpp
        front_end/front_end.h
        front_end/parser/grammatical_analysis_tree.cpp
        front_end/parser/grammatical_analysis_tree.h
        front_end/parser/grammatical_analysis_tree.h
        front_end/name_generator.h
        front_end/name_generator.h
)
