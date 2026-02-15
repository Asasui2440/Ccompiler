#include <stdarg.h>  // 可変引数を扱うために必要

#include "9cc.h"

int label_number = 0;
void gen_lval(Node* node) {
  if (node->kind == ND_LVAR) {
    gen_comment("ローカル変数のアドレスを取得する");
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_GVAR) {
    gen_comment("グローバル変数のアドレスを取得する");
    printf("  lea rax, [rip + _%s]\n", node->funcname);
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_DEREF) {
    gen(node->lhs);  // ポインタの値（アドレス）を計算
    return;          // そのアドレスがそのまま左辺値
  }

  error("代入の左辺値が変数でもポインタでもありません");
}

int size_of(Type* type) {
  if (type->ty == PTR) return 8;  // ポインタは8バイト
  if (type->ty == INT) return 4;  // intは4バイト（配列の要素サイズ）
  error("sizeofの中身がポインタでもint型でもありません");
}

void gen_comment(const char* format, ...) {
  printf("# ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);  // 可変引数を処理
  va_end(args);
  printf("\n");
}

void gen(Node* node) {
  if (node->kind == ND_FUNC) {
    // 関数定義のコード生成
    printf("\n_%s:\n", node->funcname);
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");  // ローカル変数用のスタック領域を確保

    // 引数をスタックに保存（x86-64呼び出し規約に従う）
    char* arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < node->params_len && i < 6; i++) {
      // 引数をメモリに保存（すべて8バイトとして扱う）
      printf("  mov [rbp-%d], %s\n", node->params[i]->offset, arg_regs[i]);
    }

    // 関数本体を生成
    gen(node->body);
    return;
  }

  if (node->kind == ND_BLOCK) {
    for (int i = 0; i < node->stmts_len; i++) {
      gen(node->stmts[i]);
      // return文と変数の宣言以外の場合のみpopする
      if (node->stmts[i]->kind != ND_RETURN &&
          node->stmts[i]->kind != ND_DECL) {
        printf("  pop rax\n");
      }
    }
    return;
  }

  if (node->kind == ND_RETURN) {
    gen(node->lhs);
    gen_comment("リターンする");
    printf("  pop rax\n");  // スタックから値を取り出して rax に設定
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");  // rbpを戻す
    printf("  ret\n");
    return;
  }

  // if (A) B
  if (node->kind == ND_IF && node->els == NULL) {
    gen(node->cond);
    gen_comment("IF (A) B");
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", label_number);
    gen(node->then);
    printf(".Lend%d:\n", label_number);
    label_number++;
    return;
  }

  // if (A) B else C
  if (node->kind == ND_IF && node->els != NULL) {
    int lelse = label_number;
    int lend = label_number + 1;
    label_number += 2;
    gen(node->cond);
    gen_comment("IF (A) B ELSE C");
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
    gen_comment("WHILE文");
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
    gen_comment("FOR文");
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

    printf("  call _%s\n", node->funcname);
    printf("  push rax\n");  // 関数の戻り値をスタックにプッシュ
    return;
  }

  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_DECL:
      return;
    case ND_LVAR:
      gen_lval(node);
      // 配列型の場合はアドレスをそのまま使う（配列からポインタへの減衰）
      if (node->type && node->type->ty == ARRAY) {
        // アドレスがスタックに積まれている状態でそのまま返す
        return;
      }
      // 通常の変数の場合は値をロード（すべて8バイトとして扱う）
      gen_comment("右辺値として変数の値を取得");
      printf("  pop rax\n");         // raxにアドレスの値が入っているはず
      printf("  mov rax, [rax]\n");  // すべて64ビットとして読み込む
      printf("  push rax\n");        // ロードした値をpush
      return;
    case ND_GVAR:
      gen_lval(node);
      // 配列型の場合はアドレスをそのまま使う（配列からポインタへの減衰）
      if (node->type && node->type->ty == ARRAY) {
        // アドレスがスタックに積まれている状態でそのまま返す
        return;
      }
      // 通常の変数の場合は値をロード（すべて8バイトとして扱う）
      gen_comment("右辺値としてグローバル変数の値を取得");
      printf("  pop rax\n");         // raxにアドレスの値が入っているはず
      printf("  mov rax, [rax]\n");  // すべて64ビットとして読み込む
      printf("  push rax\n");        // ロードした値をpush
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen_comment("ASSIGN : 左辺値として変数の値を取得");
      gen(node->rhs);

      // スタックのトップにある右辺値を取り出してrdiに格納
      printf("  pop rdi\n");
      // スタックの次の値(左辺値のアドレスを取り出す)
      printf("  pop rax\n");
      // ポインタ経由のアクセス（配列要素など）の場合は4バイトアクセス
      if (node->lhs->kind == ND_DEREF) {
        // デリファレンス結果の型をチェック
        if (node->lhs->type && node->lhs->type->ty == PTR) {
          // ポインタ型への代入は8バイト
          printf("  mov [rax], rdi\n");
        } else {
          // int型（配列要素）への代入は4バイト
          printf("  mov [rax], edi\n");
        }
      } else {
        // スカラー変数は8バイト
        printf("  mov [rax], rdi\n");
      }
      printf("  push rdi\n");
      return;
    case ND_ADDR:
      gen_lval(node->lhs);  // nodeのアドレスを取得すれば良い
      gen_comment("&演算 : &%d", node->lhs->val);
      return;
    case ND_DEREF:
      gen(node->lhs);  // まず値を計算する
      gen_comment("単項*の計算");
      printf("  pop rax\n");  // スタックのtopにある値を取得
      // デリファレンス結果の型に応じてメモリアクセスサイズを決定
      if (node->type && node->type->ty == PTR) {
        // ポインタ型の場合は8バイト
        printf("  mov rax, [rax]\n");
      } else {
        // int型（配列要素など）の場合は4バイト
        printf("  movsxd rax, DWORD PTR [rax]\n");
      }
      printf("  push rax\n");
      return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      // ポインタ + 整数の場合、整数側に要素サイズを掛ける
      // 配列型の場合もポインタとして扱う
      if (node->lhs->type &&
          (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)) {
        gen_comment("ポインタの足し算");
        int size = node->lhs->type->ty == PTR
                       ? size_of(node->lhs->type->ptr_to)
                       : size_of(node->lhs->type->ptr_to);
        printf("  imul rdi, %d\n", size);
      }
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      // ポインタ - 整数の場合
      // 配列型の場合もポインタとして扱う
      if (node->lhs->type &&
          (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)) {
        gen_comment("ポインタの引き算");
        int size =
            size_of(node->lhs->type->ptr_to);  // ポインタが指す型のサイズ
        printf("  imul rdi, %d\n", size);
      }
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      // cqo .. raxに入っている64ビットの値を128ビットに引き延ばして
      // rdxとraxにセットする
      // idiv rdi ... raxをrdiで割って商をraxに、余りをrdxにセットする
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:  // ==
      printf("  cmp rax, rdi\n");
      // sete... cmpで比較したレジスタが同じなら1,
      // 違ったら0をALレジスタにセットする AL ..
      // raxの下位8ビットを指すレジスタ
      // rax全部を0か1にセットするので、上位56ビットをmovzx命令でゼロクリアする
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