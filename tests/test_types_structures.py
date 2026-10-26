"""CP04：多类型、结构体及标量ABI；由用户运行，前后端均保留原Parser检查。"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

PRELUDE = '#include <stdio.h>\n#include <stdbool.h>\n'
POSITIVE = [
    ('float_function', 'float scale(float x) { return x * 0.5f; } int main(void) { printf("%.3f\\n", scale(3)); return 0; }', '1.500\n'),
    ('double_function', 'double adjust(double x, int y) { return x * 1.25 + y; } int main(void) { printf("%.2f\\n", adjust(2.0, 3)); return 0; }', '5.50\n'),
    ('char_function', "char next(char x) { return x + 1; } int main(void) { printf(\"%c\\n\", next('A')); return 0; }", 'B\n'),
    ('bool_function', 'bool negative(double x) { return x < 0.0; } int main(void) { printf("%d %d\\n", negative(-0.5), negative(0.0)); return 0; }', '1 0\n'),
    ('void_return', 'void show(int n) { printf("n=%d\\n", n); return; } int main(void) { show(7); return 0; }', 'n=7\n'),
    ('void_fallthrough', 'void show(void) { printf("ready\\n"); } int main(void) { show(); return 0; }', 'ready\n'),
    ('typed_prototype', 'double blend(float a, char b); int main(void) { printf("%.1f\\n", blend(1.5f, 2)); return 0; } double blend(float x, char y) { return x + y; }', '3.5\n'),
    ('implicit_conversions', 'int main(void) { double d=3; float f=d; char c=65; bool b=-0.5; int n=3.75; printf("%.1f %.1f %c %d %d\\n", d, f, c, b, n); return 0; }', '3.0 3.0 A 1 3\n'),
    ('explicit_casts', 'int main(void) { double x=(double)3; int y=(int)2.75; char c=(char)66; printf("%.1f %d %c\\n", x, y, c); return 0; }', '3.0 2 B\n'),
    ('literal_forms', 'int main(void) { float f=1.25e1F; double d=.5 + 2.5e-1; printf("%.2f %.2f\\n", f, d); return 0; }', '12.50 0.75\n'),
    ('floating_comparisons', 'int main(void) { printf("%d %d %d %d %d %d\\n", 1.5<2, 2.0<=2, 3.0>2, 2.0>=2, 2.0==2, 2.0!=2); return 0; }', '1 1 1 1 1 0\n'),
    ('floating_while', 'int main(void) { double x=2.0; int n=0; while(x) { n=n+1; x=x-0.5; } printf("%d\\n", n); return 0; }', '4\n'),
    ('typed_shadow', 'int main(void) { double x=1.5; { char x=65; printf("%c ", x); } printf("%.1f\\n", x); return 0; }', 'A 1.5\n'),
    ('char_escapes', r"int main(void) { char a='\n'; char b='\''; char c='\\'; char z='\0'; printf(\"%d %c %c %d\n\", a, b, c, z); return 0; }".replace(r'\"', '"'), "10 ' \\ 0\n"),
    ('char_signed_promotion', 'char identity(char x) { return x; } int main(void) { char x=-7; printf("%d %d\\n", x, identity(x)); return 0; }', '-7 -7\n'),
    ('bool_arithmetic', 'int main(void) { bool a=true; bool b=false; printf("%d %d\\n", a+2, b+1); return 0; }', '3 1\n'),
    ('float_varargs_promotion', 'int main(void) { float x=1.5f; char c=65; bool b=true; printf("%.1f %c %d\\n", x, c, b); return 0; }', '1.5 A 1\n'),
    ('struct_fields', 'struct Record { char tag; double value; bool valid; }; int main(void) { struct Record r; r.tag=65; r.value=2.5; r.valid=true; printf("%c %.1f %d\\n", r.tag, r.value, r.valid); return 0; }', 'A 2.5 1\n'),
    ('struct_used_field_only', 'struct P { int x; int y; }; int main(void) { struct P p; p.x=7; printf("%d\\n", p.x); return 0; }', '7\n'),
    ('struct_copy', 'struct P { int x; double y; }; int main(void) { struct P p; p.x=2; p.y=3.5; struct P q=p; p.x=9; printf("%d %.1f\\n", q.x, q.y); return 0; }', '2 3.5\n'),
    ('struct_assignment', 'struct P { int x; }; int main(void) { struct P p; struct P q; p.x=4; q=p; printf("%d\\n", q.x); return 0; }', '4\n'),
    ('nested_struct', 'struct P { int x; double y; }; struct R { struct P p; char tag; }; int main(void) { struct R r; r.p.x=4; r.p.y=1.5; r.tag=66; struct R copy=r; printf("%d %.1f %c\\n", copy.p.x, copy.p.y, copy.tag); return 0; }', '4 1.5 B\n'),
    ('struct_parameter_copy', 'struct P { int x; }; int alter(struct P p) { p.x=p.x+3; return p.x; } int main(void) { struct P p; p.x=4; printf("%d %d\\n", alter(p), p.x); return 0; }', '7 4\n'),
    ('struct_return', 'struct P { int x; double y; }; struct P make(int n) { struct P p; p.x=n; p.y=2.5; return p; } int main(void) { struct P p=make(4); printf("%d %.1f\\n", p.x, p.y); return 0; }', '4 2.5\n'),
    ('struct_return_member', 'struct P { int x; }; struct P make(void) { struct P p; p.x=8; return p; } int main(void) { printf("%d\\n", make().x); return 0; }', '8\n'),
    ('struct_prototype', 'struct P { int x; }; struct P make(int n); int main(void) { struct P p=make(6); printf("%d\\n", p.x); return 0; } struct P make(int x) { struct P p; p.x=x; return p; }', '6\n'),
    ('member_branch_merge', 'struct P { int x; int y; }; int main(void) { struct P p; int flag=0; if(flag) { p.x=1; } else { p.x=2; } printf("%d\\n", p.x); return 0; }', '2\n'),
    ('member_loop', 'struct P { int x; }; int main(void) { struct P p; p.x=3; while(p.x>0) { p.x=p.x-1; } printf("%d\\n", p.x); return 0; }', '0\n'),
    ('nested_member_assignment', 'struct P { int x; }; struct R { struct P p; }; int main(void) { struct P p; struct R r; p.x=9; r.p=p; printf("%d\\n", r.p.x); return 0; }', '9\n'),
    ('struct_shadow', 'struct P { int x; }; int main(void) { struct P p; p.x=1; { struct P p; p.x=2; printf("%d ", p.x); } printf("%d\\n", p.x); return 0; }', '2 1\n'),
]

NEGATIVE = [
    ('void_object', 'int main(void) { void x; return 0; }', 'E_SEMA_TYPE', 'void/string'),
    ('void_parameter', 'int f(void x) { return 0; }', 'E_SEMA_TYPE', 'void/string'),
    ('void_value_return', 'void f(void) { return 1; }', 'E_SEMA_RETURN', 'void function'),
    ('missing_return_value', 'double f(void) { return; }', 'E_SEMA_RETURN', 'return value'),
    ('void_initializer', 'void f(void) { return; } int main(void) { int x=f(); return 0; }', 'E_SEMA_TYPE', 'cannot convert void'),
    ('void_condition', 'void f(void) { return; } int main(void) { if(f()) { } return 0; }', 'E_SEMA_TYPE', 'numeric value'),
    ('void_argument', 'void f(void) { return; } int g(int x) { return x; } int main(void) { return g(f()); }', 'E_SEMA_TYPE', 'cannot convert void'),
    ('wrong_main_type', 'float main(void) { return 0; }', 'E_SEMA_MAIN', 'main requires int'),
    ('prototype_return_mismatch', 'float f(int x); double f(int x) { return x; }', 'E_SEMA_FUNCTION', 'incompatible declaration'),
    ('prototype_parameter_mismatch', 'double f(float x); double f(double x) { return x; }', 'E_SEMA_FUNCTION', 'incompatible declaration'),
    ('unknown_struct', 'int main(void) { struct Missing p; return 0; }', 'E_SEMA_STRUCT', 'fully defined before use'),
    ('future_struct', 'int f(void) { struct P p; return 0; } struct P { int x; };', 'E_SEMA_STRUCT', 'fully defined before use'),
    ('duplicate_struct', 'struct P { int x; }; struct P { int y; }; int main(void) { return 0; }', 'E_SEMA_STRUCT', 'duplicate struct'),
    ('duplicate_member', 'struct P { int x; double x; }; int main(void) { return 0; }', 'E_SEMA_MEMBER', 'duplicate member'),
    ('void_member', 'struct P { void x; }; int main(void) { return 0; }', 'E_SEMA_TYPE', 'void/string'),
    ('empty_struct', 'struct P { }; int main(void) { return 0; }', 'E_SEMA_STRUCT', '1..64 members'),
    ('recursive_value_struct', 'struct P { struct P p; }; int main(void) { return 0; }', 'E_SEMA_STRUCT', 'fully defined before use'),
    ('missing_member', 'struct P { int x; }; int main(void) { struct P p; p.y=1; return 0; }', 'E_SEMA_MEMBER', 'no member'),
    ('scalar_member', 'int main(void) { int x=1; return x.y; }', 'E_SEMA_MEMBER', 'requires a struct'),
    ('uninitialized_member', 'struct P { int x; int y; }; int main(void) { struct P p; p.x=1; return p.y; }', 'E_SEMA_UNINITIALIZED', "p.y"),
    ('partial_struct_copy', 'struct P { int x; int y; }; int main(void) { struct P p; p.x=1; struct P q=p; return 0; }', 'E_SEMA_UNINITIALIZED', "p.y"),
    ('partial_struct_argument', 'struct P { int x; int y; }; int f(struct P p) { return p.x; } int main(void) { struct P p; p.x=1; return f(p); }', 'E_SEMA_UNINITIALIZED', "p.y"),
    ('partial_struct_return', 'struct P { int x; int y; }; struct P f(void) { struct P p; p.x=1; return p; }', 'E_SEMA_UNINITIALIZED', "p.y"),
    ('branch_different_members', 'struct P { int x; int y; }; int main(void) { struct P p; int flag=1; if(flag) { p.x=1; } else { p.y=2; } return p.x; }', 'E_SEMA_UNINITIALIZED', "p.x"),
    ('loop_only_member', 'struct P { int x; }; int main(void) { struct P p; int flag=0; while(flag) { p.x=1; } return p.x; }', 'E_SEMA_UNINITIALIZED', "p.x"),
    ('self_init_struct', 'struct P { int x; }; int main(void) { struct P p=p; return 0; }', 'E_SEMA_UNINITIALIZED', "p.x"),
    ('nominal_struct_mismatch', 'struct P { int x; }; struct Q { int x; }; int main(void) { struct P p; p.x=1; struct Q q=p; return 0; }', 'E_SEMA_TYPE', 'cannot convert struct P to struct Q'),
    ('aggregate_arithmetic', 'struct P { int x; }; int main(void) { struct P p; p.x=1; return p+1; }', 'E_SEMA_TYPE', 'numeric value'),
    ('aggregate_condition', 'struct P { int x; }; int main(void) { struct P p; p.x=1; if(p) { } return 0; }', 'E_SEMA_TYPE', 'numeric value'),
    ('external_aggregate_abi', 'struct P { int x; }; struct P external(void); int main(void) { return 0; }', 'E_SEMA_ABI', 'definition in this translation unit'),
    ('float_for_int_format', 'int main(void) { printf("%d", 1.5); return 0; }', 'E_SEMA_FORMAT', 'int argument'),
    ('int_for_float_format', 'int main(void) { printf("%f", 1); return 0; }', 'E_SEMA_FORMAT', 'float or double'),
    ('struct_for_format', 'struct P { int x; }; int main(void) { struct P p; p.x=1; printf("%d", p); return 0; }', 'E_SEMA_FORMAT', 'int argument'),
    ('bad_float_exponent', 'int main(void) { return 1e+; }', 'E_LEX_NUMBER', 'invalid numeric'),
    ('double_literal_overflow', 'double f(void) { return 1e400; }', 'E_AST_LITERAL', 'floating literal'),
    ('float_literal_overflow', 'float f(void) { return 1e39f; }', 'E_AST_LITERAL', 'floating literal'),
    ('character_empty', "int main(void) { return ''; }", 'E_LEX_CHARACTER', 'exactly one'),
    ('character_multiple', "int main(void) { return 'ab'; }", 'E_LEX_CHARACTER', 'exactly one'),
    ('character_escape', r"int main(void) { return '\q'; }", 'E_LEX_CHARACTER', 'unsupported character escape'),
]

ABI_SOURCE = '''#include <stdbool.h>
char c_identity(char x) { return x; }
bool b_identity(bool x) { return x; }
float f_identity(float x) { return x * 0.5f; }
double d_mix(double x, float y, char c, bool b) { return x + y + c + b; }
char ext_char(char x);
float ext_float(float x);
double ext_double(double x);
bool ext_bool(bool x);
void ext_void(void);
int call_external(void) { int code=0; ext_void(); if(ext_char(-7)!=-7) { code=1; } if(ext_float(1.5f)!=3.0f) { code=2; } if(ext_double(2.5)!=3.5) { code=3; } if(ext_bool(false)!=true) { code=4; } return code; }
'''
ABI_HARNESS = '''#include <stdbool.h>
#include <stdio.h>
extern char c_identity(char);
extern bool b_identity(bool);
extern float f_identity(float);
extern double d_mix(double,float,char,bool);
extern int call_external(void);
static int seen=0;
char ext_char(char x) { return x; }
float ext_float(float x) { return x*2.0f; }
double ext_double(double x) { return x+1.0; }
bool ext_bool(bool x) { return !x; }
void ext_void(void) { seen=1; }
int main(void) { if(c_identity(-7)!=-7 || c_identity(65)!=65) return 1; if(b_identity(false) || !b_identity(true)) return 2; if(f_identity(3.0f)!=1.5f) return 3; if(d_mix(2.5,1.5f,-7,true)!=-2.0) return 4; if(call_external()!=0 || !seen) return 5; puts("scalar ABI ok"); return 0; }
'''


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(argv, directory, label, expected=0):
    result = subprocess.run([str(x) for x in argv], cwd=directory, capture_output=True, timeout=20)
    (directory / f'{label}.stdout.log').write_bytes(result.stdout)
    (directory / f'{label}.stderr.log').write_bytes(result.stderr)
    require(result.returncode == expected,
            f'{directory.name}: {label}: expected exit {expected}, got {result.returncode}; {result.stderr!r}')
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('compiler', type=Path)
    parser.add_argument('grammar', type=Path)
    parser.add_argument('output_root', type=Path)
    parser.add_argument('--clang', type=Path)
    args = parser.parse_args()
    args.compiler, args.grammar = args.compiler.resolve(), args.grammar.resolve()
    if args.clang:
        args.clang = args.clang.resolve()
    args.output_root.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='types-native-' if args.clang else 'types-ast-',
                               dir=args.output_root)).resolve()
    print(f'测试产物：{run}', flush=True)
    records = []
    grammar_before = args.grammar.read_bytes()

    def compile_source(name, text, level, code=None, detail=None, headers=None):
        case = run / name
        case.mkdir()
        source = case / 'source.c'
        source.write_text(text + '\n', encoding='utf-8')
        inputs = [source, args.grammar]
        for filename, content in (headers or {}).items():
            header = case / filename
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text(content, encoding='utf-8')
            inputs.append(header)
        before = {p: p.read_bytes() for p in inputs}
        output = case / 'output'
        argv = [args.compiler, '--input', source, '--grammar', args.grammar,
                '--output-dir', output, '--emit', 'llvm' if args.clang else 'ast']
        if args.clang:
            argv += ['--opt-level', level]
        result = command(argv, case, 'compile', 1 if code else 0)
        require(all(p.read_bytes() == data for p, data in before.items()), f'{name}: input changed')
        if not code or not code.startswith('E_LEX_'):
            stack = output / '10-stack_file.txt'
            require(stack.is_file() and '预测分析结束' in stack.read_text(encoding='utf-8'),
                    f'{name}: original parser did not complete')
        if code:
            diagnostic = result.stderr.decode('utf-8')
            require(f'error[{code}]' in diagnostic and detail in diagnostic,
                    f'{name}: wrong diagnostic {diagnostic!r}')
            for artifact in ['15-ast.txt', '16-before.ll', '17-after.ll', '18-program.o']:
                require(not (output / artifact).exists(), f'{name}: published final output after rejection')
        else:
            require(not result.stderr, f'{name}: unexpected diagnostic {result.stderr!r}')
            tree = (output / '15-ast.txt').read_text(encoding='utf-8')
            require('函数 ' in tree, f'{name}: incomplete AST')
            if args.clang:
                require((output / '18-program.o').stat().st_size > 0, f'{name}: empty object')
                ir = (output / '16-before.ll').read_text(encoding='utf-8')
                if name.startswith('float_varargs_promotion'):
                    require('fpext float' in ir and 'sext i8' in ir and 'zext i1' in ir,
                            'printf default promotions missing from raw IR')
                if name.startswith('struct_fields'):
                    require('%struct.Record = type { i8, double, i1 }' in ir and
                            'getelementptr inbounds %struct.Record' in ir, 'struct layout/GEP missing')
            else:
                require(not (output / '18-program.o').exists(), f'{name}: AST mode ran backend')
        return case, source, output, before

    levels = [0, 2] if args.clang else [0]
    cases = list(POSITIVE)
    repository = Path(__file__).resolve().parents[1]
    example = (repository / 'examples/typed_structs.c').read_text(encoding='utf-8')
    header = (repository / 'examples/typed_structs.h').read_text(encoding='utf-8')
    cases.append(('repository_typed_example', example, 'value=3.00 grade=A accepted=1\nscore=3.50\n'))
    for name, text, expected in cases:
        for level in levels:
            headers = {'typed_structs.h': header} if name == 'repository_typed_example' else {}
            case, source, output, before = compile_source(f'{name}_O{level}',
                text if name == 'repository_typed_example' else PRELUDE + text, level, headers=headers)
            if args.clang:
                exe, oracle = case / 'actual', case / 'reference'
                command([args.clang, output / '18-program.o', '-o', exe], case, 'link')
                command([args.clang, '-std=c17', '-Werror=implicit-function-declaration', f'-O{level}', source, '-o', oracle], case, 'reference_build')
                actual = command([exe], case, 'run').stdout
                reference = command([oracle], case, 'reference_run').stdout
                require(actual == reference == expected.encode('utf-8'),
                        f'{name}: stdout mismatch actual={actual!r} clang={reference!r} expected={expected!r}')
                require(all(p.read_bytes() == data for p, data in before.items()), f'{name}: input changed')
            records.append({'case': name, 'optimization': level, 'executed': bool(args.clang),
                            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                            'expected_stdout': expected})
            print(f'PASS {name}_O{level}：' + ('退出码和标准输出与Clang及预期一致' if args.clang else '类型与初始化检查通过'), flush=True)
    for name, text, code, detail in NEGATIVE:
        compile_source(name, PRELUDE + text, 2 if args.clang else 0, code, detail)
        records.append({'case': name, 'accepted': False, 'diagnostic': code})
        print(f'PASS {name}：{code}，输入未改变且未发布最终产物', flush=True)
    for level in levels:
        case, source, output, _ = compile_source(f'scalar_ABI_O{level}', ABI_SOURCE, level)
        if args.clang:
            harness = case / 'harness.c'
            harness.write_text(ABI_HARNESS, encoding='utf-8')
            exe, oracle = case / 'actual', case / 'reference'
            command([args.clang, harness, output / '18-program.o', '-o', exe], case, 'ABI_link')
            command([args.clang, '-std=c17', '-Werror=implicit-function-declaration', f'-O{level}', source, harness, '-o', oracle], case, 'ABI_reference_build')
            require(command([exe], case, 'ABI_run').stdout ==
                    command([oracle], case, 'ABI_reference_run').stdout == b'scalar ABI ok\n',
                    'bidirectional scalar ABI check failed')
        records.append({'case': 'scalar_ABI', 'optimization': level, 'executed': bool(args.clang)})
        print(f'PASS scalar_ABI_O{level}：' + ('与Clang双向互调通过' if args.clang else '标量接口检查通过'), flush=True)
    require(args.grammar.read_bytes() == grammar_before, 'grammar changed')
    (run / 'results.json').write_text(json.dumps(records, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'多类型与结构体：{len(records)}项用例通过；没有进行性能测量')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, UnicodeError, subprocess.SubprocessError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        sys.exit(1)
