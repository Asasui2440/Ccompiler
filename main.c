#include "9cc.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }
  // トークナイズする
  user_input = argv[1];
  token = tokenize(argv[1]);
  locals = calloc(1, sizeof(LVar));
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl _main\n");

  // 関数定義を先に出力
  bool has_main = false;
  for (int i = 0; code[i]; i++) {
    if (code[i]->kind == ND_FUNC) {
      if (strcmp(code[i]->funcname, "main") == 0) {
        has_main = true;
      }
      gen(code[i]);
    }
  }

  // main関数が定義されていない場合、デフォルトのmainを生成
  if (!has_main) {
    printf("_main:\n");
    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 416\n");

    for (int i = 0; code[i]; i++) {
      if (code[i]->kind != ND_FUNC) {
        gen(code[i]);
        printf("  pop rax\n");
      }
    }

    // エピローグ
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }

  return 0;
}