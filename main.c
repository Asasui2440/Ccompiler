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

  // グローバル変数の宣言を出力
  printf("\n.data\n");
  for (GVar* gvar = globals; gvar; gvar = gvar->next) {
    printf("_%s:\n", gvar->name);
    if (gvar->type->ty == ARRAY) {
      // 配列の場合：サイズ分のゼロを確保
      printf("  .zero %zu\n",
             gvar->type->array_size * size_of(gvar->type->ptr_to));
    } else if (gvar->type->ty == CHAR) {
      // char型：1バイト確保
      printf("  .byte 0\n");
    } else {
      // int型、ポインタ型：8バイト確保(値は0に初期化)
      printf("  .quad 0\n");
    }
  }
  printf("\n.text\n");

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