"""命令行集成契约，由用户通过 CTest 执行。

覆盖驱动和文件处理行为，不证明任意源程序均能正确编译。
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import subprocess
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
    compiler, grammar = args.compiler.resolve(), args.grammar.resolve()
    args.output_root.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="cli-", dir=args.output_root)).resolve()
    work = root / "unrelated working directory"
    work.mkdir()
    source = root / "source file.c"
    source.write_text("int a;\n", encoding="ascii")
    source.chmod(0o444)
    source_before = source.read_bytes()
    grammar_before = grammar.read_bytes()
    output = root / "generated output"
    print(f"测试产物：{root}", flush=True)

    def run(label, options, expected, diagnostic=None):
        result = subprocess.run([str(compiler), *map(str, options)], cwd=work,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=12)
        (root / (label + ".stdout.log")).write_bytes(result.stdout)
        (root / (label + ".stderr.log")).write_bytes(result.stderr)
        require(result.returncode == expected,
                f"{label}: expected exit {expected}, actual {result.returncode}")
        if diagnostic:
            require(diagnostic.encode() in result.stderr,
                    f"{label}: missing specific diagnostic {diagnostic!r}")
        print(f"PASS {label}：退出码={result.returncode}", flush=True)
        return result

    options = ["--input", source, "--grammar", grammar, "--output-dir", output]
    help_result = run("help", ["--help"], 0)
    require(b"--input" in help_result.stdout and b"--output-dir" in help_result.stdout,
            "help must describe the supported interface")
    run("unknown_option", ["--not-an-option"], 2, "unknown option")
    run("missing_value", ["--input"], 2, "missing value")
    run("missing_required", ["--input", source], 2, "required option")
    run("duplicate_option", [*options, "--input", source], 2, "duplicate option")
    require(not output.exists(), "usage errors must not create run outputs")
    run("missing_source", ["--input", root / "absent.c", "--grammar", grammar,
                           "--output-dir", output], 1, "source is not a regular file")
    run("missing_grammar", ["--input", source, "--grammar", root / "absent.grammar",
                            "--output-dir", output], 1, "grammar is not a regular file")
    require(not output.exists(), "bad inputs must fail before outputs are prepared")

    # 确认失败确由输入输出重叠触发，而非其他无效参数。
    collision = root / "collision"
    collision.mkdir()
    protected_source = collision / "01-uncomment.txt"
    protected_source.write_text("int a;\n", encoding="ascii")
    protected_before = protected_source.read_bytes()
    run("source_output_collision", ["--input", protected_source, "--grammar", grammar,
                                    "--output-dir", collision], 2,
        "output would overwrite an input file")
    require(protected_source.read_bytes() == protected_before, "input was overwritten")
    require(not (collision / "02-pretreatment.txt").exists(),
            "collision check must precede all generated writes")

    hardlinks = root / "hardlink collision"
    hardlinks.mkdir()
    (hardlinks / "01-uncomment.txt").hardlink_to(source)
    run("hardlink_collision", ["--input", source, "--grammar", grammar,
                               "--output-dir", hardlinks], 2,
        "output would overwrite an input file")
    require(source.read_bytes() == source_before, "hardlinked input was overwritten")

    blocked = root / "not a directory"
    blocked.write_text("keep", encoding="ascii")
    run("output_parent_is_file", ["--input", source, "--grammar", grammar,
                                  "--output-dir", blocked], 1)
    require(blocked.read_text() == "keep", "unrelated output-parent file changed")

    run("valid_explicit_paths", options, 0)
    token_bytes = (output / "03-lexer.txt").read_bytes()
    tokens = re.findall(rb'^([A-Za-z]+)\("([^"\r\n]*)"\)', token_bytes, re.MULTILINE)
    require(tokens == [(b"Keyword", b"int"), (b"Identifier", b"a"), (b"Delimiter", b";")],
            f"explicit-path output tokens differ: {tokens!r}")
    require((output / "12-grammatical_analysis_tree.dot").stat().st_size > 0,
            "explicit-path run did not produce DOT")
    require(source.read_bytes() == source_before and grammar.read_bytes() == grammar_before,
            "source or grammar changed")
    require(not (root / "files").exists(), "run still depended on ../files")

    # 两个独立进程不能混用输出文件。
    def isolated_job(index):
        inp = root / f"job-{index}.c"
        name = "a" if index == 1 else "b"
        inp.write_text(f"int {name};\n", encoding="ascii")
        out = root / f"job-{index}-out"
        run(f"isolated_job_{index}", ["--input", inp, "--grammar", grammar,
                                     "--output-dir", out], 0)
        require(f'Identifier("{name}")'.encode() in (out / "03-lexer.txt").read_bytes(),
                f"job {index} consumed another run's input")
    with ThreadPoolExecutor(max_workers=2) as executor:
        list(executor.map(isolated_job, (1, 2)))
    print("PASS：命令行路径、只读输入、失败原因与进程隔离均符合预期")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}")
        raise SystemExit(1)
