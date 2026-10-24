"""UR10：分支合流、作用域及 O0/O2 原生调用回归，由用户执行。"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


POSITIVE = [
    ("both_assign", "int calc(int flag, int x) { int y; if(flag) { y = x + 1; } else { y = x + 2; } return y; }\n", "flag ? x + 1 : x + 2"),
    ("without_else", "int calc(int flag, int x) { int y = x; if(flag) { y = y + 3; } return y; }\n", "flag ? x + 3 : x"),
    ("empty_then", "int calc(int flag, int x) { int y = x; if(flag) { } else { y = x + 2; } return y; }\n", "flag ? x : x + 2"),
    ("shadow_preserves_outer", "int calc(int flag, int x) { int y = x; if(flag) { int y = 9; y = y + 1; } else { y = y + 2; } return y; }\n", "flag ? x : x + 2"),
    ("separate_branch_locals", "int calc(int flag, int x) { int y; if(flag) { int z = x + 1; y = z; } else { int z = x + 2; y = z; } return y; }\n", "flag ? x + 1 : x + 2"),
    ("nested_else", "int calc(int flag, int x) { int y; if(flag) { if(x) { y = 11; } else { y = 12; } } else { y = 13; } return y; }\n", "flag ? (x ? 11 : 12) : 13"),
    ("nested_no_else", "int calc(int flag, int x) { int y = x; if(flag) { if(x) { y = y + 1; } } return y; }\n", "flag && x ? x + 1 : x"),
    ("sequential_conditions", "int calc(int flag, int x) { int y = x; if(flag) { y = y + 1; } if(x) { y = y + 2; } return y; }\n", "x + (flag ? 1 : 0) + (x ? 2 : 0)"),
    ("arithmetic_condition", "int calc(int flag, int x) { int y; if(flag + 1) { y = x + 1; } else { y = x + 2; } return y; }\n", "flag + 1 ? x + 1 : x + 2"),
    ("parameter_assignment", "int calc(int flag, int x) { if(flag) { x = x + 1; } else { x = x + 2; } return x; }\n", "flag ? x + 1 : x + 2"),
]

NEGATIVE = [
    ("then_only", "int calc(int flag, int x) { int y; if(flag) { y = x; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("else_only", "int calc(int flag, int x) { int y; if(flag) { } else { y = x; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("cross_path_read", "int calc(int flag, int x) { int y; if(flag) { y = 1; } else { y = y + 1; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("uninitialized_condition", "int calc(int flag, int x) { int y; if(y) { y = 1; } else { y = 2; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("shadow_does_not_assign_outer", "int calc(int flag, int x) { int y; if(flag) { int y = 1; } else { y = 2; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("branch_name_leak", "int calc(int flag, int x) { if(flag) { int z = 1; } else { } return z; }\n", "E_SEMA_NAME", "undeclared name 'z'"),
    ("cross_branch_name", "int calc(int flag, int x) { int y; if(flag) { int z = 1; y = z; } else { y = z; } return y; }\n", "E_SEMA_NAME", "undeclared name 'z'"),
    ("undeclared_target", "int calc(int flag, int x) { if(flag) { ghost = 1; } else { } return x; }\n", "E_SEMA_NAME", "undeclared name 'ghost'"),
    ("nested_missing_path", "int calc(int flag, int x) { int y; if(flag) { if(x) { y = 1; } } else { y = 2; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("duplicate_branch_local", "int calc(int flag, int x) { if(flag) { int y = 1; int y = 2; } return x; }\n", "E_SEMA_LOCAL", "duplicate local name 'y'"),
    ("shadow_self_init", "int calc(int flag, int x) { int y = x; if(flag) { int y = y; } return y; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'y'"),
    ("unbraced_then", "int calc(int flag, int x) { if(flag) x = x + 1; return x; }\n", "E_AST_BRANCH", "braced block after if condition"),
    ("unbraced_else", "int calc(int flag, int x) { if(flag) { x = x + 1; } else x = x + 2; return x; }\n", "E_AST_BRANCH", "braced block after else"),
    ("return_inside_branch", "int calc(int flag, int x) { if(flag) { return x; } return x; }\n", "E_AST_STATEMENT", "lexical block"),
]

NATIVE_NEGATIVES = ["then_only", "cross_path_read", "branch_name_leak", "nested_missing_path"]
FLAGS = [-3, -1, 0, 5]
INPUTS = [-7, 0, 7, 31]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(argv, case, label, expected=0):
    result = subprocess.run([str(x) for x in argv], cwd=case, capture_output=True, timeout=15)
    (case / f"{label}.stdout.log").write_bytes(result.stdout)
    (case / f"{label}.stderr.log").write_bytes(result.stderr)
    require(result.returncode == expected,
            f"{case.name}: {label} expected exit {expected}, got {result.returncode}; {result.stderr!r}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("grammar", type=Path)
    parser.add_argument("output_root", type=Path)
    parser.add_argument("--clang", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="branches-native-" if args.clang else "branches-ast-",
                               dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_bytes = args.grammar.read_bytes()
    records = []

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
        stack = output / "10-stack_file.txt"
        require(stack.is_file(), f"{name}: original parser was bypassed")
        require("预测分析结束" in stack.read_text(encoding="utf-8"),
                f"{name}: original parser did not complete")
        if code:
            stderr = result.stderr.decode("utf-8", errors="strict")
            require(f"error[{code}]" in stderr and detail in stderr,
                    f"{name}: wrong diagnostic {stderr!r}")
            for filename in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                require(not (output / filename).exists(), f"{name}: published {filename} after rejection")
        else:
            require(not result.stderr, f"{name}: unexpected stderr {result.stderr!r}")
            tree = (output / "15-ast.txt").read_text(encoding="utf-8", errors="strict")
            require("条件分支" in tree and "成立分支" in tree and "返回" in tree,
                    f"{name}: branch AST is missing")
            if not args.clang:
                require(not (output / "18-program.o").exists(), f"{name}: AST mode invoked backend")
        return case, output

    if not args.clang:
        for name, text, _ in POSITIVE:
            compile_source(name, text)
            records.append({"case": name, "accepted": True})
            print(f"PASS {name}：分支、作用域与合流检查通过", flush=True)
        for name, text, code, detail in NEGATIVE:
            compile_source(name, text, code=code, detail=detail)
            records.append({"case": name, "accepted": False, "diagnostic": code})
            print(f"PASS {name}：目标诊断={code}，未发布最终产物", flush=True)
        (run / "results.json").write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n")
        print(f"分支语义：{len(POSITIVE) + len(NEGATIVE)} 项用例通过")
        return

    for name, text, expected in POSITIVE:
        for level in [0, 2]:
            case, output = compile_source(f"{name}_O{level}", text, level)
            before = (output / "16-before.ll").read_text(encoding="utf-8", errors="strict")
            after = (output / "17-after.ll").read_text(encoding="utf-8", errors="strict")
            require("icmp ne i32" in before and "br i1" in before,
                    f"{name}: original IR has no integer conditional branch")
            if level == 0:
                require("br i1" in after and re.search(r"\balloca\b", after),
                        f"{name}: O0 unexpectedly removed branch/storage")
            else:
                for opcode in ["alloca", "load", "store"]:
                    require(not re.search(rf"\b{opcode}\b", after),
                            f"{name}: promotable storage remains after O2: {opcode}")
            # O2 可以生成 phi、select 或其他等价形式，不绑定某一种合法形状。
            obj = output / "18-program.o"
            require(obj.is_file() and obj.stat().st_size > 0, f"{name}: no real object")
            harness = case / "harness.c"
            harness.write_text(
                "#include <stdio.h>\nextern int calc(int, int);\nint main(void)\n{\n"
                "    const int flags[] = {-3, -1, 0, 5};\n"
                "    const int inputs[] = {-7, 0, 7, 31};\n"
                "    for(int i = 0; i < 4; ++i)\n    {\n"
                "        for(int j = 0; j < 4; ++j)\n        {\n"
                "            const int flag = flags[i];\n"
                "            const int x = inputs[j];\n"
                f"            const int expected = ({expected});\n"
                "            const int actual = calc(flag, x);\n"
                "            if(actual != expected)\n            {\n"
                '                fprintf(stderr, "flag=%d x=%d actual=%d expected=%d\\n", flag, x, actual, expected);\n'
                "                return 1;\n            }\n        }\n    }\n"
                '    puts("PASS: 16 input combinations matched");\n'
                "    return 0;\n}\n", encoding="ascii")
            exe = case / "demo"
            command([args.clang, harness, obj, "-o", exe], case, "link")
            command([exe], case, "run")
            records.append({"case": name, "optimization": level, "flag_values": FLAGS,
                            "input_values": INPUTS, "runtime_combinations_passed": 16})
            print(f"PASS {name}_O{level}：IR有效，16组原生调用结果一致", flush=True)

    for name in NATIVE_NEGATIVES:
        _, text, code, detail = next(case for case in NEGATIVE if case[0] == name)
        compile_source(name, text, level=2, code=code, detail=detail)
        records.append({"case": name, "accepted": False, "diagnostic": code})
        print(f"PASS {name}：LLVM模式拒绝且无最终产物", flush=True)
    (run / "results.json").write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n")
    print(f"分支LLVM：{len(POSITIVE) * 2 + len(NATIVE_NEGATIVES)} 项用例通过；"
          f"{len(POSITIVE) * 2 * len(FLAGS) * len(INPUTS)} 次原生调用，未作性能测量")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError, UnicodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
