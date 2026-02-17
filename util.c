#include <errno.h>
#include <string.h>

#include "9cc.h"

char* filename;

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void error_at(char* loc, char* msg, ...) {
  // locが含まれている行の開始地点と終了地点を取得
  char* line = loc;
  while (user_input < line && line[-1] != '\n') line--;

  char* end = loc;
  while (*end != '\n') end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char* p = user_input; p < line; p++)
    if (*p == '\n') line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で指し示して、エラーメッセージを表示
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");  // pos個の空白を出力
  fprintf(stderr, "^ %s\n", msg);
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

// 指定されたファイルの内容を返す
char* read_file(char* path) {
  // ファイルを開く
  FILE* fp = fopen(path, "r");
  filename = path;
  if (!fp) error("cannot open %s: %s", path, strerror(errno));

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  // ファイル内容を読み込む
  char* buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n') buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}
