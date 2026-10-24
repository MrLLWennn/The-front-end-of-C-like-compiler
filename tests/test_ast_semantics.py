"""原词法器、语法器、带类型 AST 与名字检查的集成回归，由用户执行。"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("grammar", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="ast-", dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_bytes = args.grammar.read_bytes()
    cases = [
        ("constant", "int main(void) { return 42; }\n", None, "整数 42 : i32"),
        ("precedence", "int calc(int x, int y) { return x + y * 2; }\n", None, "二元运算 * : i32"),
        ("undeclared", "int f(int x) { return y; }\n", "E_SEMA_NAME", "undeclared name 'y'"),
        ("duplicate_parameter", "int f(int x, int x) { return x; }\n", "E_SEMA_PARAMETER", "duplicate parameter"),
        ("duplicate_function", "int f(void) { return 1; } int f(void) { return 2; }\n", "E_SEMA_FUNCTION", "duplicate function"),
        ("return_type", "float f(void) { return 1; }\n", "E_AST_TYPE", "int return type"),
        ("fraction", "int f(void) { return 1.5; }\n", "E_AST_LITERAL", "decimal literal"),
        ("literal_range", "int f(void) { return 2147483648; }\n", "E_AST_LITERAL", "decimal literal"),
        ("uninitialized_local", "int f(void) { int x; return x; }\n", "E_SEMA_UNINITIALIZED", "uninitialized name 'x'"),
    ]
    for name, source_text, code, detail in cases:
        case = run / name
        case.mkdir()
        source = case / "source file.c"
        source_bytes = source_text.encode("ascii")
        source.write_bytes(source_bytes)
        output = case / "output"
        result = subprocess.run([str(args.compiler.resolve()), "--input", str(source),
            "--grammar", str(args.grammar.resolve()), "--output-dir", str(output), "--emit", "ast"],
            cwd=case, capture_output=True, timeout=10)
        (case / "stdout.log").write_bytes(result.stdout)
        (case / "stderr.log").write_bytes(result.stderr)
        require(source.read_bytes() == source_bytes and args.grammar.read_bytes() == grammar_bytes,
                f"{name}: input bytes changed")
        require(result.returncode == (1 if code else 0),
                f"{name}: wrong exit {result.returncode}; {result.stderr!r}")
        ast_file = output / "15-ast.txt"
        if code:
            stderr = result.stderr.decode("utf-8", errors="replace")
            require(f"error[{code}]" in stderr and detail in stderr,
                    f"{name}: wrong diagnostic: {stderr!r}")
            require(not ast_file.exists(), f"{name}: published AST after semantic rejection")
        else:
            require(not result.stderr, f"{name}: unexpected stderr {result.stderr!r}")
            text = ast_file.read_text(encoding="utf-8")
            require(detail in text and "返回" in text, f"{name}: wrong AST {text!r}")
            require((output / "10-stack_file.txt").is_file(), f"{name}: original parser was bypassed")
            if name == "precedence":
                require(text.index("二元运算 +") < text.index("二元运算 *"), "wrong precedence tree")
        require(not (output / "18-program.o").exists(), f"{name}: AST mode ran a backend")
        print(f"PASS {name}：诊断={code or '无'}", flush=True)
    print(f"AST 用例：{len(cases)} 项通过")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
