"""预测表的定向回归，夹具从预处理后的文法进入。
旧 Parser 删除显式 empty 选项后走回退；非空右部不能借用可空后缀的 FOLLOW。
另验证符号分类依据完整产生式左部集合，并独立于手写展示类型表。
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

CASES = [
    ("nonnullable_prefix_nullable_suffix",
     "1 <S> -> <A> ;\n2 <A> -> * <A>\n3 <A> -> empty\n",
     {";": [], "*": ["<A> -> * <A>"]}),
    ("nullable_prefix_then_required_terminal",
     "1 <S> -> <A> ;\n2 <A> -> <B> int\n3 <B> -> empty\n",
     {";": [], "int": ["<A> -> <B> int"]}),
    ("whole_rhs_nullable",
     "1 <S> -> <A> ;\n2 <A> -> <B> <C>\n3 <B> -> empty\n4 <C> -> int\n5 <C> -> empty\n",
     {";": ["<A> -> <B> <C>"]}),
    ("later_terminal_is_not_a_start",
     "1 <S> -> <A> ;\n2 <A> -> int number\n",
     {"number": [], "int": ["<A> -> int number"]}),
    ("continue_in_grammar",
     "1 <S> -> <A> ;\n2 <A> -> continue\n",
     {"continue": ["<A> -> continue"]}),
    ("unregistered_terminal_char",
     "1 <S> -> <A> ;\n2 <A> -> char\n",
     {"char": ["<A> -> char"]}),
    ("unregistered_terminal_custom",
     "1 <S> -> <A> ;\n2 <A> -> custom_token\n",
     {"custom_token": ["<A> -> custom_token"]}),
    ("primed_rhs_defined_later",
     "1 <S> -> <A> ;\n2 <A> -> <A>'\n3 <A>' -> int\n",
     {"int": ["<A> -> <A>'"]}),
    ("double_primed_rhs",
     "1 <S> -> <A> ;\n2 <A> -> <A>''\n3 <A>'' -> char\n",
     {"char": ["<A> -> <A>''"]}),
    ("primed_nullable_prefix",
     "1 <S> -> <A> ;\n2 <A> -> <A>' int\n3 <A>' -> empty\n",
     {";": [], "int": ["<A> -> <A>' int"]}),
    ("lhs_definition_without_brackets",
     "1 <S> -> <A> ;\n2 <A> -> Tail\n3 Tail -> number\n",
     {"number": ["<A> -> Tail"]}),
    ("less_than_is_a_terminal",
     "1 <S> -> <A> ;\n2 <A> -> <\n",
     {"<": ["<A> -> <"]}),
]


# 从04原始文法进入，覆盖符号边界、部分分组和空后缀，而非仅测06后的表。
RAW_CASES = [
    ("raw_identifier_boundary",
     "<S> -> <A> ;\n<A> -> identifier ; | identifier1 = number ;\n", "<A>",
     {"identifier": ["<A> -> identifier ;"], "identifier1": ["<A> -> identifier1 = number ;"], "1": []}),
    ("raw_shared_type_prefix",
     "<S> -> <A> ;\n<A> -> int identifier ; | int identifier1 = number ;\n", "<A>'",
     {"identifier": ["<A>' -> identifier ;"], "identifier1": ["<A>' -> identifier1 = number ;"], "1": []}),
    ("raw_primed_symbol_boundary",
     "<S> -> <A> ;\n<A> -> <X> ; | <X>' number ;\n<X> -> int\n<X>' -> char\n", "<A>",
     {"int": ["<A> -> <X> ;"], "char": ["<A> -> <X>' number ;"]}),
    ("raw_operator_boundary",
     "<S> -> <A> ;\n<A> -> + identifier ; | += number ;\n", "<A>",
     {"+": ["<A> -> + identifier ;"], "+=": ["<A> -> += number ;"], "=": []}),
    ("raw_whole_alternative_prefix",
     "<S> -> <A> ;\n<A> -> int | int number\n", "<A>'",
     {";": [], "number": ["<A>' -> number"]}),
    ("raw_whitespace_boundaries",
     "<S> -> <A> ;\n<A> -> int    identifier ; | int\tidentifier1 = number ;\n", "<A>'",
     {"identifier": ["<A>' -> identifier ;"], "identifier1": ["<A>' -> identifier1 = number ;"]}),
    ("raw_subset_prefix_group",
     "<S> -> <A> ;\n<A> -> int identifier ; | int identifier [ number ] ; | int identifier1 = number ;\n", "<A>'",
     {"identifier": ["<A>' -> identifier <A>''"], "identifier1": ["<A>' -> identifier1 = number ;"]}),
    ("raw_subset_tail_choices",
     "<S> -> <A> ;\n<A> -> int identifier ; | int identifier [ number ] ; | int identifier1 = number ;\n", "<A>''",
     {";": ["<A>'' -> ;"], "[": ["<A>'' -> [ number ] ;"]}),
    ("raw_fresh_symbol_collision",
     "<S> -> <A> ;\n<A> -> int identifier ; | int number ;\n<A>' -> char\n", "<A>''",
     {"identifier": ["<A>'' -> identifier ;"], "number": ["<A>'' -> number ;"]}),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("probe", type=Path)
    parser.add_argument("output_root", type=Path)
    args = parser.parse_args()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="prediction-", dir=args.output_root)).resolve()
    print(f"测试产物：{run}", flush=True)
    records = []
    cases = [(name, grammar, "<A>", expected, False) for name, grammar, expected in CASES]
    cases.extend((name, grammar, row, expected, True) for name, grammar, row, expected in RAW_CASES)
    for name, grammar, row, expected, raw_grammar in cases:
        case = run / name
        files, cwd = case / "files", case / "work"
        files.mkdir(parents=True)
        cwd.mkdir()
        source = files / ("04-grammar.txt" if raw_grammar else "06-processed_grammar.txt")
        payload = grammar.encode("ascii")
        source.write_bytes(payload)
        argv = [str(args.probe.resolve())]
        if raw_grammar:
            argv.append("--raw")
        result = subprocess.run([*argv, row, *expected], cwd=cwd, capture_output=True, timeout=5)
        (case / "stdout.log").write_bytes(result.stdout)
        (case / "stderr.log").write_bytes(result.stderr)
        if result.returncode != 0:
            raise RuntimeError(f"{name}: probe exit {result.returncode}; {result.stderr!r}")
        if source.read_bytes() != payload:
            raise RuntimeError(f"{name}: input bytes changed")
        # 文法统计、FIRST/FOLLOW、预测表都必须输出合法 UTF-8。
        for filename in ("07-Vn_Vt.txt", "08-first_follow.txt", "09-predictive_analysis_table.txt"):
            (files / filename).read_text(encoding="utf-8")
        if name.startswith("unregistered_terminal_"):
            labels = (files / "07-Vn_Vt.txt").read_text(encoding="utf-8")
            if "文法终结符（未登记类型）" not in labels:
                raise RuntimeError(f"{name}: fallback display label is missing")
        if raw_grammar:
            transformed = (files / "05-LL(1)_grammar.txt").read_text(encoding="utf-8")
            (files / "06-processed_grammar.txt").read_text(encoding="utf-8")
            if name == "raw_whole_alternative_prefix":
                require_empty = "<A>' -> empty | number"
                if require_empty not in transformed:
                    raise RuntimeError(f"{name}: complete-prefix empty suffix was lost: {transformed!r}")
        actual = {token: [] for token in expected}
        observed = set()
        for line in result.stdout.splitlines():
            parts = line.split(b"\t")
            if parts[0] not in (b"TABLE_ENTRY", b"TABLE_EMPTY"):
                continue
            if parts[1].decode("ascii") != row:
                raise RuntimeError(f"{name}: wrong table row {parts[1]!r}")
            token = parts[2].decode("ascii")
            observed.add(token)
            if parts[0] == b"TABLE_ENTRY":
                actual[token].append(" ".join(parts[3].decode("ascii").split()))
        if observed != set(expected) or actual != expected:
            raise RuntimeError(f"{name}: expected={expected!r}, actual={actual!r}, observed={observed!r}")
        records.append({"case": name, "expected": expected, "actual": actual})
        print(f"PASS {name}：{actual}", flush=True)
    (run / "table-results.json").write_text(json.dumps(records, indent=2) + "\n")
    print(f"预测表：{len(cases)} 组文法、{sum(len(case[3]) for case in cases)} 个表项通过；含{len(RAW_CASES)}组原始文法预处理")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
