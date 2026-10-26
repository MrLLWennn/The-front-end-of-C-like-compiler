"""CP02：循环、比较、调用与完整程序的语义及原生运行对照。"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


POSITIVE = [
    ("loop_sum", "int calc(int n) { int i = 1; int total = 0; while(i <= n) { total = total + i; i = i + 1; } return total; }"),
    ("zero_iterations", "int calc(int n) { int total = n; while(0) { total = total + 1; } return total; }"),
    ("countdown", "int calc(int n) { int total = 0; while(n > 0) { total = total + n; n = n - 1; } return total; }"),
    ("condition_call", "int positive(int x) { return x > 0; } int calc(int n) { int total = 0; while(positive(n)) { total = total + n; n = n - 1; } return total; }"),
    ("nested_loops", "int calc(int n) { int i = 0; int total = 0; while(i < n) { int j = 0; while(j < i) { total = total + j; j = j + 1; } i = i + 1; } return total; }"),
    ("branch_in_loop", "int calc(int n) { int i = 0; int total = 0; while(i < n) { if(i < 3) { total = total + 1; } else { total = total + 2; } i = i + 1; } return total; }"),
    ("loop_in_branch", "int calc(int n) { int total = 0; if(n > 0) { int i = 0; while(i < n) { total = total + i; i = i + 1; } } else { total = 99; } return total; }"),
    ("loop_shadow", "int calc(int n) { int i = 0; int total = 0; while(i < n) { int n = 2; total = total + n; i = i + 1; } return total + n; }"),
    ("zero_arguments", "int seed(void) { return 7; } int calc(int n) { return seed() + n; }"),
    ("multiple_arguments", "int combine(int a, int b) { return a * 10 + b; } int calc(int n) { return combine(n + 1, 3); }"),
    ("nested_calls", "int inc(int x) { return x + 1; } int calc(int n) { return inc(inc(n)); }"),
    ("prototype_then_definition", "int helper(int x); int calc(int n) { return helper(n); } int helper(int x) { return x + 3; }"),
    ("self_recursion", "int calc(int n) { int result = 1; if(n > 1) { result = n * calc(n - 1); } return result; }"),
    ("comparison_arithmetic", "int calc(int n) { return (n < 2) + (n == 2) * 10; }"),
    ("comparison_local", "int calc(int n) { int value = n <= 2; return value * 9; }"),
    ("comparison_precedence", "int calc(int n) { return n + 1 < 3; }"),
]
POSITIVE.extend((f"compare_{name}", f"int calc(int n) {{ return n {op} 2; }}")
                for name, op in [("lt", "<"), ("le", "<="), ("gt", ">"),
                                 ("ge", ">="), ("eq", "=="), ("ne", "!=")])

NEGATIVE = [
    ("loop_only_initialization", "int calc(int n) { int value; while(n) { value = 1; n = 0; } return value; }", "E_SEMA_UNINITIALIZED", "uninitialized name 'value'"),
    ("loop_uninitialized_condition", "int calc(int n) { int value; while(value) { value = n; } return n; }", "E_SEMA_UNINITIALIZED", "uninitialized name 'value'"),
    ("loop_read_before_write", "int calc(int n) { int value; while(n) { value = value + 1; n = 0; } return n; }", "E_SEMA_UNINITIALIZED", "uninitialized name 'value'"),
    ("loop_scope_leak", "int calc(int n) { while(n) { int value = 1; n = 0; } return value; }", "E_SEMA_NAME", "undeclared name 'value'"),
    ("loop_shadow_self_init", "int calc(int n) { while(n) { int n = n; } return n; }", "E_SEMA_UNINITIALIZED", "uninitialized name 'n'"),
    ("undefined_function", "int calc(int n) { return missing(n); }", "E_SEMA_CALL", "function 'missing'"),
    ("forward_without_prototype", "int calc(int n) { return later(n); } int later(int x) { return x; }", "E_SEMA_CALL", "function 'later'"),
    ("too_few_arguments", "int helper(int a, int b) { return a + b; } int calc(int n) { return helper(n); }", "E_SEMA_ARGUMENTS", "expected 2, got 1"),
    ("too_many_arguments", "int helper(int a) { return a; } int calc(int n) { return helper(n, 1); }", "E_SEMA_ARGUMENTS", "expected 1, got 2"),
    ("uninitialized_argument", "int helper(int a) { return a; } int calc(int n) { int value; return helper(value); }", "E_SEMA_UNINITIALIZED", "uninitialized name 'value'"),
    ("local_shadows_function", "int helper(int a) { return a; } int calc(int n) { int helper = 4; return helper(n); }", "E_SEMA_CALL", "is a local object"),
    ("parameter_shadows_function", "int helper(int a) { return a; } int calc(int helper) { return helper(1); }", "E_SEMA_CALL", "is a local object"),
    ("unsupported_break", "int calc(int n) { while(n) { break; } return n; }", "E_AST_STATEMENT", "got 'break'"),
    ("unsupported_continue", "int calc(int n) { while(n) { continue; } return n; }", "E_AST_STATEMENT", "got 'continue'"),
    ("unsupported_early_return", "int calc(int n) { while(n) { return n; } return 0; }", "E_AST_STATEMENT", "got 'return'"),
    ("unsupported_division", "int calc(int n) { return n / 2; }", "E_AST_EXPECTED", "got '/'"),
]
INPUTS = [-3, -1, 0, 1, 2, 3, 5, 8, 12]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(argv, case, label, expected=0):
    result = subprocess.run([str(value) for value in argv], cwd=case,
                            capture_output=True, timeout=15)
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
    args.compiler = args.compiler.resolve()
    args.grammar = args.grammar.resolve()
    if args.clang:
        args.clang = args.clang.resolve()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="programs-native-" if args.clang else "programs-ast-",
                               dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_bytes = args.grammar.read_bytes()
    records = []

    def compile_source(name, source_text, level=0, code=None, detail=None):
        case = run / name
        case.mkdir()
        source = case / "source.c"
        payload = (source_text.rstrip() + "\n").encode("utf-8")
        source.write_bytes(payload)
        output = case / "output"
        argv = [args.compiler, "--input", source, "--grammar", args.grammar,
                "--output-dir", output, "--emit", "llvm" if args.clang else "ast"]
        if args.clang:
            argv.extend(["--opt-level", str(level)])
        result = command(argv, case, "compile", 1 if code else 0)
        require(source.read_bytes() == payload and args.grammar.read_bytes() == grammar_bytes,
                f"{name}: input bytes changed")
        stack = output / "10-stack_file.txt"
        require(stack.is_file() and "预测分析结束" in stack.read_text(encoding="utf-8"),
                f"{name}: original parser did not complete")
        if code:
            stderr = result.stderr.decode("utf-8", errors="strict")
            require(f"error[{code}]" in stderr and detail in stderr,
                    f"{name}: wrong diagnostic: {stderr!r}")
            if name == "unsupported_continue":
                tokens = (output / "03-lexer.txt").read_text(encoding="utf-8")
                require('Keyword("continue")' in tokens,
                        "continue must be classified as a keyword, not an assignment identifier")
            for artifact in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                require(not (output / artifact).exists(), f"{name}: published {artifact} after rejection")
        else:
            require(not result.stderr, f"{name}: unexpected diagnostic: {result.stderr!r}")
            tree = (output / "15-ast.txt").read_text(encoding="utf-8")
            require("函数 " in tree and "返回" in tree, f"{name}: incomplete AST")
            if "while(" in source_text or "while(" in source_text.replace(" ", ""):
                require("while循环" in tree, f"{name}: loop missing from AST")
            if args.clang:
                require((output / "18-program.o").stat().st_size > 0, f"{name}: no object file")
                before = (output / "16-before.ll").read_text(encoding="utf-8")
                if "while循环" in tree:
                    require("while.condition" in before and "while.body" in before and
                            "while.end" in before, f"{name}: loop CFG missing")
                if "函数调用 " in tree:
                    require("call i32" in before, f"{name}: call missing from raw IR")
            else:
                require(not (output / "18-program.o").exists(), f"{name}: AST mode invoked backend")
        return case, source, output

    # 独立的C调用程序只负责传入固定输入、打印结果，由Clang处理其stdio和数组。
    harness_text = (
        '#include <stdio.h>\nextern int calc(int);\nint main(void)\n{\n'
        '    int inputs[] = {-3, -1, 0, 1, 2, 3, 5, 8, 12};\n'
        '    for(int i = 0; i < 9; ++i)\n    {\n'
        '        printf("%d:%d\\n", inputs[i], calc(inputs[i]));\n'
        '    }\n    return 0;\n}\n')

    levels = [0, 2] if args.clang else [0]
    for name, text in POSITIVE:
        for level in levels:
            case, source, output = compile_source(f"{name}_O{level}", text, level)
            record = {"case": name, "optimization": level,
                      "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
            if args.clang:
                harness = case / "harness.c"
                harness.write_text(harness_text, encoding="ascii")
                exe, reference = case / "actual", case / "reference"
                command([args.clang, harness, output / "18-program.o", "-o", exe], case, "link")
                command([args.clang, "-std=c17", "-Werror=implicit-function-declaration",
                         f"-O{level}", "-fwrapv", source, harness, "-o", reference], case, "reference_build")
                actual = command([exe], case, "run").stdout
                oracle = command([reference], case, "reference_run").stdout
                require(actual == oracle, f"{name}: runtime differs from Clang: {actual!r} != {oracle!r}")
                lines = actual.decode("ascii").splitlines()
                require(len(lines) == len(INPUTS), f"{name}: missing runtime inputs")
                if name in ["loop_sum", "countdown", "condition_call"]:
                    expected = [f"{n}:{max(n, 0) * (max(n, 0) + 1) // 2}" for n in INPUTS]
                    require(lines == expected, f"{name}: independent sum formula disagrees")
                record["runtime_inputs"] = INPUTS
                record["actual"] = lines
            records.append(record)
            print(f"PASS {name}_O{level}：" + ("9组输入与Clang结果一致" if args.clang else "语义检查通过"), flush=True)

    example = Path(__file__).resolve().parents[1] / "examples" / "mini_c.c"
    original_example = example.read_bytes()
    for level in levels:
        case, source, output = compile_source(f"complete_program_O{level}", original_example.decode("utf-8"), level)
        if args.clang:
            exe, reference = case / "mini_c", case / "reference"
            command([args.clang, output / "18-program.o", "-o", exe], case, "link")
            command([args.clang, "-std=c17", f"-O{level}", source, "-o", reference], case, "reference_build")
            actual = command([exe], case, "run")
            oracle = command([reference], case, "reference_run")
            require(actual.stdout == oracle.stdout, "complete program output mismatch")
        require(example.read_bytes() == original_example, "tracked example was changed")
        records.append({"case": "complete_program", "optimization": level,
                        "expected_main_exit": 0, "executed": bool(args.clang)})
        print(f"PASS complete_program_O{level}：" + ("编译、链接、运行及3项自检通过" if args.clang else "完整main程序语义检查通过"), flush=True)

    for name, text, code, detail in NEGATIVE:
        compile_source(name, text, level=2 if args.clang else 0, code=code, detail=detail)
        records.append({"case": name, "accepted": False, "diagnostic": code})
        print(f"PASS {name}：诊断={code}，没有发布最终产物", flush=True)

    (run / "results.json").write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"简易程序：{len(records)}项用例通过；" +
          (f"{len(POSITIVE) * len(levels) * len(INPUTS)}次calc调用及2次完整程序运行，未作性能测量"
           if args.clang else "包含正例、完整程序与拒绝用例"))


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, UnicodeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
