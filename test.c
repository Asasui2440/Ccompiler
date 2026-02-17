#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int test_count = 0;
int test_passed = 0;
int test_failed = 0;

// テストヘルパー関数：コードをコンパイル・実行して終了コードを取得
int run_code(const char* code, int wrap_main) {
  FILE* fp = fopen("tmp.c", "w");
  if (!fp) {
    perror("Failed to create tmp.c");
    return -1;
  }

  if (wrap_main) {
    fprintf(fp, "int main() { %s }", code);
  } else {
    fprintf(fp, "%s", code);
  }
  fclose(fp);

  // 9ccでコンパイル
  if (system("./9cc tmp.c > tmp.s 2>/dev/null") != 0) {
    fprintf(stderr, "Compilation failed: %s\n", code);
    return -1;
  }

  // アセンブリをコンパイル
  if (system(
          "cc -target x86_64-apple-darwin -o tmp.x tmp.s tmp2.o 2>/dev/null") !=
      0) {
    fprintf(stderr, "Assembly failed: %s\n", code);
    return -1;
  }

  // 実行して終了コードを取得
  int status = system("./tmp.x 2>/dev/null");
  if (status == -1) {
    fprintf(stderr, "Execution failed: %s\n", code);
    return -1;
  }

  return WEXITSTATUS(status);
}

// アサーション関数
void assert_code(int expected, const char* code) {
  test_count++;
  int actual = run_code(code, 1);

  if (actual == expected) {
    printf("✓ Test %d: %s => %d\n", test_count, code, actual);
    test_passed++;
  } else {
    printf("✗ Test %d: %s => expected %d, but got %d\n", test_count, code,
           expected, actual);
    test_failed++;
  }
}

// プログラム全体のアサーション（main()で囲まない）
void assert_program(int expected, const char* code) {
  test_count++;
  int actual = run_code(code, 0);

  if (actual == expected) {
    printf("✓ Test %d: program => %d\n", test_count, actual);
    test_passed++;
  } else {
    printf("✗ Test %d: program => expected %d, but got %d\n", test_count,
           expected, actual);
    test_failed++;
  }
}

// ファイルからコードを読み込んで実行
int run_file(const char* filename) {
  char cmd[256];

  // 9ccでコンパイル
  snprintf(cmd, sizeof(cmd), "./9cc %s > tmp.s 2>/dev/null", filename);
  if (system(cmd) != 0) {
    fprintf(stderr, "Compilation failed: %s\n", filename);
    return -1;
  }

  // アセンブリをコンパイル
  if (system("cc -target x86_64-apple-darwin -o tmp.x tmp.s 2>/dev/null") !=
      0) {
    fprintf(stderr, "Assembly failed: %s\n", filename);
    return -1;
  }

  // 実行して終了コードを取得
  int status = system("./tmp.x 2>/dev/null");
  if (status == -1) {
    fprintf(stderr, "Execution failed: %s\n", filename);
    return -1;
  }

  return WEXITSTATUS(status);
}

void assert_file(int expected, const char* filename) {
  test_count++;
  int actual = run_file(filename);

  if (actual == expected) {
    printf("✓ Test %d: %s => %d\n", test_count, filename, actual);
    test_passed++;
  } else {
    printf("✗ Test %d: %s => expected %d, but got %d\n", test_count, filename,
           expected, actual);
    test_failed++;
  }
}

void setup_tmp2() {
  FILE* fp = fopen("tmp2.c", "w");
  if (!fp) {
    perror("Failed to create tmp2.c");
    exit(1);
  }

  fprintf(fp,
          "#include <stdio.h>\n"
          "#include <stdlib.h>\n"
          "void alloc4(int **p, int a, int b, int c, int d) {\n"
          "    int *int_ptr = (int *)malloc(sizeof(int) * 4);\n"
          "    int_ptr[0] = a;\n"
          "    int_ptr[1] = b;\n"
          "    int_ptr[2] = c;\n"
          "    int_ptr[3] = d;\n"
          "    *p = int_ptr;\n"
          "}\n");
  fclose(fp);

  if (system("cc -target x86_64-apple-darwin -c tmp2.c -o tmp2.o") != 0) {
    fprintf(stderr, "Failed to compile tmp2.c\n");
    exit(1);
  }
}

