cmake_minimum_required(VERSION 3.22)
project(my_compiler LANGUAGES CXX)

add_library(compiler_core STATIC
        header_files.cpp
        compiler.cpp
        front_end/front_end.cpp
        front_end/preprocessor.cpp
        front_end/ast.cpp
        front_end/ast_builder.cpp
        front_end/parser/parser.cpp
        front_end/parser/context_free_grammar.cpp
        front_end/parser/grammatical_analysis_tree.cpp
        token/token.cpp
        token/keyword/keyword_token.cpp
        token/identifier/identifier_token.cpp
        token/operator/operator_token.cpp
        token/delimiter/delimiter_token.cpp
        token/number/number_token.cpp
        SymbolTable/SymbolTable.cpp
        ErrorHandler/ErrorTable.cpp
)

target_compile_features(compiler_core PUBLIC cxx_std_17)

target_include_directories(compiler_core
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
               "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(my_compiler main.cpp app/driver_options.cpp)
target_link_libraries(my_compiler PRIVATE compiler_core)

set_target_properties(compiler_core my_compiler
        PROPERTIES CXX_EXTENSIONS OFF
)

option(MY_COMPILER_ENABLE_LLVM "Build the LLVM 18 native code backend" OFF)
if(MY_COMPILER_ENABLE_LLVM)
        # LLVM 的依赖检查可能编译 C 探针，例如 FFI 和 terminfo。
        enable_language(C)
        find_package(LLVM 18.1 CONFIG REQUIRED)
        if(NOT LLVM_VERSION_MAJOR EQUAL 18)
                message(FATAL_ERROR "This initial backend is validated against LLVM 18 headers only")
        endif()
        add_library(compiler_llvm STATIC codegen/llvm_backend.cpp)
        target_link_libraries(compiler_llvm PUBLIC compiler_core PRIVATE LLVM)
        target_include_directories(compiler_llvm SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
        separate_arguments(llvm_definition_flags NATIVE_COMMAND "${LLVM_DEFINITIONS}")
        target_compile_options(compiler_llvm PRIVATE ${llvm_definition_flags})
        set_target_properties(compiler_llvm PROPERTIES CXX_EXTENSIONS OFF)
        target_link_libraries(my_compiler PRIVATE compiler_llvm)
        target_compile_definitions(my_compiler PRIVATE MY_COMPILER_WITH_LLVM=1)
        message(STATUS "my_compiler LLVM backend: ${LLVM_PACKAGE_VERSION}")
endif()

include(CTest)

if(BUILD_TESTING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        find_program(GRAPHVIZ_DOT NAMES dot REQUIRED)

        add_test(NAME compiler_minimal_program
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_minimal_program.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${GRAPHVIZ_DOT}"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
        )

        set_tests_properties(compiler_minimal_program PROPERTIES TIMEOUT 25)

        add_executable(cfg_probe tests/cfg_probe.cpp)
        target_link_libraries(cfg_probe PRIVATE compiler_core)

        add_test(NAME compiler_follow_sets
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_follow_sets.py"
                    "$<TARGET_FILE:cfg_probe>"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
        )
        set_tests_properties(compiler_follow_sets PROPERTIES TIMEOUT 25)

        add_test(NAME compiler_cli_contract
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli_contract.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
        )
        set_tests_properties(compiler_cli_contract PROPERTIES TIMEOUT 45)
        add_test(NAME compiler_diagnostics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_diagnostics.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
        )
        set_tests_properties(compiler_diagnostics PROPERTIES TIMEOUT 45)

        add_executable(cfg_table_probe tests/cfg_table_probe.cpp)
        target_link_libraries(cfg_table_probe PRIVATE compiler_core)
        add_test(NAME compiler_prediction_table
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_prediction_table.py"
                    "$<TARGET_FILE:cfg_table_probe>"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_prediction_table PROPERTIES TIMEOUT 30)

        add_test(NAME compiler_ast_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ast_semantics.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_ast_semantics PROPERTIES TIMEOUT 45)
        add_test(NAME compiler_local_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_local_variables.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_local_semantics PROPERTIES TIMEOUT 60)

        add_test(NAME compiler_branch_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_branches.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_branch_semantics PROPERTIES TIMEOUT 60)

        add_test(NAME compiler_program_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_programs.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_program_semantics PROPERTIES TIMEOUT 90)

        add_test(NAME compiler_headers_printf_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_headers_printf.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_headers_printf_semantics PROPERTIES TIMEOUT 120)

        add_test(NAME compiler_types_structures_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_types_structures.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_types_structures_semantics PROPERTIES TIMEOUT 180)

        add_test(NAME compiler_arrays_pointers_for_semantics
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_arrays_pointers_for.py"
                    "$<TARGET_FILE:my_compiler>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
        set_tests_properties(compiler_arrays_pointers_for_semantics PROPERTIES TIMEOUT 180)

        if(MY_COMPILER_ENABLE_LLVM)
                find_program(LLVM_TEST_CLANG NAMES clang-18
                        HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
                add_test(NAME compiler_llvm_pipeline
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_llvm_pipeline.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${LLVM_TEST_CLANG}"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs")
                set_tests_properties(compiler_llvm_pipeline PROPERTIES TIMEOUT 60)
                add_test(NAME compiler_local_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_local_variables.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_local_llvm PROPERTIES TIMEOUT 90)
                add_test(NAME compiler_branch_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_branches.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_branch_llvm PROPERTIES TIMEOUT 120)
                add_test(NAME compiler_program_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_programs.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_program_llvm PROPERTIES TIMEOUT 180)
                add_test(NAME compiler_headers_printf_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_headers_printf.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_headers_printf_llvm PROPERTIES TIMEOUT 240)
                add_test(NAME compiler_types_structures_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_types_structures.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_types_structures_llvm PROPERTIES TIMEOUT 300)
                add_test(NAME compiler_arrays_pointers_for_llvm
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_arrays_pointers_for.py"
                            "$<TARGET_FILE:my_compiler>"
                            "${CMAKE_CURRENT_SOURCE_DIR}/files/04-grammar.txt"
                            "${CMAKE_CURRENT_BINARY_DIR}/test-runs"
                            --clang "${LLVM_TEST_CLANG}")
                set_tests_properties(compiler_arrays_pointers_for_llvm PROPERTIES TIMEOUT 300)
        endif()

endif()
