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

  // 関数定義を出力
  for (int i = 0; code[i]; i++) {
    if (code[i]->kind == ND_FUNC) {
      gen(code[i]);
    }
  }

  // エピローグ
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");

  return 0;
}