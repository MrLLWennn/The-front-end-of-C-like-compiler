"""CP03：真实本地包含、受限stdio契约、字符串与printf端到端回归。"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

STDIO = "#include <stdio.h>\n"
POSITIVE = [
    ("plain_text", STDIO + r'int main(void) { printf("hello\n"); return 0; }', {}, "hello\n"),
    ("integer_formats", STDIO + r'int main(void) { printf("%d %i %c %%\n", 0 - 7, 12, 65); return 0; }', {}, "-7 12 A %\n"),
    ("string_format", STDIO + r'int main(void) { printf("%s:%d\n", "result", 55); return 0; }', {}, "result:55\n"),
    ("width_flags", STDIO + r'int main(void) { printf("%04d|%-5s|%+d\n", 7, "ok", 2); return 0; }', {}, "0007|ok   |+2\n"),
    ("comment_markers_in_string", STDIO + r'int main(void) { printf("http://demo/*keep*/ //keep  two  spaces\n"); return 0; }', {}, "http://demo/*keep*/ //keep  two  spaces\n"),
    ("escaped_quote_backslash", STDIO + r'int main(void) { printf("\"hi\"\\end\t!\n"); return 0; }', {}, '"hi"\\end\t!\n'),
    ("utf8_text", STDIO + r'int main(void) { printf("结果=%d\n", 55); return 0; }', {}, "结果=55\n"),
    ("printf_return_count", STDIO + r'int main(void) { int count = printf("%s:%d\n", "x", 3); int code = 0; if(count != 4) { code = 1; } return code; }', {}, "x:3\n"),
    ("local_prototype", STDIO + '#include "api.h"\n' + r'int main(void) { printf("%d\n", helper(4)); return 0; } int helper(int x) { return x + 1; }', {"api.h": "int helper(int x);\n"}, "5\n"),
    ("guarded_repeated_include", STDIO + '#include "helper.h"\n#include "helper.h"\n' + r'int main(void) { printf("%d\n", helper(4)); return 0; }', {"helper.h": "#ifndef HELPER_H\n#define HELPER_H\nint helper(int x) { return x + 1; }\n#endif\n"}, "5\n"),
    ("pragma_once", STDIO + '#include "helper.h"\n#include "helper.h"\n' + r'int main(void) { printf("%d\n", helper(4)); return 0; }', {"helper.h": "#pragma once\nint helper(int x) { return x + 1; }\n"}, "5\n"),
    ("nested_relative_include", STDIO + '#include "inc/api.h"\n' + r'int main(void) { printf("%d\n", helper(4)); return 0; }', {"inc/api.h": '#include "detail.h"\n', "inc/detail.h": "int helper(int x) { return x + 1; }\n"}, "5\n"),
    ("guarded_mutual_include", STDIO + '#include "a.h"\n' + r'int main(void) { printf("%d\n", one(2) + two(2)); return 0; } int one(int x) { return x + 1; } int two(int x) { return x + 2; }', {"a.h": '#ifndef A_H\n#define A_H\n#include "b.h"\nint one(int x);\n#endif\n', "b.h": '#ifndef B_H\n#define B_H\n#include "a.h"\nint two(int x);\n#endif\n'}, "7\n"),
    ("integer_macro", STDIO + '#include "limits.h"\n' + r'int main(void) { printf("LIMIT=%d\n", LIMIT); return 0; }', {"limits.h": "#ifndef LOCAL_LIMITS\n#define LOCAL_LIMITS\n#define LIMIT 5\n#endif\n"}, "LIMIT=5\n"),
    ("conditional_include", STDIO + '#define READY\n#ifdef READY\n#include "helper.h"\n#else\n#include "missing.h"\n#endif\n' + r'int main(void) { printf("%d\n", helper(4)); return 0; }', {"helper.h": "int helper(int x) { return x + 1; }\n"}, "5\n"),
    ("stdio_from_header", '#include "io.h"\n' + r'int main(void) { printf("ok\n"); return 0; }', {"io.h": "#include <stdio.h>\n"}, "ok\n"),
    ("comment_separator", STDIO + r'int main(void) { int/**/value = 5; printf("%d\n", value); return 0; }', {}, "5\n"),
    ("operator_lookahead", STDIO + r'int main(void) { int value=(2+3)*4; printf("%d\n", value); return 0; }', {}, "20\n"),
    ("standalone_user_call", STDIO + r'int helper(int x) { return x + 1; } int main(void) { helper(4); printf("ok\n"); return 0; }', {}, "ok\n"),
]

# 每项都同时检查原始输入未被改写，以及失败后未发布新的最终产物。
NEGATIVE = [
    ("no_stdio", r'int main(void) { printf("x\n"); return 0; }', {}, "E_SEMA_CALL", "function 'printf'"),
    ("unsupported_system_header", "#include <math.h>\nint main(void) { return 0; }", {}, "E_PP_HEADER", "unsupported system header"),
    ("missing_header", '#include "missing.h"\nint main(void) { return 0; }', {}, "E_PP_INCLUDE", "cannot find local header"),
    ("unguarded_cycle", '#include "a.h"\nint main(void) { return 0; }', {"a.h": '#include "b.h"\n', "b.h": '#include "a.h"\n'}, "E_PP_CYCLE", "unguarded recursive include"),
    ("unsupported_if", "#if 1\nint main(void) { return 0; }\n#endif", {}, "E_PP_DIRECTIVE", "unsupported preprocessing directive"),
    ("function_macro", "#define INC(x) x+1\nint main(void) { return 0; }", {}, "E_PP_DIRECTIVE", "unsupported preprocessing directive"),
    ("expression_macro", "#define LIMIT (5)\nint main(void) { return 0; }", {}, "E_PP_DIRECTIVE", "unsupported preprocessing directive"),
    ("macro_redefinition", "#define LIMIT 5\n#define LIMIT 6\nint main(void) { return 0; }", {}, "E_PP_DIRECTIVE", "macro redefinition"),
    ("unmatched_endif", "#endif\nint main(void) { return 0; }", {}, "E_PP_DIRECTIVE", "unmatched #endif"),
    ("unterminated_guard", "#ifndef LOCAL\n#define LOCAL\nint main(void) { return 0; }", {}, "E_PP_DIRECTIVE", "unterminated"),
    ("unclosed_comment", "/* missing end", {}, "E_PP_COMMENT", "unterminated block comment"),
    ("no_format", STDIO + "int main(void) { printf(); return 0; }", {}, "E_SEMA_FORMAT", "string literal format"),
    ("nonliteral_format", STDIO + "int main(void) { printf(5); return 0; }", {}, "E_SEMA_FORMAT", "string literal format"),
    ("missing_format_argument", STDIO + r'int main(void) { printf("%d\n"); return 0; }', {}, "E_SEMA_FORMAT", "argument count mismatch"),
    ("extra_format_argument", STDIO + r'int main(void) { printf("ok\n", 5); return 0; }', {}, "E_SEMA_FORMAT", "argument count mismatch"),
    ("string_for_integer", STDIO + r'int main(void) { printf("%d\n", "bad"); return 0; }', {}, "E_SEMA_FORMAT", "int argument"),
    ("integer_for_string", STDIO + r'int main(void) { printf("%s\n", 5); return 0; }', {}, "E_SEMA_FORMAT", "%s requires a string literal"),
    ("float_format_with_integer", STDIO + r'int main(void) { printf("%f\n", 5); return 0; }', {}, "E_SEMA_FORMAT", "float or double argument"),
    ("writeback_conversion", STDIO + r'int main(void) { printf("%n", 5); return 0; }', {}, "E_SEMA_FORMAT", "supported printf conversions"),
    ("length_modifier", STDIO + r'int main(void) { printf("%ld", 5); return 0; }', {}, "E_SEMA_FORMAT", "supported printf conversions"),
    ("dynamic_width", STDIO + r'int main(void) { printf("%*d", 3, 5); return 0; }', {}, "E_SEMA_FORMAT", "supported printf conversions"),
    ("incomplete_conversion", STDIO + r'int main(void) { printf("%"); return 0; }', {}, "E_SEMA_FORMAT", "incomplete printf conversion"),
    ("uninitialized_printf_argument", STDIO + r'int main(void) { int value; printf("%d", value); return 0; }', {}, "E_SEMA_UNINITIALIZED", "uninitialized name 'value'"),
    ("string_arithmetic", STDIO + r'int main(void) { return "bad" + 1; }', {}, "E_SEMA_TYPE", "cannot convert i8* to i32"),
    ("string_return", r'int main(void) { return "bad"; }', {}, "E_SEMA_TYPE", "cannot convert i8* to i32"),
    ("string_initializer", r'int main(void) { int value="bad"; return value; }', {}, "E_SEMA_TYPE", "cannot convert i8* to i32"),
    ("incompatible_prototype", "int helper(int x); int helper(int x, int y) { return x + y; } int main(void) { return 0; }", {}, "E_SEMA_FUNCTION", "incompatible declaration"),
    ("unknown_escape", STDIO + r'int main(void) { printf("bad\q"); return 0; }', {}, "E_LEX_STRING", "unsupported string escape"),
    ("unclosed_string", STDIO + 'int main(void) { printf("bad', {}, "E_LEX_STRING", "string literal"),
    ("nul_string", STDIO + 'int main(void) { printf("bad\x00"); return 0; }', {}, "E_LEX_STRING", "NUL"),
    ("header_output_collision", '#include "output/15-ast.txt"\nint main(void) { return 0; }', {"output/15-ast.txt": "int helper(int x);\n"}, "E_PP_OUTPUT", "overwrite an included input"),
    ("header_first_output_collision", '#include "output/01-uncomment.txt"\nint main(void) { return 0; }', {"output/01-uncomment.txt": "int helper(int x);\n"}, "E_PP_OUTPUT", "overwrite an included input"),
    ("header_hardlink_collision", '#include "api.h"\nint main(void) { return 0; }', {"api.h": "int helper(int x);\n"}, "E_PP_OUTPUT", "overwrite an included input"),
]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(argv, case, label, expected=0):
    result = subprocess.run([str(value) for value in argv], cwd=case, capture_output=True, timeout=15)
    (case / f"{label}.stdout.log").write_bytes(result.stdout)
    (case / f"{label}.stderr.log").write_bytes(result.stderr)
    require(result.returncode == expected,
            f"{case.name}: {label}: expected exit {expected}, got {result.returncode}; {result.stderr!r}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("grammar", type=Path)
    parser.add_argument("output_root", type=Path)
    parser.add_argument("--clang", type=Path)
    args = parser.parse_args()
    args.compiler, args.grammar = args.compiler.resolve(), args.grammar.resolve()
    if args.clang:
        args.clang = args.clang.resolve()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="headers-native-" if args.clang else "headers-ast-", dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    records = []

    def compile_source(name, text, headers, level=0, code=None, detail=None):
        case = run / name
        case.mkdir()
        source = case / "source.c"
        source.write_text(text + "\n", encoding="utf-8")
        inputs = [source, args.grammar]
        for path, content in headers.items():
            header = case / path
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text(content, encoding="utf-8")
            inputs.append(header)
        output = case / "output"
        if name == "header_hardlink_collision":
            output.mkdir(exist_ok=True)
            os.link(case / "api.h", output / "15-ast.txt")
        before = {path: path.read_bytes() for path in inputs}
        argv = [args.compiler, "--input", source, "--grammar", args.grammar,
                "--output-dir", output, "--emit", "llvm" if args.clang else "ast"]
        if args.clang:
            argv += ["--opt-level", level]
        result = command(argv, case, "compile", 1 if code else 0)
        require(all(path.read_bytes() == data for path, data in before.items()), f"{name}: input changed")
        if code:
            diagnostic = result.stderr.decode("utf-8")
            require(f"error[{code}]" in diagnostic and detail in diagnostic,
                    f"{name}: wrong diagnostic: {diagnostic!r}")
            for filename in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                artifact = output / filename
                protected = artifact.exists() and any(artifact.samefile(path) for path in inputs)
                require(not artifact.exists() or protected, f"{name}: published {filename} after failure")
            if not code.startswith(("E_PP_", "E_LEX_")):
                stack = (output / "10-stack_file.txt").read_text(encoding="utf-8")
                require("预测分析结束" in stack, f"{name}: original parser was bypassed")
        else:
            require(not result.stderr, f"{name}: unexpected stderr: {result.stderr!r}")
            stack = (output / "10-stack_file.txt").read_text(encoding="utf-8")
            require("预测分析结束" in stack, f"{name}: original parser was bypassed")
            (output / "15-ast.txt").read_text(encoding="utf-8")
            if args.clang:
                require((output / "18-program.o").stat().st_size > 0, f"{name}: missing object")
                before_ir = (output / "16-before.ll").read_text(encoding="utf-8")
                require("@printf" in before_ir and "i32 (ptr, ...)" in before_ir,
                        f"{name}: missing real variadic printf call")
            else:
                require(not (output / "18-program.o").exists(), f"{name}: AST mode invoked backend")
        return case, source, output, before

    levels = [0, 2] if args.clang else [0]
    for name, text, headers, expected in POSITIVE:
        for level in levels:
            case, source, output, before = compile_source(f"{name}_O{level}", text, headers, level)
            if args.clang:
                exe, reference = case / "actual", case / "reference"
                command([args.clang, output / "18-program.o", "-o", exe], case, "link")
                command([args.clang, "-std=c17", "-Werror=implicit-function-declaration", f"-O{level}",
                         source, "-o", reference], case, "reference_build")
                actual = command([exe], case, "run").stdout
                oracle = command([reference], case, "reference_run").stdout
                require(actual == oracle == expected.encode("utf-8"),
                        f"{name}: output mismatch: actual={actual!r}, clang={oracle!r}, expected={expected!r}")
                require(all(path.read_bytes() == data for path, data in before.items()), f"{name}: input changed during run")
            records.append({"case": name, "optimization": level, "executed": bool(args.clang),
                            "expected_stdout": expected,
                            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest()})
            print(f"PASS {name}_O{level}：" + ("退出码及标准输出与Clang和预期一致" if args.clang else "语义检查通过"), flush=True)

    for name, text, headers, code, detail in NEGATIVE:
        compile_source(name, text, headers, 2 if args.clang else 0, code, detail)
        records.append({"case": name, "accepted": False, "diagnostic": code})
        print(f"PASS {name}：{code}；输入未改写，未发布新的最终产物", flush=True)

    # 同一个演示源码也走CTest，避免只测试临时夹具却漏掉实际展示程序。
    root = Path(__file__).resolve().parents[1]
    example = (root / "examples/mini_c_io.c").read_text(encoding="utf-8")
    header = (root / "examples/mini_c_math.h").read_text(encoding="utf-8")
    for level in levels:
        case, source, output, _ = compile_source(f"repository_example_O{level}", example,
                                                {"mini_c_math.h": header}, level)
        if args.clang:
            exe, reference = case / "actual", case / "reference"
            command([args.clang, output / "18-program.o", "-o", exe], case, "link")
            command([args.clang, "-std=c17", f"-O{level}", source, "-o", reference], case, "reference_build")
            actual, oracle = command([exe], case, "run"), command([reference], case, "reference_run")
            require(actual.stdout == oracle.stdout == b"sum_squares(5) = 55\nstatus: ok\n", "repository example output mismatch")
        records.append({"case": "repository_example", "optimization": level, "executed": bool(args.clang)})
        print(f"PASS repository_example_O{level}：样例通过", flush=True)
    (run / "results.json").write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"头文件与printf：{len(records)}项用例通过；本次未作性能测量")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, UnicodeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
