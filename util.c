#include "9cc.h"

// エラーを表示する関数
void error_at(char* loc, char* input, char* fmt, ...) {
  // エラー箇所を示す位置を計算
  int pos = loc - input;

  // 入力全体を表示
  fprintf(stderr, "%s\n", input);

  // エラー箇所を示すカーソルを表示
  fprintf(stderr, "%*s^\n", pos, "");

  // 可変引数を処理してエラーメッセージを表示
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");

  // プログラムを終了
  exit(1);
}

void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Vector構造体の操作関数
Vector* new_vector() {
  Vector* vec = calloc(1, sizeof(Vector));
  vec->capacity = 8;  // 初期容量
  vec->data = calloc(vec->capacity, sizeof(Node*));
  vec->len = 0;
  return vec;
}

void vec_push(Vector* vec, Node* elem) {
  if (vec->len == vec->capacity) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, vec->capacity * sizeof(Node*));
  }
  vec->data[vec->len++] = elem;
}
