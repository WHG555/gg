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

/* 表达式类型 */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

typedef struct lval {
	int type;
	long num;
	/* 错误类型和符号类型 */
	char* err;
	char* sym;
	/* 记数和指向下一个lval */
	int count;
	struct lval** cell;
}lval;

/* 所有函数 */
void lval_print(lval* v);
/*  */
lval* lval_eval(lval* v); 
lval* lval_read(mpc_ast_t* t);
lval* lval_take(lval* v, int i); 
lval* builtin_op(lval* a, char* op);
/* 取出一个 */
lval* lval_pop(lval* v, int i);
	

/* 创建一个数据lval */
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* 创建一个错误 */
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

/* 指针指向一个新的symbol */
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

/* 指向一个新的sexpr */
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/* 删除一个lvel */
void lval_del(lval* v) {
	switch(v->type) {
		
		case LVAL_NUM: break;

		case LVAL_ERR: free(v->err); break;

		case LVAL_SYM: free(v->sym); break;

		case LVAL_SEXPR: 
			for(int i = 0; i<v->count; i++) {
				lval_del(v->cell[i]);
			}
			free(v->cell);
			break;
	}
	/* 删除一个lval */
	free(v);
}

/* 读数字 */
lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}


lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

/* 打印lval表达式 */
void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for(int i=0; i<v->count; i++) {
		lval_print(v->cell[i]);
		if(i != (v->count-1)) {
			putchar(' ');
		}
	}
}
/* 打印lval */
void lval_print(lval* v) {
	switch(v->type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
	}
}

/* 打印一个lval 加一个换行 */
void lval_println(lval* v) { lval_print(v); putchar('\n');}


/* 计算表达式 */
lval* lval_eval_sexpr(lval* v) {
	for(int i = 0; i<v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	/* 错误检查 */
	for(int i=0; i<v->count; i++) {
		if(v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}

	/* 空的表达式 */
	if(v->count == 0) { return v; }

	/* 简单的表达式 */
	if(v->count == 1) { return lval_take(v, 0); }

	lval* f = lval_pop(v, 0);
	if(f->type != LVAL_SYM) {
		lval_del(f); lval_del(v);
		return lval_err("S-expression Does not start with symbol");
	}

	/* 进行运算 */
	lval* result = builtin_op(v, f->sym);
	lval_del(f);
	return result;
}

/*  */
lval* lval_eval(lval* v) {
	if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
	/* 如果是其他的就直接返回 */
	return v;
}

/* 运算 */
lval* builtin_op(lval* a, char* op) {
	
	for(int i = 0; i<a->count; i++) {
		if(a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on non-number!");
		}
	}

	/* 取出一个 */
	lval* x = lval_pop(a, 0);

	if((strcmp(op, "-") == 0) && a->count == 0) {
		x->num = -x->num;
	}

	while(a->count > 0) {
		lval* y = lval_pop(a,0);

		if(strcmp(op, "+") == 0) { x->num += y->num; }
		if(strcmp(op, "-") == 0) { x->num -= y->num; }
		if(strcmp(op, "*") == 0) { x->num *= y->num; }
		if(strcmp(op, "/") == 0) {
			if(y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("Division By Zero!");
				break;
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

/*  */
lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

/* 取出一个 */
lval* lval_pop(lval* v, int i) {
	lval* x = v->cell[i];

	memmove(&v->cell[i], &v->cell[i+1],
			sizeof(lval*) * (v->count-i-1));

	v->count--;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_read(mpc_ast_t* t) {
	if(strstr(t->tag, "number")) {return lval_read_num(t); }
	if(strstr(t->tag, "symbol")) {return lval_sym(t->contents); }
	
	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if(strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
	
	for(int i = 0; i<t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if(strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}


int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");  //数字
  mpc_parser_t* Symbol = mpc_new("symbol"); //符号
  mpc_parser_t* Sexpr = mpc_new("sexpr"); 
  mpc_parser_t* Expr = mpc_new("expr"); //表达式
  mpc_parser_t* Lispy = mpc_new("lispy"); //语言名字

  mpca_lang(MPCA_LANG_DEFAULT,
    "														\
      number   : /-?[0-9]+/ ;								\
      symbol   : '+' | '-' | '*' | '/' | '!' ;				\
	  sexpr    : '(' <expr>* ')' ;							\
      expr     : <number> | <symbol> | <sexpr> ;			\
      lispy    : /^/ <expr>* /$/ ;						\
    ",
    Number, Symbol, Sexpr, Expr, Lispy);

  puts("Lispy Version 0.5");
  puts("Press Ctrl+c to Exit\n");

  while (1) {

    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
	  lval_del(x);
      mpc_ast_delete(r.output);

    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);

  }

  mpc_cleanup(4, Number, Symbol, Sexpr, Expr, Lispy);

  return 0;
}

