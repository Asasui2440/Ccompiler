#!/bin/bash
cat <<EOF > tmp2.c
#include <stdio.h>
#include <stdlib.h>
void alloc4(int **p, int a, int b, int c, int d) {
    int *int_ptr = (int *)malloc(sizeof(int) * 4);
    int_ptr[0] = a;
    int_ptr[1] = b;
    int_ptr[2] = c;
    int_ptr[3] = d;
 
    *p = int_ptr;
}
EOF

cc -target x86_64-apple-darwin -c tmp2.c -o tmp2.o

assert() {
  expected="$1"
  input="$2"

  # 自動的にmain()で囲む
  input="int main() { $input }"

  # 一時ファイルに書き込む
  echo "$input" > tmp.c
  ./9cc tmp.c > tmp.s
  cc -target x86_64-apple-darwin -o tmp.x tmp.s tmp2.o
  ./tmp.x
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# グローバル変数や複数関数定義用のassert（main()で囲まない）
assert_program() {
  expected="$1"
  input="$2"

  # 一時ファイルに書き込む
  echo "$input" > tmp.c
  ./9cc tmp.c > tmp.s
  cc -target x86_64-apple-darwin -o tmp.x tmp.s tmp2.o
  ./tmp.x
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "return 0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 10 "-10+20;"
assert 50 "-10*(-5);"
assert 14 "(-2)*(+3)+4*5;"
assert 0 "1>2;"
assert 0 "1>1;"
assert 1 "1<2;"
assert 1 "1<=2;"
assert 0 "1>=2;"
assert 1 "1>=1;"
assert 1 "1>0;"
assert 4 "int a; int b; a = 2; b = 2; return a+b;"
assert 1 "int a; a = 2; if (a == 2) { return 1; }"
assert 5 "int a; a = 2; while(a < 5) a = a + 1; return a;"
assert 3 "int a; a = 2; if ( a == 3) { return 0; } else {return 3;}"
assert 4 "int a; a = 2; if (a == 2) { a = 3; a = a + 1;  return a; }";

# 引数なしの関数呼び出し
echo '#include <stdio.h>
int foo() {
    printf("OK");
    printf("\n");
    return 0;
}' > foo.c
cc -target x86_64-apple-darwin -c foo.c -o foo.o
echo "int main() { foo(); }" > tmp.c
./9cc tmp.c > tmp.s
cc -target x86_64-apple-darwin -o tmp tmp.s foo.o
./tmp

# 引数ありの関数呼び出し
echo '#include <stdio.h>
void foo(int x, int y) { 
    printf("%d\n", x + y); 
}' > foo.c
cc -target x86_64-apple-darwin -c foo.c -o foo.o
echo "int main() {foo(3, 4);}" > tmp.c
./9cc tmp.c > tmp.s
cc -target x86_64-apple-darwin -o tmp.x tmp.s foo.o
./tmp.x

# 関数定義 フィボナッチ数列
./9cc fib.txt > tmp.s
cc -target x86_64-apple-darwin -o tmp.x tmp.s
./tmp.x

# *と&
assert 3 "int x; x = 3; int y; y = 5; int z; z = &y + 2; return *z;";

# ポインタ型
assert 3 "int x; int *y; y = &x; *y = 3; return x;";


# ポインタの演算
assert 4 'int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; '
assert 8 'int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; return *q; '
assert 2 'int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; q = q - 1; return *q; '
assert 6 'int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q + *(p+1); '

# sizeof演算子
assert 4 'int x; sizeof(x);'
assert 8 'int *y; sizeof(y);'
assert 4 'int x; sizeof(x+3);'
assert 8 'int *y; sizeof(y+3);'
assert 4 'int *y; sizeof(*y);'
assert 4 'sizeof(1);'
assert 4 'sizeof(sizeof(1));'

# 配列
assert 1 'int a[1]; *a = 1; return *a;'
assert 3 'int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *p + *(p+1);'
assert 4 'int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1);'

# 配列の添え字
assert 1 'int a[1]; *a = 1; return a[0];'
assert 3 'int a[2]; *a = 2; a[1] = 1;  return a[0] + a[1];'
assert 1 'int a[2]; *a = 2; a[1] = 1;  return a[0] - a[1];'

# グローバル変数
assert_program 42 'int foo; int a() { foo = 42; return foo; } int main() { return a(); }' 
assert_program 42 'int *y; int a() { int x; y = &x; *y = 42; return x; } int main() { return a(); }' 
assert_program 3 'int foo[2]; int a() { foo[0] = 1; foo[1] = 2; return foo[0] + foo[1]; } int main() { return a(); }' 

assert_program 1 'int x; int* a() { x = 1; return &x; } int main() { int *b; b = a(); return *b; }' 
assert_program 5 'int arr[10]; int* a() { arr[3] = 5; return &arr[3]; } int main() { int *b; b = a(); return *b; }'
assert_program 42 'int* a(int *p) { *p = 42; return p; } int main() { int x; int *b; b = a(&x); return *b; }'

# 文字型
assert 3 'char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y;'

# 文字列リテラル
assert_program 100 'char* a;  char* printf(char* c){ return c; } char* foo() { a = "hello,world!";  return printf(a); } int main() { foo(); return 100; }'

# コメント
# 行コメント
assert 2 'int a; a = 2; 
// a = 3;
return a;'

# ブロックコメント
assert 2 'int a; a = 2; /* a = 3; */ return a;'
assert 2  'int a; a = 2;
/* a = 3;
*/
return a;'


echo OK