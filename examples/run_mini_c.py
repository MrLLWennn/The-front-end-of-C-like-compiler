"""演示简易C源码经过本编译器和Clang链接后成为可执行程序，不自动构建。"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def stage(process, step):
    print(f"************------------ 过程：{process}----步骤：{step}------------************", flush=True)


def command(argv, directory, label):
    result = subprocess.run([str(value) for value in argv], cwd=directory,
                            capture_output=True, timeout=30)
    (directory / f"{label}.stdout.log").write_bytes(result.stdout)
    (directory / f"{label}.stderr.log").write_bytes(result.stderr)
    if result.stdout:
        print(result.stdout.decode("utf-8", errors="replace"), end="", flush=True)
    if result.stderr:
        print(result.stderr.decode("utf-8", errors="replace"), end="", file=sys.stderr, flush=True)
    if result.returncode != 0:
        raise RuntimeError(f"{label}: exit={result.returncode}; see {directory}")
    return result


def main():
    parser = argparse.ArgumentParser(description="编译、链接并运行所选简易C样例，保留每一步产物")
    parser.add_argument("compiler", type=Path)
    parser.add_argument("--opt-level", type=int, choices=[0, 2], default=2)
    parser.add_argument("--clang", default="clang-18")
    parser.add_argument("--example", choices=["basic", "stdio", "typed", "memory"], default="stdio")
    parser.add_argument("--output-root", type=Path, default=Path("build-mini-c-runs"))
    args = parser.parse_args()
    compiler = args.compiler.resolve(strict=True)
    clang = shutil.which(args.clang)
    if not clang:
        raise RuntimeError(f"cannot find linker driver: {args.clang}")
    root = Path(__file__).resolve().parents[1]
    examples = {"basic": "mini_c.c", "stdio": "mini_c_io.c", "typed": "typed_structs.c", "memory": "arrays_pointers_for.c"}
    source = root / "examples" / examples[args.example]
    grammar = root / "files" / "04-grammar.txt"
    original_source, original_grammar = source.read_bytes(), grammar.read_bytes()
    header = root / "examples" / ("typed_structs.h" if args.example == "typed" else "mini_c_math.h")
    original_header = header.read_bytes() if args.example in ["stdio", "typed"] else None
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix=f"O{args.opt_level}-", dir=args.output_root)).resolve()
    output = run / "output"
    print(f"本次产物目录：{run}", flush=True)

    command([compiler, "--input", source, "--grammar", grammar,
             "--output-dir", output, "--emit", "llvm", "--opt-level", args.opt_level], run, "compile")
    stage("链接", "由Clang组织启动代码、生成的目标文件与系统运行库")
    exe = run / "mini_c"
    command([clang, output / "18-program.o", "-o", exe], run, "link")
    print(f"可执行文件：{exe}\n链接成功\n{'-' * 78}", flush=True)

    stage("运行验证", "执行main并核对退出码与标准输出")
    actual = command([exe], run, "run")
    expected = {"basic": b"", "stdio": b"sum_squares(5) = 55\nstatus: ok\n",
                "typed": b"value=3.00 grade=A accepted=1\nscore=3.50\n",
                "memory": b"count=4 total=15.0 label=verified\n"}[args.example]
    if actual.stdout != expected:
        raise RuntimeError(f"unexpected program output: {actual.stdout!r}")
    print("程序退出码：0；标准输出与样例预期一致。", flush=True)

    stage("独立对照", "由Clang直接编译同一份C源码并运行")
    reference = run / "clang_reference"
    command([clang, "-std=c17", f"-O{args.opt_level}", source, "-o", reference], run, "reference_build")
    oracle = command([reference], run, "reference_run")
    if actual.stdout != oracle.stdout:
        raise RuntimeError("runtime output differs from Clang reference")
    if source.read_bytes() != original_source or grammar.read_bytes() != original_grammar:
        raise RuntimeError("input bytes changed")
    if original_header is not None and header.read_bytes() != original_header:
        raise RuntimeError("header bytes changed")
    (run / "result.json").write_text(json.dumps({
        "optimization": args.opt_level,
        "source_sha256": hashlib.sha256(original_source).hexdigest(),
        "compiler": str(compiler), "clang": clang,
        "object": str(output / "18-program.o"), "executable": str(exe),
        "actual_main_exit": 0, "reference_main_exit": 0,
        "example": args.example, "stdout": actual.stdout.decode("utf-8"),
        "checks": ["sum_squares(5)==55", "sum_squares(0)==0", "sum_squares(10)==385"]
                  if args.example == "basic" else ["sum_squares(5)==55", "printf output matches Clang"]
                  if args.example == "stdio" else ["typed structs and conversions", "printf output matches Clang"]
                  if args.example == "typed" else ["arrays, pointers and for", "printf output matches Clang"]
    }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("PASS：本编译器生成程序与Clang对照程序均通过自检；未测量性能。", flush=True)
    print(f"{'-' * 78}\nAST、优化前后IR、目标文件、可执行文件和日志：{run}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
