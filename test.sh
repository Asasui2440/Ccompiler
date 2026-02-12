#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -target x86_64-apple-darwin -o tmp.x tmp.s 
  ./tmp.x
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
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
assert 1 "return 1>0;"
assert 4 "a = 2; b = 2; return a+b;"
assert 1 "a = 2; if (a == 2) { return 1; }"
assert 5 "a = 2; while(a < 5) a = a + 1; return a;"
assert 3 "a = 2; if ( a == 3) { return 0; } else {return 3;}"
assert 4 "a = 2; if (a == 2) { a = 3; a = a + 1;  return a; }";

echo '#include <stdio.h>
int foo() {
    printf("OK\n");
    return 0;
}' > foo.c

cc -target x86_64-apple-darwin -c foo.c -o foo.o
./9cc "return foo();" > tmp.s
cc -target x86_64-apple-darwin -o tmp tmp.s foo.o
./tmp


echo '#include <stdio.h>
void foo(int x, int y) { printf("%d\n", x + y); }' > foo.c

cc -target x86_64-apple-darwin -c foo.c -o foo.o
./9cc "foo(3, 4);" > tmp.s
cc -target x86_64-apple-darwin -o tmp tmp.s foo.o
./tmp

echo OK