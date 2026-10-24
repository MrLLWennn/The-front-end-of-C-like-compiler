"""错误输入不能伪装成成功运行的行为回归。
用户通过 CTest 执行；每个用例保留输入、标准输出和错误输出。
"""
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
    run = Path(tempfile.mkdtemp(prefix="diagnostics-", dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    grammar_before = args.grammar.read_bytes()
    final_names = ("11-grammatical_analysis_tree.txt", "12-grammatical_analysis_tree.dot", "13-symbol_table.txt")
    cases = [
        ("valid", "int a;\n", None, None),
        ("nearby_valid", "int b;\nint c;\n", None, None),
        ("missing_semicolon", "int a\n", "E_PARSE_EOF", "unexpected end of input"),
        ("missing_identifier", "int ;\n", "E_PARSE_TOKEN", "identifier"),
        ("missing_identifier_before_initializer", "int = 1;\n", "E_PARSE_TOKEN", "identifier"),
        ("missing_local_identifier", "int f(void) { int ; return 0; }\n", "E_PARSE_TOKEN", "identifier"),
        ("bad_number", "int 123abc;\n", "E_LEX_NUMBER", "123abc"),
        ("unknown_character", "int a; @\n", "E_LEX_CHAR", "64"),
        ("empty", "", "E_PARSE_EMPTY", "empty"),
        ("whitespace_only", " \t\r\n \t\n", "E_PARSE_EMPTY", "empty"),
        ("block_comment_only", "/* comment only */\n", "E_PARSE_EMPTY", "empty"),
        ("line_comment_only", "// comment only", "E_PARSE_EMPTY", "empty"),
        ("nul_byte", "\x00", "E_LEX_CHAR", "byte 0"),
        ("leading_identifier", "orphan;\n", "E_PARSE_TABLE", "no production"),
    ]
    for name, source_text, code, detail in cases:
        case = run / name
        case.mkdir()
        source = case / "source.c"
        # 精确保留输入字节，包括 CRLF 和嵌入的 NUL。
        source_bytes = source_text.encode("ascii")
        source.write_bytes(source_bytes)
        output = case / "output"
        result = subprocess.run(
            [str(args.compiler.resolve()), "--input", str(source),
             "--grammar", str(args.grammar.resolve()), "--output-dir", str(output)],
            cwd=case, capture_output=True, timeout=10)
        (case / "stdout.log").write_bytes(result.stdout)
        (case / "stderr.log").write_bytes(result.stderr)
        require(source.read_bytes() == source_bytes, f"{name}: source bytes changed")
        require(args.grammar.read_bytes() == grammar_before, f"{name}: grammar changed")
        expected_exit = 1 if code else 0
        require(result.returncode == expected_exit,
                f"{name}: expected exit {expected_exit}, got {result.returncode}; {result.stderr!r}")
        if code:
            stderr = result.stderr.decode("utf-8", errors="replace")
            require(f"error[{code}]" in stderr and detail in stderr,
                    f"{name}: wrong diagnostic: {stderr!r}")
            require("02-pretreatment.txt" in stderr, f"{name}: misleading location file")
            if code == "E_PARSE_EMPTY":
                require((output / "02-pretreatment.txt").read_bytes() == b"",
                        f"{name}: preprocessing invented source bytes")
            for filename in final_names:
                require(not (output / filename).exists(), f"{name}: published {filename} after failure")
            if code.startswith("E_LEX"):
                require(not (output / "10-stack_file.txt").exists(), f"{name}: parser ran after lexer failure")
        else:
            require(not result.stderr, f"{name}: unexpected stderr: {result.stderr!r}")
            for filename in final_names:
                artifact = output / filename
                require(artifact.is_file() and artifact.stat().st_size > 0,
                        f"{name}: missing {filename}")
        print(f"PASS {name}：退出码={result.returncode}，诊断={code or '无'}", flush=True)
    print(f"用例：{len(cases)} 项通过")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
