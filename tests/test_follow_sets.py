"""通过旧公开文法接口验证 FOLLOW 集。

夹具从带编号的 06-processed_grammar.txt 进入，
不覆盖文法预处理、词法分析、语法分析或树输出。
"""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

CASES = [
    ("normal_terminal", "1 <S> -> <B> int\n2 <B> -> char\n", {"int"}),
    ("nullable_then_terminal", "1 <S> -> <B> <C> int\n2 <B> -> char\n3 <C> -> empty\n", {"int"}),
    ("whole_suffix_nullable", "1 <S> -> <B> <C> <D>\n2 <B> -> char\n3 <C> -> empty\n4 <D> -> empty\n", {"$"}),
    ("indirect_nullable_suffix", "1 <S> -> <B> <C>\n2 <B> -> char\n3 <C> -> <D>\n4 <D> -> empty\n", {"$"}),
    (
        "mixed_nullable_suffix",
        "1 <S> -> <B> <C> <D>\n"
        "2 <B> -> char\n"
        "3 <C> -> int\n"
        "4 <C> -> empty\n"
        "5 <D> -> char\n"
        "6 <D> -> empty\n",
        {"int", "char", "$"},
    ),
]


def read_follow_b(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8")
    parts = text.split("非终结符号的 FOLLOW 集合：", 1)
    if len(parts) != 2:
        raise ValueError("FOLLOW section is missing")
    rows = [line for line in parts[1].splitlines() if line.lstrip().startswith("<B>")]
    if len(rows) != 1 or "：" not in rows[0]:
        raise ValueError("Expected one FOLLOW(<B>) row")
    return set(rows[0].split("：", 1)[1].split())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("probe", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="follow-", dir=args.output_root)).resolve()
    print(f"测试产物：{root}", flush=True)
    records = []
    for name, grammar, expected in CASES:
        run = root / name
        files, work = run / "files", run / "work"
        files.mkdir(parents=True)
        work.mkdir()
        (files / "06-processed_grammar.txt").write_text(grammar, encoding="ascii")
        record = {"case": name, "expected": sorted(expected), "grammar": grammar, "directory": str(run)}
        try:
            result = subprocess.run(
                [str(args.probe.absolute())], cwd=work,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=5,
            )
            (run / "run.log").write_bytes(result.stdout)
            record["probe_exit"] = result.returncode
            if result.returncode != 0:
                raise RuntimeError(f"Probe exit: {result.returncode}")
            actual = read_follow_b(files / "08-first_follow.txt")
            record["actual"] = sorted(actual)
            record["passed"] = actual == expected
            status = "PASS" if record["passed"] else "FAIL"
            print(f"{status} {name}：FOLLOW(<B>) 预期={sorted(expected)}，实际={sorted(actual)}")
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
            record.update(passed=False, error=str(error))
            print(f"FAIL {name}: {error}")
        records.append(record)
    (root / "follow-results.json").write_text(
        json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    failures = sum(not item["passed"] for item in records)
    print(f"用例：{len(records) - failures} 项通过，{failures} 项失败")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
