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
  ND_EQ,       // ==
  ND_NE,       // !=
  ND_LE,       // <=
  ND_LT,       // <
  ND_ASSIGN,   // =
  ND_LVAR,     // ローカル変数
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
} NodeKind;

// トークンの種類
typedef enum {
  TK_RESERVED,  // 記号
  TK_IDENT,     // 識別子
  TK_NUM,       // 整数トークン
  TK_EOF,       // 入力の終わりを表すトークン
  TK_RETURN,    // return
  TK_IF,        // if
  TK_ELSE,      // else
  TK_FOR,       // for
  TK_WHILE,     // while
  TK_INT,       // int
} TokenKind;

typedef struct Node Node;
typedef struct Token Token;
typedef struct LVar LVar;
typedef struct Type Type;

// 型定義
struct Type {
  enum { INT, PTR } ty;
  Type* ptr_to;
};

// ローカル変数の型
struct LVar {
  LVar* next;  // 次の変数かNULL
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

// グローバル変数
extern Token* token;
extern char* user_input;
extern Node* code[100];
extern LVar* locals;
extern int label_number;
extern Vector* stms;
// parse.c
void error(char* fmt, ...);
void error_at(char* loc, char* input, char* fmt, ...);
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
Token* tokenize(char* p);
void program();
LVar* find_lvar(Token* tok);
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

// codegen.c
void gen(Node* node);

#endif
