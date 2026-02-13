#include "9cc.h"

// グローバル変数の実体
Node* code[100];
LVar* locals;
Vector* stms;

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char* op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// ここではtokenの情報を返す (他はトークンをそのまま読み進めるだけ)
Token* consume_ident() {
  if (token->kind != TK_IDENT) return NULL;
  Token* tok = token;
  token = token->next;
  return tok;
}

bool consume_return() {
  if (token->kind != TK_RETURN) return false;
  token = token->next;
  return true;
}

bool consume_int() {
  if (token->kind != TK_INT) return false;
  token = token->next;
  return true;
}

bool consume_if() {
  if (token->kind != TK_IF) return false;
  token = token->next;
  return true;
}

bool consume_while() {
  if (token->kind != TK_WHILE) return false;
  token = token->next;
  return true;
}

bool consume_for() {
  if (token->kind != TK_FOR) return false;
  token = token->next;
  return true;
}

bool consume_else() {
  if (token->kind != TK_ELSE) return false;
  token = token->next;
  return true;
}

bool consume_sizeof() {
  if (token->kind != TK_SIZEOF) return false;
  token = token->next;
  return true;
}

// プログラムの終わり
bool at_eof() { return token->kind == TK_EOF; }

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char* op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, op, "expected \"%s\"");
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, user_input, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// 次のトークンがintの場合、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect_int() {
  if (token->kind != TK_INT) {
    error_at(token->str, user_input, "int型が必要です");
  }
  token = token->next;
}

Node* new_node(NodeKind kind) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node* new_binary(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// intのnodeを作り、その値を登録
Node* new_node_num(int val) {
  Node* node = new_node(ND_NUM);
  node->val = val;

  Type* type = calloc(1, sizeof(Type));
  type->ty = INT;
  node->type = type;
  return node;
}

// 関数定義をパース
Node* function() {
  expect_int();
  Token* tok = consume_ident();
  if (!tok) {
    error("関数名がありません");
  }

  Node* node = new_node(ND_FUNC);
  node->funcname = strndup(tok->str, tok->len);

  // 引数リストをパース
  expect("(");
  Vector* params = new_vector();

  if (!consume(")")) {
    expect_int();
    Token* param = consume_ident();
    if (param) {
      Node* p = new_node(ND_LVAR);
      p->offset = 8;  // 引数の最初のオフセット
      vec_push(params, p);

      LVar* lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = param->str;
      lvar->len = param->len;
      lvar->offset = 8;
      locals = lvar;

      while (consume(",")) {
        param = consume_ident();
        if (!param) error("引数名がありません");

        p = new_node(ND_LVAR);
        p->offset = locals->offset + 8;
        vec_push(params, p);

        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = param->str;
        lvar->len = param->len;
        lvar->offset = locals->offset + 8;
        locals = lvar;
      }
    }
    expect(")");
  }

  node->params = params->data;
  node->params_len = params->len;
  free(params);

  // 関数本体をパース
  node->body = stmt();

  return node;
}

void program() {
  int i = 0;

  // トップレベルでは関数定義のみを許可
  while (!at_eof()) {
    code[i++] = function();
  }
  code[i] = NULL;
}

Node* stmt() {
  Node* node;

  if (consume("{")) {  // ブロックの開始
    node = new_node(ND_BLOCK);
    Vector* stmts = new_vector();

    while (!consume("}")) {  // `}` が出現するまで繰り返す
      vec_push(stmts, stmt());
    }

    node->stmts = stmts->data;
    node->stmts_len = stmts->len;
    free(stmts);  // Vector 構造体自体は解放
    return node;
  }

  if (consume_return()) {
    node = new_node(ND_RETURN);
    node->lhs = expr();  // 式を解析して左辺に格納
    expect(";");
    return node;
  }

  if (consume_if()) {
    node = new_node(ND_IF);
    expect("(");
    node->cond = expr();  // 条件式
    expect(")");
    node->then = stmt();                     // then ブロック
    if (consume_else()) node->els = stmt();  // else ブロック（オプション）
    return node;
  }

  if (consume_while()) {
    node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();  // 条件式
    expect(")");
    node->body = stmt();  // ループ本体
    return node;
  }

  if (consume_for()) {
    node = new_node(ND_FOR);
    expect("(");
    if (!consume(";")) {
      node->init = expr();  // 初期化式
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();  // 条件式
      expect(";");
    }
    if (!consume(")")) {
      node->inc = expr();  // 更新式
      expect(")");
    }
    node->body = stmt();  // ループ本体
    return node;
  }

  // 通常の式文
  node = expr();
  expect(";");
  return node;
}

Node* primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node* node = expr();
    expect(")");
    return node;
  }

  if (consume_int()) {
    Type* typ = calloc(1, sizeof(Type));
    typ->ty = INT;

    while (consume("*")) {
      Type* ptr_type = calloc(1, sizeof(Type));
      ptr_type->ty = PTR;      // ポインタ型に設定
      ptr_type->ptr_to = typ;  // 元の型を参照
      typ = ptr_type;          // 現在の型を更新
    }

    Node* node = new_node(ND_DECL);  // ND_LVAR -> ND_DECL に変更
    Token* tok = consume_ident();
    if (!tok) {
      error("変数名がありません");
    }
    LVar* lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->type = typ;  // 型情報を保存

    if (locals) {
      lvar->offset = locals->offset + 8;
    } else {
      lvar->offset = 8;
    }
    node->offset = lvar->offset;
    node->type = lvar->type;
    locals = lvar;
    return node;
  }

  Token* tok = consume_ident();
  if (tok) {
    Node* node = calloc(1, sizeof(Node));

    // 関数呼び出しだった場合
    if (consume("(")) {
      node->kind = ND_CALL;
      node->funcname = strndup(tok->str, tok->len);
      // node->type->ty = INT;
      Vector* args = new_vector();

      // 引数がある時
      if (!consume(")")) {
        vec_push(args, expr());
        while (consume(",")) {
          vec_push(args, expr());
        }
        expect(")");
      }
      node->stmts = args->data;
      node->stmts_len = args->len;
      free(args);
      return node;

    } else {
      // 変数の場合
      node->kind = ND_LVAR;
      LVar* lvar = find_lvar(tok);
      if (lvar) {
        node->offset = lvar->offset;
        node->type = lvar->type;  // 変数の型情報を設定
      } else {
        error("変数が定義されていません\n");
      }
      return node;
    }
  }

  // そうでなければ数値のはず
  Node* node = new_node_num(expect_number());
  return node;
}

