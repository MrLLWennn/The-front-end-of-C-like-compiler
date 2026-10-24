"""UR09：作用域/初始化负例与真实 O0/O2 原生调用，由用户执行。"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile


POSITIVE = [
    ("initialized", "int calc(int x) { int y = x + 1; y = y * 2; return y; }\n", "(x + 1) * 2"),
    ("assigned_later", "int calc(int x) { int y; y = x + 3; return y; }\n", "x + 3"),
    ("parameter_assignment", "int calc(int x) { x = x + 2; return x; }\n", "x + 2"),
    ("shadow_restored", "int calc(int x) { int y = x; { int y = 5; y = y + 1; } return y; }\n", "x"),
    ("outer_assignment", "int calc(int x) { int y; { y = x + 3; } return y; }\n", "x + 3"),
    ("nested_scope", "int calc(int x) { int y = x + 1; { int z = y + 2; { int y = z + 1; z = y + 2; } y = z; } return y; }\n", "x + 6"),
]
NEGATIVE = [
    ("uninitialized_read", "int f(void) { int y; return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("uninitialized_rhs", "int f(void) { int y; y = y + 1; return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("shadow_self_init", "int f(void) { int y = 2; { int y = y; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("duplicate_local", "int f(void) { int y; int y; return 0; }\n", "E_SEMA_LOCAL", "duplicate local name 'y'"),
    ("parameter_redeclaration", "int f(int x) { int x; return x; }\n", "E_SEMA_LOCAL", "duplicate local name 'x'"),
    ("undeclared_target", "int f(void) { y = 1; return 0; }\n", "E_SEMA_NAME", "undeclared name 'y'"),
    ("scope_leak", "int f(void) { { int y = 1; } return y; }\n", "E_SEMA_NAME", "undeclared name 'y'"),
    ("forward_reference", "int f(void) { int y = z; int z = 1; return y; }\n", "E_SEMA_NAME", "undeclared name 'z'"),
    ("missing_return", "int f(void) { int y = 1; }\n", "E_AST_RETURN", "final return"),
    ("nested_return", "int f(void) { { return 1; } return 0; }\n", "E_AST_STATEMENT", "lexical block"),
]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(argv, case, label, expected=0):
    result = subprocess.run([str(x) for x in argv], cwd=case, capture_output=True, timeout=15)
    (case / f"{label}.stdout.log").write_bytes(result.stdout)
    (case / f"{label}.stderr.log").write_bytes(result.stderr)
    require(result.returncode == expected,
            f"{label}: expected exit {expected}, got {result.returncode}; {result.stderr!r}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("grammar", type=Path)
    parser.add_argument("output_root", type=Path)
    parser.add_argument("--clang", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="locals-native-" if args.clang else "locals-ast-",
                               dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_bytes = args.grammar.read_bytes()

    def compile_source(name, text, level=0, code=None, detail=None):
        case = run / name
        case.mkdir()
        source = case / "source.c"
        payload = text.encode("ascii")
        source.write_bytes(payload)
        output = case / "output"
        argv = [args.compiler.resolve(), "--input", source,
                "--grammar", args.grammar.resolve(), "--output-dir", output,
                "--emit", "llvm" if args.clang else "ast"]
        if args.clang:
            argv.extend(["--opt-level", str(level)])
        result = command(argv, case, "compile", 1 if code else 0)
        require(source.read_bytes() == payload and args.grammar.read_bytes() == grammar_bytes,
                f"{name}: input bytes changed")
        require((output / "10-stack_file.txt").is_file(), f"{name}: original parser was bypassed")
        if code:
            stderr = result.stderr.decode("utf-8", errors="strict")
            require(f"error[{code}]" in stderr and detail in stderr,
                    f"{name}: wrong diagnostic {stderr!r}")
            for filename in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                require(not (output / filename).exists(), f"{name}: published {filename} after rejection")
        else:
            require(not result.stderr, f"{name}: unexpected stderr {result.stderr!r}")
            ast_text = (output / "15-ast.txt").read_text(encoding="utf-8", errors="strict")
            require("返回" in ast_text and ("声明" in ast_text or "赋值" in ast_text),
                    f"{name}: statements missing from AST")
            if not args.clang:
                require(not (output / "18-program.o").exists(), "AST mode invoked a backend")
        return case, output

    if not args.clang:
        for name, text, _ in POSITIVE:
            compile_source(name, text)
            print(f"PASS {name}：作用域与初始化检查通过", flush=True)
        for name, text, code, detail in NEGATIVE:
            compile_source(name, text, code=code, detail=detail)
            print(f"PASS {name}：目标诊断={code}，未发布AST或LLVM产物", flush=True)
        print(f"局部语义：{len(POSITIVE) + len(NEGATIVE)} 项用例通过")
        return

    for name, text, expected in POSITIVE:
        for level in [0, 2]:
            case, output = compile_source(f"{name}_O{level}", text, level)
            before = (output / "16-before.ll").read_text(encoding="utf-8", errors="strict")
            after = (output / "17-after.ll").read_text(encoding="utf-8", errors="strict")
            for opcode in ["alloca", "load", "store"]:
                require(re.search(rf"\b{opcode}\b", before), f"{name}: original IR has no {opcode}")
                if level == 2:
                    require(not re.search(rf"\b{opcode}\b", after),
                            f"{name}: promotable local access remains after O2: {opcode}")
                else:
                    require(re.search(rf"\b{opcode}\b", after),
                            f"{name}: O0 unexpectedly removed {opcode}")
            obj = output / "18-program.o"
            require(obj.is_file() and obj.stat().st_size > 0, f"{name}: no real object")
            harness = case / "harness.c"
            harness.write_text(
                "#include <stdio.h>\nextern int calc(int);\nint main(void)\n{\n"
                "    const int inputs[] = {-7, 0, 7, 31};\n"
                "    for(int i = 0; i < 4; ++i)\n    {\n"
                "        const int x = inputs[i];\n"
                f"        const int expected = {expected};\n"
                "        const int actual = calc(x);\n"
                "        if(actual != expected)\n        {\n"
                '            fprintf(stderr, "x=%d actual=%d expected=%d\\n", x, actual, expected);\n'
                "            return 1;\n        }\n    }\n    return 0;\n}\n",
                encoding="ascii")
            exe = case / "demo"
            command([args.clang, harness, obj, "-o", exe], case, "link")
            command([exe], case, "run")
            print(f"PASS {name}_O{level}：IR、原生目标文件及四组结果一致", flush=True)

    # 目标负例也进入 LLVM 模式，防止语义失败后仍生成后端产物。
    for name in ["uninitialized_read", "scope_leak"]:
        _, text, code, detail = next(case for case in NEGATIVE if case[0] == name)
        compile_source(name, text, level=2, code=code, detail=detail)
        print(f"PASS {name}：拒绝后未发布LLVM产物", flush=True)
    print(f"局部LLVM：{len(POSITIVE) * 2 + 2} 项用例通过；未作性能测量")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError, UnicodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
