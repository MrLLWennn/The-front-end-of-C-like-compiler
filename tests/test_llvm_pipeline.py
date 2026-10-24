"""由用户执行源码 → 原前端 → LLVM → .o → 原生程序的回归。
必须使用本编译器实际生成的 IR 和目标文件，不能替换成手写 IR。
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile


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
    parser.add_argument("clang", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="llvm-", dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_bytes = args.grammar.read_bytes()

    def compile_source(name, text, level=0, expected=0, code=None):
        case = run / name
        case.mkdir()
        source = case / "source.c"
        source_bytes = text.encode("ascii")
        source.write_bytes(source_bytes)
        output = case / "output"
        result = command([args.compiler.resolve(), "--input", source,
            "--grammar", args.grammar.resolve(), "--output-dir", output,
            "--emit", "llvm", "--opt-level", str(level)], case, "compile", expected)
        require(source.read_bytes() == source_bytes and args.grammar.read_bytes() == grammar_bytes,
                f"{name}: input bytes changed")
        if code:
            require(f"error[{code}]".encode() in result.stderr,
                    f"{name}: wrong rejection {result.stderr!r}")
            for filename in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                require(not (output / filename).exists(), f"{name}: published {filename} after failure")
        else:
            require(not result.stderr, f"{name}: unexpected compiler stderr {result.stderr!r}")
            for filename in ["15-ast.txt", "16-before.ll", "17-after.ll", "18-program.o"]:
                p = output / filename
                require(p.is_file() and p.stat().st_size > 0, f"{name}: missing {filename}")
            require((output / "10-stack_file.txt").is_file(), "original parser was bypassed")
        return case, output

    case, output = compile_source("return42", "int main(void) { return 42; }\n")
    require("整数 42" in (output / "15-ast.txt").read_text(), "source literal absent from AST")
    require("ret i32 42" in (output / "16-before.ll").read_text(), "source return absent from IR")
    exe = case / "program"
    command([args.clang, output / "18-program.o", "-o", exe], case, "link")
    command([exe], case, "run", expected=42) # 此处是程序返回值；编译器本身应成功退出。
    print("PASS source_main：生成的原生程序返回 42", flush=True)

    for level in [0, 2]:
        case, output = compile_source(f"identity_O{level}", "int simplify(int x) { return x + 0; }\n", level)
        before = (output / "16-before.ll").read_text()
        after = (output / "17-after.ll").read_text()
        require(re.search(r"\badd i32\b", before), "IRBuilder folded away the optimization fixture")
        if level == 2:
            require(not re.search(r"\badd i32\b", after) and re.search(r"\bret i32 %x\b", after),
                    "O2 did not simplify x + 0 to x")
        else:
            require(re.search(r"\badd i32\b", after), "O0 unexpectedly ran the optimizing pipeline")
        harness = case / "harness.c"
        harness.write_text("#include <stdint.h>\nextern int32_t simplify(int32_t);\n"
            "int main(void) { int32_t xs[] = {-7,0,7,31}; for(int i=0;i<4;++i)"
            "if(simplify(xs[i])!=xs[i]) return 1; return 0; }\n", encoding="ascii")
        exe = case / "identity"
        command([args.clang, harness, output / "18-program.o", "-o", exe], case, "link")
        command([exe], case, "run")
        print(f"PASS identity_O{level}：目标文件在 -7、0、7、31 四组输入上结果正确", flush=True)

    case, output = compile_source("precedence", "int calc(int x, int y) { return x + y * 2; }\n", 2)
    harness = case / "harness.c"
    harness.write_text("#include <stdint.h>\nextern int32_t calc(int32_t,int32_t);\n"
        "int main(void) { int32_t xs[]={-7,0,5,31}, ys[]={3,0,-4,9};"
        "for(int i=0;i<4;++i) if(calc(xs[i],ys[i])!=xs[i]+ys[i]*2) return 1; return 0; }\n",
        encoding="ascii")
    exe = case / "precedence"
    command([args.clang, harness, output / "18-program.o", "-o", exe], case, "link")
    command([exe], case, "run")
    print("PASS precedence：AST、IR 与目标文件保留 x + y * 2 的语义", flush=True)

    compile_source("semantic_rejection", "int f(int x) { return y; }\n", expected=1, code="E_SEMA_NAME")
    print("PASS semantic_rejection：未声明名字报错后没有发布 LLVM 产物", flush=True)

    case = run / "ir_input_alias"
    case.mkdir()
    source = case / "16-before.ll"
    payload = b"int main(void) { return 42; }\n"
    source.write_bytes(payload)
    result = command([args.compiler.resolve(), "--input", source, "--grammar", args.grammar.resolve(),
        "--output-dir", case, "--emit", "llvm"], case, "compile", expected=2)
    require(b"overwrite an input" in result.stderr and source.read_bytes() == payload,
            "LLVM destination alias was not safely rejected")
    require(not (case / "01-uncomment.txt").exists(), "alias rejection happened after preprocessing")
    print("PASS ir_input_alias：新增产物路径也参与输入保护", flush=True)
    print("LLVM 编译链：6 项用例通过；未据此主张性能提升")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