Node* mul() {
  Node* node = unary();

  for (;;) {
    if (consume("*")) {
      Node* rhs = unary();
      node = new_binary(ND_MUL, node, rhs);
      // 乗算・除算の結果はINT型
      if (node->lhs->type && node->rhs->type) {
        Type* int_type = calloc(1, sizeof(Type));
        int_type->ty = INT;
        node->type = int_type;
      }
    } else if (consume("/")) {
      Node* rhs = unary();
      node = new_binary(ND_DIV, node, rhs);
      // 乗算・除算の結果はINT型
      if (node->lhs->type && node->rhs->type) {
        Type* int_type = calloc(1, sizeof(Type));
        int_type->ty = INT;
        node->type = int_type;
      }
    } else {
      return node;
    }
  }
}

Node* equality() {
  Node* node = relational();
  for (;;) {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

Node* relational() {
  Node* node = add();
  for (;;) {
    if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else
      return node;
  }
}
Node* expr() { return assign(); }

Node* assign() {
  Node* node = equality();
  if (consume("=")) node = new_binary(ND_ASSIGN, node, assign());
  return node;
}

Node* unary() {
  if (consume_sizeof()) {
    Node* lhs = unary();

    // sizeofの中身がポインタなら8を返す
    if (lhs->type && lhs->type->ty == PTR) {
      return new_node_num(8);

      // 中身がintなら4を返す
    } else if (lhs->type && lhs->type->ty == INT) {
      return new_node_num(4);
    }
    error("sizeofの中身がポインタでもint型でもありません");
  }
  if (consume("+")) return primary();
  if (consume("-")) return new_binary(ND_SUB, new_node_num(0), primary());
  if (consume("*")) {
    Node* node = new_node(ND_DEREF);
    node->lhs = unary();
    // *演算子の結果は、ポインタが指す型になる
    if (node->lhs->type && node->lhs->type->ty == PTR) {
      node->type = node->lhs->type->ptr_to;
    }
    return node;
  }
  if (consume("&")) {
    Node* node = new_node(ND_ADDR);
    node->lhs = unary();
    // &演算子の結果はポインタ型になる
    if (node->lhs->type) {
      Type* ptr_type = calloc(1, sizeof(Type));
      ptr_type->ty = PTR;

      // &a (aはint) なら、 &aの型は pointer → int (intのポインタ)
      ptr_type->ptr_to = node->lhs->type;
      node->type = ptr_type;
    }
    return node;
  }
  return primary();
}

Node* add() {
  Node* node = mul();
  for (;;) {
    if (consume("+")) {
      Node* rhs = mul();
      node = new_binary(ND_ADD, node, rhs);

      // 型推論: ポインタ演算があればポインタ型、それ以外はINT型
      if ((node->lhs->type && node->lhs->type->ty == PTR) ||
          (node->rhs->type && node->rhs->type->ty == PTR)) {
        Type* ptr_type = calloc(1, sizeof(Type));
        ptr_type->ty = PTR;
        node->type = ptr_type;
      } else if (node->lhs->type && node->rhs->type) {
        Type* int_type = calloc(1, sizeof(Type));
        int_type->ty = INT;
        node->type = int_type;
      }
    } else if (consume("-")) {
      Node* rhs = mul();
      node = new_binary(ND_SUB, node, rhs);

      // 引き算の時も同様
      if ((node->lhs->type && node->lhs->type->ty == PTR) ||
          (node->rhs->type && node->rhs->type->ty == PTR)) {
        Type* ptr_type = calloc(1, sizeof(Type));
        ptr_type->ty = PTR;
        node->type = ptr_type;
      } else if (node->lhs->type && node->rhs->type) {
        Type* int_type = calloc(1, sizeof(Type));
        int_type->ty = INT;
        node->type = int_type;
      }
    } else {
      return node;
    }
  }
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar* find_lvar(Token* tok) {
  for (LVar* var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}
