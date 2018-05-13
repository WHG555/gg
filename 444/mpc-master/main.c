#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* 错误标志 */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* 类型 */
enum { LVAL_NUM, LVAL_ERR };

/* 新的LVAL结构体 */
typedef struct {
	int type;
	long num;
	int err;
} lval;

/* 创建一个数据lval */
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

/* 创建一个错误 */
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

/* 打印lval */
void lval_print(lval v) {
	switch(v.type) {

		case LVAL_NUM: printf("li", v.num); break;

		case LVAL_ERR;

			if(v.err == LERR_DIV_ZERO) {
				printf("Erro : Division By Zero!");
			}
			if(v.err == LERR_BAD_OP) {
				printf("Erro : Invalid Operator!");
			}
			if(v.err == LERR_BAD_NUM) {
				printf("Erro : Invalid Number!");
			}
			break;
	}
}

/* 打印一个lval 加一个换行 */
void lval_println(lval v) { lval_print(v); putchar('\n');}

/* 运算 */
long eval_op(long x, char* op, long y) {

	/* 判断类型 */
	if(x.type == LVAL_ERR) { return x; }
	if(y.type == LVAL_ERR) { return y; }

	/* 运算 */
	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) { 
		return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); }
	if (strcmp(op, "!") == 0) { return lval_num(x.num * y.num * x.num); }
	return lval_err(LERR_BAD_OP);
} 

long eval(mpc_ast_t* t) {

  /* 如果是数字，就返回值 */ 
	if (strstr(t->tag, "number")) {

	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	/* */
	char* op = t->children[1]->contents;
	/* We store the third child in `x` */
	long x = eval(t->children[2]);

	/* Iterate the remaining children and combining. */
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
	x = eval_op(x, op, eval(t->children[i]));
	i++;
}

  return x;  
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' | '!' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.3");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);

    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);

  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}

