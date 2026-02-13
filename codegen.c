#include "9cc.h"

int label_number = 0;
void gen_lval(Node* node) {
  if (node->kind == ND_LVAR) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_DEREF) {
    // デリファレンスの場合、中身を「右辺値」として評価
    gen(node->lhs);  // ポインタの値（アドレス）を計算
    return;          // そのアドレスがそのまま左辺値
  }

  error("代入の左辺値が変数でもポインタでもありません");
}

void gen(Node* node) {
  if (node->kind == ND_FUNC) {
    // 関数定義のコード生成
    printf("\n_%s:\n", node->funcname);
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 416\n");  // ローカル変数用のスタック領域を確保

    // 引数をスタックに保存（x86-64呼び出し規約に従う）
    char* arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < node->params_len && i < 6; i++) {
      // 引数をメモリに保存
      printf("  mov [rbp-%d], %s\n", node->params[i]->offset, arg_regs[i]);
    }

    // 関数本体を生成
    gen(node->body);

    // 明示的なreturnがない場合のエピローグ
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->kind == ND_BLOCK) {
    for (int i = 0; i < node->stmts_len; i++) {
      gen(node->stmts[i]);
      printf("  pop rax\n");  // 各文の評価結果をポップ
    }
    printf("  push rax\n");  // 最後の文の結果をプッシュ
    return;
  }

  if (node->kind == ND_RETURN) {
    gen(node->lhs);
    printf("  pop rax\n");  // スタックから値を取り出して rax に設定
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->kind == ND_IF && node->els == NULL) {
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", label_number);
    gen(node->then);
    printf(".Lend%d:\n", label_number);
    label_number++;
    return;
  }

  if (node->kind == ND_IF && node->els != NULL) {
    int lelse = label_number;
    int lend = label_number + 1;
    label_number += 2;
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lelse%d\n", lelse);
    gen(node->then);
    printf("  jmp .Lend%d\n", lend);
    printf(".Lelse%d:\n", lelse);
    gen(node->els);
    printf(".Lend%d:\n", lend);
    return;
  }

  if (node->kind == ND_WHILE) {
    int lbegin = label_number;
    int lend = label_number + 1;
    printf(".Lbegin%d:\n", lbegin);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", lend);
    gen(node->body);
    // ループ内で return 文が実行された場合、ループを終了する
    printf("  jmp .Lbegin%d\n", lbegin);
    printf(".Lend%d:\n", lend);
    label_number += 2;
    return;
  }

  if (node->kind == ND_FOR) {
    int lbegin = label_number;
    int lend = label_number + 1;
    if (node->init) gen(node->init);
    printf(".Lbegin%d:\n", lbegin);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", lend);
    }
    gen(node->body);
    if (node->inc) gen(node->inc);
    printf("  jmp .Lbegin%d\n", lbegin);
    printf(".Lend%d:\n", lend);
    label_number += 2;
    return;
  }

  if (node->kind == ND_CALL) {
    char* arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

    // 引数を評価してスタックに積む
    for (int i = 0; i < node->stmts_len; i++) {
      gen(node->stmts[i]);
    }

    // スタックから引数をポップしてレジスタに格納
    // 最後の引数から順にポップして、最初の引数がrdiに入るようにする
    for (int i = node->stmts_len - 1; i >= 0; i--) {
      printf("  pop %s\n", arg_regs[i]);
    }

    printf("  call _%s\n", node->funcname);  // 関数名にアンダースコアを追加
    printf("  push rax\n");                  // 関数の戻り値をスタックにプッシュ
    return;
  }

  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
      return;
    case ND_ADDR:
      gen_lval(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:  // ==
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NE:  // !=
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LE:  // <=
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LT:  // <
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzx rax, al\n");
      break;
    default:
      error("未対応のノード種類です: %d", node->kind);
  }

  printf("  push rax\n");
}