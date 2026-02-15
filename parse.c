#include "9cc.h"

// グローバル変数の実体
Node* code[100];
LVar* locals;
GVar* globals;
Vector* stms;
Type* type;
Str_vec* strings;

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar* find_lvar(Token* tok) {
  for (LVar* var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// グローバル変数を検索する。見つからなかった場合はNULLを返す。
GVar* find_gvar(Token* tok) {
  for (GVar* var = globals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char* op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 型の前半を読んでtokenを進める。型を返す
Type* consume_type() {
  if (token->kind != TK_INT && token->kind != TK_CHAR) {
    error("型ではありません");
  }
  Type* type = calloc(1, sizeof(Type));
  if (token->kind == TK_INT) {
    type->ty = INT;
  } else if (token->kind == TK_CHAR) {
    type->ty = CHAR;
  }
  token = token->next;
  while (consume("*")) {
    Type* new_type = calloc(1, sizeof(Type));
    new_type->ty = PTR;
    new_type->ptr_to = type;
    type = new_type;
  }

  return type;
}

bool is_next_token(char* op) {
  if (memcmp(token->next->str, op, token->next->len) == 0) {
    return true;
  } else {
    return false;
  }
}

// ここではtokenの情報を返す。tokenを一つ読み進める
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

Node* top_level() {
  type = consume_type();
  Token* tok = consume_ident();

  if (!tok) {
    error("関数名またはグローバル変数名がありません");
  }

  // consume_ident()の後、tokenは次のトークンを指しているので、
  // 現在のtokenが"("かどうかを確認
  if (token->kind == TK_RESERVED && token->len == 1 && token->str[0] == '(') {
    return function(tok);
  } else {
    return global_def(tok);
  }
}

Node* global_def(Token* tok) {
  // グローバル変数宣言
  GVar* gvar = calloc(1, sizeof(GVar));
  gvar->next = globals;
  gvar->name = strndup(tok->str, tok->len);
  gvar->len = tok->len;

  // 配列だった時: int a[10]など
  if (consume("[")) {
    Type* array_type = calloc(1, sizeof(Type));
    array_type->ty = ARRAY;
    array_type->array_size = expect_number();
    array_type->ptr_to = type;  // 配列の要素型
    type = array_type;
    expect("]");
  }

  gvar->type = type;

  // オフセットを計算
  if (globals) {
    if (type->ty == ARRAY) {
      // 配列の場合: 配列サイズ × 要素のサイズ
      gvar->offset = globals->offset + type->array_size * size_of(type->ptr_to);
    } else {
      // スカラー変数：型のサイズを使う
      gvar->offset = globals->offset + size_of(type);
    }
  } else {
    if (type->ty == ARRAY) {
      // 配列の場合: 配列サイズ × 要素のサイズ
      gvar->offset = type->array_size * size_of(type->ptr_to);
    } else {
      // スカラー変数：型のサイズを使う
      gvar->offset = size_of(type);
    }
  }
  globals = gvar;

  expect(";");
  // 変数宣言は式として値を返さないので空のノードを返す
  Node* node = new_node(ND_DECL);
  node->offset = gvar->offset;
  node->type = gvar->type;
  return node;
}

// 関数定義をパース
Node* function(Token* tok) {
  // 新しい関数を解析するので、ローカル変数リストをリセット
  locals = NULL;

  Node* node = new_node(ND_FUNC);
  node->funcname = strndup(tok->str, tok->len);
  node->type = type;

  // 引数リストをパース
  expect("(");
  Vector* params = new_vector();

  if (!consume(")")) {
    Type* arg_type = consume_type();
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
      lvar->type = arg_type;
      locals = lvar;

      while (consume(",")) {
        arg_type = consume_type();
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
        lvar->type = arg_type;
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

  // 関数定義またはグローバル変数定義
  while (!at_eof()) {
    code[i++] = top_level();
  }
  code[i] = NULL;
}

Node* stmt() {
  Node* node;

  if (consume("{")) {  // ブロックの開始
    node = new_node(ND_BLOCK);
    Vector* stmts = new_vector();

    while (!consume("}")) {  // `}` が出現するまで繰り返す
      if (at_eof()) {
        error_at(token->str, user_input, "'}'が見つかりません");
      }
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

  // 変数の宣言（intまたはchar）
  if (token->kind == TK_INT || token->kind == TK_CHAR) {
    Type* typ = consume_type();

    Token* tok = consume_ident();
    if (!tok) {
      error("変数名がありません");
    }

    LVar* lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;

    // 配列だった時: int a[10]など
    if (consume("[")) {
      Type* array_type = calloc(1, sizeof(Type));
      array_type->ty = ARRAY;
      array_type->array_size = expect_number();
      array_type->ptr_to = typ;  // 配列の要素型
      typ = array_type;
      expect("]");
    }

    lvar->type = typ;

    // オフセットを計算
    if (locals) {
      if (typ->ty == ARRAY) {
        // 配列の場合: 配列サイズ × 要素のサイズ
        lvar->offset = locals->offset + typ->array_size * size_of(typ->ptr_to);
      } else {
        // スカラー変数：型のサイズを使う（ただし最低8バイト確保）
        int var_size = size_of(typ);
        if (var_size < 8) var_size = 8;  // スタックアライメントのため
        lvar->offset = locals->offset + var_size;
      }
    } else {
      if (typ->ty == ARRAY) {
        // 配列の場合: 配列サイズ × 要素のサイズ
        lvar->offset = typ->array_size * size_of(typ->ptr_to);
      } else {
        // スカラー変数：型のサイズを使う（ただし最低8バイト確保）
        int var_size = size_of(typ);
        if (var_size < 8) var_size = 8;  // スタックアライメントのため
        lvar->offset = var_size;
      }
    }
    locals = lvar;

    expect(";");
    // 変数宣言は式として値を返さないので空のノードを返す
    Node* node = new_node(ND_DECL);
    node->offset = lvar->offset;
    node->type = lvar->type;
    return node;
  }

  // 通常の式文
  node = expr();
  expect(";");
  return node;
}

Node* add_str_to_vec() {
  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_STR;

  // 文字列リテラルをvectorに追加
  Str_vec* str = calloc(1, sizeof(Str_vec));
  str->str = strndup(token->str, token->len);
  str->len = token->len;
  str->label = label_number++;
  str->next = strings;
  strings = str;

  node->str_label = str->label;

  // 文字列リテラルの型は char* (char へのポインタ)
  Type* ptr_type = calloc(1, sizeof(Type));
  ptr_type->ty = PTR;
  Type* char_type = calloc(1, sizeof(Type));
  char_type->ty = CHAR;
  ptr_type->ptr_to = char_type;
  node->type = ptr_type;

  token = token->next;
  return node;
}

Node* primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node* node = expr();
    expect(")");
    return node;
  }

  // 文字列リテラル
  if (token->kind == TK_STR) {
    return add_str_to_vec();
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
      LVar* lvar = find_lvar(tok);
      GVar* gvar = find_gvar(tok);

      if (lvar) {
        node->kind = ND_LVAR;
        node->offset = lvar->offset;
        node->type = lvar->type;
      } else if (gvar) {
        node->kind = ND_GVAR;
        node->funcname =
            strndup(gvar->name, gvar->len);  // グローバル変数名を保存
        node->offset = gvar->offset;
        node->type = gvar->type;
      } else {
        error("変数が宣言されていません");
      }

      // 配列アクセス: a[i]
      if (consume("[")) {
        Node* index = expr();
        expect("]");

        // 配列アクセスはポインタ演算として扱う (a[i] は *(a + i) と同じ)
        Node* array_addr = calloc(1, sizeof(Node));
        if (lvar) {
          array_addr->kind = ND_LVAR;
          array_addr->offset = lvar->offset;
          array_addr->type = lvar->type;
        } else {
          array_addr->kind = ND_GVAR;
          array_addr->funcname = strndup(gvar->name, gvar->len);
          array_addr->offset = gvar->offset;
          array_addr->type = gvar->type;
        }

        Node* addr = new_binary(ND_ADD, array_addr, index);
        // 型の伝播：ポインタ型として設定
        Type* var_type = lvar ? lvar->type : gvar->type;
        if (var_type->ty == ARRAY && var_type->ptr_to) {
          Type* ptr_type = calloc(1, sizeof(Type));
          ptr_type->ty = PTR;
          ptr_type->ptr_to = var_type->ptr_to;
          addr->type = ptr_type;
        }

        Node* deref = new_node(ND_DEREF);
        deref->lhs = addr;
        if (var_type->ty == ARRAY && var_type->ptr_to) {
          deref->type = var_type->ptr_to;
        }
        return deref;
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
      // 配列型もポインタとして扱う
      int lhs_is_ptr = (node->lhs->type && (node->lhs->type->ty == PTR ||
                                            node->lhs->type->ty == ARRAY));
      int rhs_is_ptr = (node->rhs->type && (node->rhs->type->ty == PTR ||
                                            node->rhs->type->ty == ARRAY));

      if (lhs_is_ptr || rhs_is_ptr) {
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
      // 配列型もポインタとして扱う
      int lhs_is_ptr = (node->lhs->type && (node->lhs->type->ty == PTR ||
                                            node->lhs->type->ty == ARRAY));
      int rhs_is_ptr = (node->rhs->type && (node->rhs->type->ty == PTR ||
                                            node->rhs->type->ty == ARRAY));

      if (lhs_is_ptr || rhs_is_ptr) {
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
