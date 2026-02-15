#ifndef _9CC_H_
#define _9CC_H_

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,      // +
  ND_SUB,      // -
  ND_MUL,      // *
  ND_DIV,      // /
  ND_NUM,      // 整数
  ND_CHAR,     // 文字型(char)
  ND_STR,      // 文字列リテラル
  ND_EQ,       // ==
  ND_NE,       // !=
  ND_LE,       // <=
  ND_LT,       // <
  ND_ASSIGN,   // =
  ND_LVAR,     // ローカル変数
  ND_GVAR,     // グローバル変数
  ND_RETURN,   // return
  ND_IF,       // if
  ND_FOR,      // for
  ND_WHILE,    // while
  ND_BLOCK,    // block
  ND_CALL,     // 関数呼び出し
  ND_FUNC,     // 関数定義
  ND_FUNCDEF,  // 関数パラメータ
  ND_ADDR,     // &
  ND_DEREF,    // *
  ND_DECL,     // 変数宣言
} NodeKind;

// トークンの種類
typedef enum {
  TK_RESERVED,  // 記号
  TK_IDENT,     // 識別子
  TK_NUM,       // 整数トークン
  TK_CHAR,      // 文字型(char)
  TK_STR,       // 文字列リテラル
  TK_EOF,       // 入力の終わりを表すトークン
  TK_RETURN,    // return
  TK_IF,        // if
  TK_ELSE,      // else
  TK_FOR,       // for
  TK_WHILE,     // while
  TK_INT,       // int
  TK_SIZEOF,    // sizeof
} TokenKind;

typedef struct Node Node;
typedef struct Token Token;
typedef struct LVar LVar;
typedef struct GVar GVar;
typedef struct Type Type;
typedef struct Str_vec Str_vec;

// 型定義
struct Type {
  enum { INT, PTR, ARRAY, CHAR } ty;
  Type* ptr_to;
  size_t array_size;
};

// ローカル変数の型
struct LVar {
  LVar* next;  // 次の変数かNULL
  char* name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
  Type* type;
};

struct GVar {
  GVar* next;  // 次の変数かNULL
  char* name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
  Type* type;
};

// トークン型
struct Token {
  TokenKind kind;  // トークンの型
  Token* next;     // 次の入力トークン
  int val;         // kindがTK_NUMの場合、その数値
  char* str;       // トークン文字列
  int len;         // トークンの長さ
};

// 抽象構文木のノードの型
struct Node {
  NodeKind kind;   // ノードの型
  Node* lhs;       // 左辺
  Node* rhs;       // 右辺
  int val;         // kindがND_NUMの場合のみ使う
  int offset;      // kindがND_LVARの場合のみ使う
  char* funcname;  // 関数名
  int str_label;   // 文字列リテラルのラベル番号（ND_STRの場合）
  struct {
    Node* cond;  // ifの条件式
    Node* then;  // 真の時
    Node* els;   // 偽の時
    Node* init;  // forの初期条件
    Node* inc;   // 更新式
    Node* body;  // 文（ND_FUNCの場合は関数本体）
  };

  Node** stmts;  // ND_BLOCKの文リスト / ND_CALLの引数リスト
  int stmts_len;
  Node** params;   // ND_FUNCの引数リスト
  int params_len;  // 引数の数
  Type* type;      // 型
};

typedef struct {
  Node** data;
  int capacity;
  int len;
} Vector;

// 文字列リテラルを保存する構造体
struct Str_vec {
  char* str;      // 文字列の内容
  int len;        // 文字列の長さ
  int label;      // ラベル番号
  Str_vec* next;  // 次の文字列
};

// グローバル変数
extern Token* token;
extern char* user_input;
extern Node* code[100];
extern LVar* locals;
extern GVar* globals;
extern int label_number;
extern Vector* stms;
extern Str_vec* strings;  // 文字列リテラルのリスト

// util.c
void error(char* fmt, ...);
void error_at(char* loc, char* input, char* fmt, ...);
Vector* new_vector();
void vec_push(Vector* vec, Node* elem);

// tokenizer.c
Token* tokenize(char* p);

// parse.c
bool consume(char* op);
Token* consume_ident();
bool consume_return();
bool consume_if();
bool consume_while();
bool consume_for();
bool consume_else();
void expect(char* op);
int expect_number();
bool at_eof();
void program();
LVar* find_lvar(Token* tok);
GVar* find_gvar(Token* tok);
Node* stmt();
Node* expr();
Node* assign();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();
Node* new_node(NodeKind kind);
Node* new_binary(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_num(int val);
Node* top_level();
Node* global_def(Token* tok);
Node* function(Token* tok);
Node* add_str_to_vec();

// codegen.c
void gen(Node* node);
int size_of(Type* type);
void gen_comment(const char* format, ...);

#endif
