#include "9cc.h"

int flag_cnt = 0;
void gen_lval(Node* node) {
  if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node* node) {
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
    printf("  je  .Lend%d\n", flag_cnt);
    gen(node->then);
    printf(".Lend%d:\n", flag_cnt);
    flag_cnt++;
    return;
  }

  if (node->kind == ND_IF && node->els != NULL) {
    int lelse = flag_cnt;
    int lend = flag_cnt + 1;
    flag_cnt += 2;
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
    int lbegin = flag_cnt;
    int lend = flag_cnt + 1;
    printf(".Lbegin%d:\n", lbegin);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", lend);
    gen(node->body);
    // ループ内で return 文が実行された場合、ループを終了する
    printf("  jmp .Lbegin%d\n", lbegin);
    printf(".Lend%d:\n", lend);
    flag_cnt += 2;
    return;
  }

  if (node->kind == ND_FOR) {
    int lbegin = flag_cnt;
    int lend = flag_cnt + 1;
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
    flag_cnt += 2;
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