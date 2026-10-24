"""最小合法输入的黑盒回归。

只使用 Python 标准库；产物保留在构建目录，便于失败后检查。
该测试不证明全部语言特性均正确。
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("grammar", type=Path)
    parser.add_argument("dot", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()

    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="minimal-", dir=args.output_root)).resolve()
    files, work = run / "files", run / "work"
    files.mkdir()
    work.mkdir()
    print(f"测试产物：{run}", flush=True)

    (files / "00-source.txt").write_text("int a;\n", encoding="ascii")
    shutil.copyfile(args.grammar, files / "04-grammar.txt")
    with (run / "run.log").open("wb") as log:
        result = subprocess.run(
            [str(args.compiler.absolute())],
            cwd=work,
            stdout=log,
            stderr=subprocess.STDOUT,
            timeout=10,
        )
    require(result.returncode == 0, f"Compiler exit: {result.returncode}")

    token_bytes = (files / "03-lexer.txt").read_bytes()
    tokens = re.findall(rb'^([A-Za-z]+)\("([^"\r\n]*)"\)', token_bytes, re.MULTILINE)
    expected = [(b"Keyword", b"int"), (b"Identifier", b"a"), (b"Delimiter", b";")]
    require(tokens == expected, f"Unexpected tokens: {tokens!r}")

    # 合法输入仍需检查旧错误标记，并确认新生成的日志可按 UTF-8 严格解码。
    log_bytes = (run / "run.log").read_bytes()
    log_bytes.decode("utf-8")
    token_bytes.decode("utf-8")
    for marker in ("产生式出错", "发生错误，开始恢复"):
        require(
            marker.encode("utf-8") not in log_bytes,
            f"Unexpected parser diagnostic: {marker}",
        )

    dot_file = files / "12-grammatical_analysis_tree.dot"
    require(dot_file.is_file() and dot_file.stat().st_size > 0, "DOT output is missing or empty")
    svg = run / "tree.svg"
    graphviz = subprocess.run(
        [str(args.dot.absolute()), "-Tsvg", str(dot_file), "-o", str(svg)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        timeout=10,
    )
    require(graphviz.returncode == 0, f"Invalid DOT: {graphviz.stdout}")
    require(svg.is_file() and svg.stat().st_size > 0, "SVG output is missing or empty")
    print("PASS：最小输入、Token、分析日志、DOT 与 SVG 均符合预期")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