int main() {
  printf("Setting up test environment...\n");
  setup_tmp2();

  printf("\n=== Running Tests ===\n\n");

  // 基本的な演算
  assert_code(0, "return 0;");
  assert_code(42, "42;");
  assert_code(21, "5+20-4;");
  assert_code(47, "5+6*7;");
  assert_code(15, "5*(9-6);");
  assert_code(4, "(3+5)/2;");
  assert_code(10, "-10+20;");
  assert_code(50, "-10*(-5);");
  assert_code(14, "(-2)*(+3)+4*5;");

  // 比較演算子
  assert_code(0, "1>2;");
  assert_code(0, "1>1;");
  assert_code(1, "1<2;");
  assert_code(1, "1<=2;");
  assert_code(0, "1>=2;");
  assert_code(1, "1>=1;");
  assert_code(1, "1>0;");

  // 変数
  assert_code(4, "int a; int b; a = 2; b = 2; return a+b;");
  assert_code(1, "int a; a = 2; if (a == 2) { return 1; }");
  assert_code(5, "int a; a = 2; while(a < 5) a = a + 1; return a;");
  assert_code(3, "int a; a = 2; if ( a == 3) { return 0; } else {return 3;}");
  assert_code(4, "int a; a = 2; if (a == 2) { a = 3; a = a + 1;  return a; }");

  // ポインタ
  assert_code(3, "int x; x = 3; int y; y = 5; int z; z = &y + 2; return *z;");
  assert_code(3, "int x; int *y; y = &x; *y = 3; return x;");

  // ポインタ演算
  assert_code(4,
              "int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q;");
  assert_code(8,
              "int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; return *q;");
  assert_code(2,
              "int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; q = q - 1; "
              "return *q;");
  assert_code(
      6,
      "int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q + *(p+1);");

  // sizeof演算子
  assert_code(4, "int x; sizeof(x);");
  assert_code(8, "int *y; sizeof(y);");
  assert_code(4, "int x; sizeof(x+3);");
  assert_code(8, "int *y; sizeof(y+3);");
  assert_code(4, "int *y; sizeof(*y);");
  assert_code(4, "sizeof(1);");
  assert_code(4, "sizeof(sizeof(1));");

  // 配列
  assert_code(1, "int a[1]; *a = 1; return *a;");
  assert_code(
      3, "int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *p + *(p+1);");
  assert_code(4, "int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1);");

  // 配列の添え字
  assert_code(1, "int a[1]; *a = 1; return a[0];");
  assert_code(3, "int a[2]; *a = 2; a[1] = 1;  return a[0] + a[1];");
  assert_code(1, "int a[2]; *a = 2; a[1] = 1;  return a[0] - a[1];");

  // グローバル変数
  assert_program(
      42,
      "int foo; int a() { foo = 42; return foo; } int main() { return a(); }");
  assert_program(42,
                 "int *y; int a() { int x; y = &x; *y = 42; return x; } int "
                 "main() { return a(); }");
  assert_program(3,
                 "int foo[2]; int a() { foo[0] = 1; foo[1] = 2; return foo[0] "
                 "+ foo[1]; } int main() { return a(); }");
  assert_program(1,
                 "int x; int* a() { x = 1; return &x; } int main() { int *b; b "
                 "= a(); return *b; }");
  assert_program(5,
                 "int arr[10]; int* a() { arr[3] = 5; return &arr[3]; } int "
                 "main() { int *b; b = a(); return *b; }");
  assert_program(42,
                 "int* a(int *p) { *p = 42; return p; } int main() { int x; "
                 "int *b; b = a(&x); return *b; }");

  // 文字型
  assert_code(3,
              "char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y;");

  // 文字列リテラル
  assert_program(100,
                 "char* a;  char* printf(char* c){ return c; } char* foo() { a "
                 "= \"hello,world!\";  return printf(a); } int main() { foo(); "
                 "return 100; }");

  // コメント
  assert_code(2, "int a; a = 2; \n// a = 3;\nreturn a;");
  assert_code(2, "int a; a = 2; /* a = 3; */ return a;");
  assert_code(2, "int a; a = 2;\n/* a = 3;\n*/\nreturn a;");

  // フィボナッチ数列（ファイルから）
  assert_file(55, "fib.txt");

  printf("\n=== Test Summary ===\n");
  printf("Total: %d\n", test_count);
  printf("Passed: %d\n", test_passed);
  printf("Failed: %d\n", test_failed);

  if (test_failed == 0) {
    printf("\n✓ All tests passed!\n");
    return 0;
  } else {
    printf("\n✗ Some tests failed\n");
    return 1;
  }
}
