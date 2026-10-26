"""CP05：只对有定义行为的程序比较O0/O2、Clang和独立预期；错误程序不得发布产物。"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

PRELUDE = '#include <stdio.h>\n#include <stdbool.h>\n'
POSITIVE = [
 ('array_fill_sum', 'int main(void) { int a[5]; int sum=0; for(int i=0;i<5;i++) { a[i]=i*3; sum+=a[i]; } printf("%d\\n",sum); return 0; }', '30\n', 0),
 ('partial_array', 'int main(void) { int a[4]={2,3}; printf("%d %d %d %d\\n",a[0],a[1],a[2],a[3]); return 0; }','2 3 0 0\n',0),
 ('multidimensional', 'int main(void) { int a[2][3]={{1,2},{4,5,6}}; int sum=0; for(int i=0;i<2;++i) { for(int j=0;j<3;++j) { sum+=a[i][j]; } } printf("%d\\n",sum); return 0; }','18\n',0),
 ('array_parameter', 'int sum(int a[],int n) { int total=0; for(int i=0;i<n;i=i+1) { total+=a[i]; } return total; } int main(void) { int a[3]={4,5,6}; printf("%d\\n",sum(a,3)); return 0; }','15\n',0),
 ('matrix_parameter', 'int get(int a[][3],int i,int j) { return a[i][j]; } int main(void) { int a[2][3]={{1,2,3},{4,5,6}}; printf("%d\\n",get(a,1,2)); return 0; }','6\n',0),
 ('pointer_store', 'void set(int *p,int value) { *p=value; } int main(void) { int x; set(&x,17); printf("%d\\n",x); return 0; }','17\n',0),
 ('pointer_to_pointer', 'int main(void) { int x=1; int *p=&x; int **q=&p; **q=9; printf("%d\\n",x); return 0; }','9\n',0),
 ('pointer_arithmetic', 'int main(void) { int a[4]={10,20,30,40}; int *p=a; p+=2; int x=*p; --p; p=p+2; printf("%d %d %d %d\\n",x,*p,p-a,*(1+a)); return 0; }','30 40 3 20\n',0),
 ('pointer_subtraction', 'int main(void) { double a[4]={1.5,2.5,3.5,4.5}; double *p=a+3; p-=2; printf("%.1f %d\\n",*p,(a+1)-(a+3)); return 0; }','2.5 -2\n',0),
 ('pointer_update', 'int main(void) { int a[3]={3,5,7}; int *p=a; int x=*p++; int y=*++p; printf("%d %d %d\\n",x,y,p-a); return 0; }','3 7 2\n',0),
 ('single_evaluation', 'int main(void) { int a[3]={1,2,3}; int i=0; a[i++]+=5; int *p=a; *p++=8; printf("%d %d %d\\n",a[0],i,p-a); return 0; }','8 1 1\n',0),
 ('pointer_return', 'int *choose(int *a) { return a+1; } int main(void) { int a[2]={4,9}; printf("%d\\n",*choose(a)); return 0; }','9\n',0),
 ('void_pointer', 'int main(void) { int x=12; void *p=&x; int *q=p; int *r=(int*)p; printf("%d %d\\n",*q,*r); return 0; }','12 12\n',0),
 ('null_and_condition', 'int *nil(void) { return 0; } int main(void) { int *p=0; int x=1; int *q=&x; bool b=q; int y=0; if(q) { y=3; } printf("%d %d %d %d %d\\n",p==0,0==nil(),q!=p,b,y); return 0; }','1 1 1 1 3\n',0),
 ('pointer_relations', 'int main(void) { int a[3]; int *p=a; int *q=a+3; printf("%d %d %d %d\\n",p<q,p<=p,q>p,q>=q); return 0; }','1 1 1 1\n',0),
 ('pointer_array', 'int main(void) { int x=2; int y=7; int *a[2]={&x,&y}; *a[1]=11; printf("%d %d\\n",*a[0],y); return 0; }','2 11\n',0),
 ('struct_pointer', 'struct P { int x; double y; }; void set(struct P *p) { p->x=4; p->y=2.5; } int main(void) { struct P p; set(&p); printf("%d %.1f\\n",p.x,p.y); return 0; }','4 2.5\n',0),
 ('recursive_struct_pointer', 'struct Node { int x; struct Node *next; }; int main(void) { struct Node a; struct Node b; a.x=3; b.x=8; a.next=&b; b.next=0; printf("%d %d\\n",a.next->x,b.next==0); return 0; }','8 1\n',0),
 ('struct_array', 'struct P { int x; }; int main(void) { struct P a[2]; a[0].x=3; a[1].x=4; struct P *p=a; printf("%d %d\\n",p->x,(p+1)->x); return 0; }','3 4\n',0),
 ('array_member', 'struct P { int a[3]; }; int main(void) { struct P p; p.a[0]=2; p.a[1]=3; p.a[2]=5; printf("%d\\n",p.a[0]+p.a[1]+p.a[2]); return 0; }','10\n',0),
 ('char_array', 'int main(void) { char text[6]="hello"; text[0]=\'H\'; char *p=text; printf("%s %c\\n",p,*(p+1)); return 0; }','Hello e\n',0),
 ('char_exact_bound', 'int main(void) { char a[2]="hi"; printf("%c%c\\n",a[0],a[1]); return 0; }','hi\n',0),
 ('string_pointer', 'int main(void) { char *p="hello"; printf("%s %c\\n",p,p[4]); return 0; }','hello o\n',0),
 ('float_array', 'int main(void) { float a[3]={1.5f,2.5f,3.5f}; float *p=a; *p+=1.0f; printf("%.1f %.1f\\n",a[0],p[2]); return 0; }','2.5 3.5\n',0),
 ('zero_iteration', 'int main(void) { int total=7; for(int i=5;i<2;i++) { total=99; } printf("%d\\n",total); return 0; }','7\n',0),
 ('for_shadow', 'int main(void) { int i=9; int sum=0; for(int i=0;i<3;++i) { sum+=i; } printf("%d %d\\n",i,sum); return 0; }','9 3\n',0),
 ('for_existing_variable', 'int main(void) { int i=0; int sum=0; for(;i<4;) { sum+=i; i++; } printf("%d %d\\n",sum,i); return 0; }','6 4\n',0),
 ('for_step_assignment', 'int main(void) { int sum=0; for(int i=5;i>0;i-=2) { sum+=i; } printf("%d\\n",sum); return 0; }','9\n',0),
 ('integer_update_values', 'int main(void) { int x=2; int a=x++; int b=++x; int c=x--; int d=--x; printf("%d %d %d %d %d\\n",a,b,c,d,x); return 0; }','2 4 4 2 2\n',0),
 ('nonzero_exit', 'int main(void) { int a[2]={2,5}; printf("done\\n"); return a[1]; }','done\n',5),
]
POSITIVE += [
 ('inferred_array', 'int main(void) { int a[]={2,4,6}; char text[]="ok"; printf("%d %s\\n",a[2],text); return 0; }','6 ok\n',0),
 ('array_of_strings', 'int main(void) { char *a[2]={"yes","no"}; char **p=a; printf("%s %s\\n",*p,p[1]); return 0; }','yes no\n',0),
 ('nested_partial_init', 'int main(void) { int a[2][2]={{3},{5}}; printf("%d %d %d %d\\n",a[0][0],a[0][1],a[1][0],a[1][1]); return 0; }','3 0 5 0\n',0),
 ('bool_array', 'int main(void) { bool a[2]={true,false}; bool *p=a; p[1]=true; printf("%d %d\\n",a[0],a[1]); return 0; }','1 1\n',0),
 ('negative_offset', 'int main(void) { int a[3]={2,4,6}; int *p=a+2; printf("%d %d\\n",*(p+-1),*(p-2)); return 0; }','4 2\n',0),
]

NEGATIVE = [
 ('uninitialized_pointer','int main(void) { int *p; return *p; }','E_SEMA_UNINITIALIZED'),
 ('void_dereference','int main(void) { void *p=0; return *p; }','E_SEMA_POINTER'),
 ('void_arithmetic','int main(void) { void *p=0; p=p+1; return 0; }','E_SEMA_POINTER'),
 ('wrong_pointer','int main(void) { int x=1; double *p=&x; return 0; }','E_SEMA_TYPE'),
 ('nonzero_integer_pointer','int main(void) { int *p=1; return 0; }','E_SEMA_TYPE'),
 ('float_index','int main(void) { int a[2]; return a[0.5]; }','E_SEMA_POINTER'),
 ('array_assignment','int main(void) { int a[2]; int b[2]; a=b; return 0; }','E_SEMA_LVALUE'),
 ('void_array','int main(void) { void a[2]; return 0; }','E_SEMA_TYPE'),
 ('zero_array','int main(void) { int a[0]; return 0; }','E_AST_ARRAY'),
 ('variable_array','int main(void) { int n=3; int a[n]; return 0; }','E_AST_ARRAY'),
 ('negative_array','int main(void) { int a[-1]; return 0; }','E_AST_ARRAY'),
 ('too_many_initializers','int main(void) { int a[2]={1,2,3}; return 0; }','E_SEMA_INITIALIZER'),
 ('long_string','int main(void) { char a[2]="abc"; return 0; }','E_SEMA_INITIALIZER'),
 ('for_scope_leak','int main(void) { for(int i=0;i<2;i++) { } return i; }','E_SEMA_NAME'),
 ('loop_only_init','int main(void) { int x; for(int i=0;i<0;i++) { x=1; } return x; }','E_SEMA_UNINITIALIZED'),
 ('invalid_lvalue','int main(void) { int x=1; &(x+1); return 0; }','E_SEMA_LVALUE'),
 ('pointer_multiply','int main(void) { int a[2]; int *p=a; p*=2; return 0; }','E_SEMA_POINTER'),
 ('pointer_plus_pointer','int main(void) { int a[2]; int *p=a; p=p+p; return 0; }','E_SEMA_POINTER'),
 ('bad_printf_pointer','int main(void) { int a[2]; printf("%s",a); return 0; }','E_SEMA_FORMAT'),
 ('bool_increment','int main(void) { bool x=true; x++; return 0; }','E_SEMA_POINTER'),
]
NEGATIVE += [
 ('void_array_parameter','void f(void a[]) { } int main(void) { return 0; }','E_SEMA_TYPE'),
 ('void_matrix_parameter','void f(void a[][2]) { } int main(void) { return 0; }','E_SEMA_TYPE'),
 ('missing_inferred_init','int main(void) { int a[]; return 0; }','E_AST_ARRAY'),
 ('temporary_array_member','struct P { int a[2]; }; struct P make(void) { struct P p; p.a[0]=1; p.a[1]=2; return p; } int main(void) { return make().a[0]; }','E_SEMA_LVALUE'),
]

ABI_SOURCE = """int *identity(int *p) { return p; }
void fill(int *p,int n) { for(int i=0;i<n;i++) { p[i]=i*3; } }
void external_set(int *p);
int call_external(void) { int x=0; external_set(&x); return x; }
"""
ABI_HARNESS = """#include <stdio.h>
extern int *identity(int*);
extern void fill(int*,int);
extern int call_external(void);
void external_set(int *p) { *p=17; }
int main(void) { int a[4]={0}; fill(a,4); if(identity(a)!=a || a[3]!=9 || call_external()!=17) return 1; puts("pointer ABI ok"); return 0; }
"""



def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('compiler',type=Path)
    parser.add_argument('grammar',type=Path)
    parser.add_argument('output_root',type=Path)
    parser.add_argument('--clang',type=Path)
    args=parser.parse_args()
    args.compiler=args.compiler.resolve(); args.grammar=args.grammar.resolve()
    if args.clang: args.clang=args.clang.resolve()
    args.output_root.mkdir(parents=True,exist_ok=True)
    root=Path(tempfile.mkdtemp(prefix='memory-native-' if args.clang else 'memory-ast-',dir=args.output_root)).resolve()
    print(f'测试产物：{root}',flush=True)
    grammar=args.grammar.read_bytes()
    records=[]
    def command(argv,case,label,expected=0):
        p=subprocess.run([str(x) for x in argv],cwd=case,capture_output=True,timeout=15)
        (case/(label+'.stdout')).write_bytes(p.stdout)
        (case/(label+'.stderr')).write_bytes(p.stderr)
        require(p.returncode==expected,f'{case.name}/{label}: exit {p.returncode}, expected {expected}; {p.stderr!r}')
        return p
    def compile_case(name,text,level,diagnostic=None):
        case=root/name;case.mkdir()
        src=case/'source.c';src.write_text(PRELUDE+text+'\n',encoding='utf-8');before=src.read_bytes()
        out=case/'output'
        argv=[args.compiler,'--input',src,'--grammar',args.grammar,'--output-dir',out,'--emit','llvm' if args.clang else 'ast']
        if args.clang:argv+=['--opt-level',level]
        p=command(argv,case,'compile',1 if diagnostic else 0)
        require(src.read_bytes()==before,'source modified')
        stack=out/'10-stack_file.txt'
        require(stack.is_file() and '预测分析结束' in stack.read_text(),f'{name}: original parser did not finish')
        if diagnostic:
            require(f'error[{diagnostic}]' in p.stderr.decode(),f'{name}: wrong diagnostic {p.stderr!r}')
            require(not any((out/x).exists() for x in ['15-ast.txt','16-before.ll','17-after.ll','18-program.o']),f'{name}: invalid final artifacts')
        else:
            require(not p.stderr,f'{name}: unexpected diagnostic')
            require('函数 ' in (out/'15-ast.txt').read_text(),f'{name}: missing AST')
        return case,src,out
    cases=list(POSITIVE)
    example=(Path(__file__).resolve().parents[1]/'examples/arrays_pointers_for.c').read_text()
    cases.append(('repository_memory_example',example,'count=4 total=15.0 label=verified\n',0))
    rng=random.Random(20261026)
    for k in range(12):
        values=[rng.randint(-20,20) for _ in range(rng.randint(2,9))];factor=rng.randint(1,5)
        init=','.join(map(str,values));n=len(values);expected=sum(v*factor+i for i,v in enumerate(values))
        text=f'int main(void) {{ int a[{n}]={{{init}}}; int *p=a; int total=0; for(int i=0;i<{n};++i) {{ total+=p[i]*{factor}+i; }} printf("%d\\n",total); return 0; }}'
        cases.append((f'seeded_array_{k}',text,f'{expected}\n',0))
    for name,text,expected,exit_code in cases:
        for level in ([0,2] if args.clang else [0]):
            case,src,out=compile_case(f'{name}_O{level}',text,level)
            if args.clang:
                actual=case/'actual';reference=case/'reference'
                command([args.clang,out/'18-program.o','-o',actual],case,'link')
                command([args.clang,'-std=c17','-Werror=implicit-function-declaration',f'-O{level}',src,'-o',reference],case,'clang')
                a=command([actual],case,'run',exit_code);r=command([reference],case,'reference',exit_code)
                require(a.stdout==r.stdout==expected.encode() and a.stderr==r.stderr==b'',f'{name}: output differs from Clang/independent expectation')
            records.append({'case':name,'level':level,'executed':bool(args.clang),'stdout':expected,'exit':exit_code,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest()})
            print(f'PASS {name}_O{level}',flush=True)
    for level in ([0,2] if args.clang else [0]):
        case,src,out=compile_case(f'pointer_ABI_O{level}',ABI_SOURCE,level)
        if args.clang:
            harness=case/'harness.c';harness.write_text(ABI_HARNESS)
            actual=case/'actual';reference=case/'reference'
            command([args.clang,harness,out/'18-program.o','-o',actual],case,'abi_link')
            command([args.clang,f'-O{level}',src,harness,'-o',reference],case,'abi_clang')
            a=command([actual],case,'abi_run');r=command([reference],case,'abi_reference')
            require(a.stdout==r.stdout==b'pointer ABI ok\n' and a.stderr==r.stderr==b'', 'bidirectional pointer ABI mismatch')
        records.append({'case':'pointer_ABI','level':level,'executed':bool(args.clang)})
        print(f'PASS pointer_ABI_O{level}',flush=True)
    # 无条件for接受语法与控制流；不运行这个有意无限的例子。
    compile_case('omitted_condition_ast','int main(void) { for(;;) { } return 0; }',0)
    for name,text,code in NEGATIVE:
        compile_case(name,text,2 if args.clang else 0,code)
        records.append({'case':name,'diagnostic':code,'accepted':False})
        print(f'PASS {name}: {code}',flush=True)
    require(args.grammar.read_bytes()==grammar,'grammar modified')
    (root/'results.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n')
    print(f'数组、指针、for：{len(records)}项记录通过；无限循环仅构建检查，不计运行验证')

if __name__=='__main__':
    try: main()
    except (RuntimeError,OSError,subprocess.SubprocessError) as error:
        print(f'FAIL: {error}',file=sys.stderr);sys.exit(1)
