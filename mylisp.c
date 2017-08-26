#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "mpc/mpc.h"

#define LASSERT(args, cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            lval *err = lval_err(fmt, ##__VA_ARGS__); \
            lval_del(args); \
            return err; \
        } \
    } while (0);

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef enum lval_type {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN,
} lval_type;

typedef lval* lbuiltin(lenv*, lval*);

typedef struct lval {
    lval_type type;
    long num;
    char *err;
    char *sym;
    lbuiltin *func;
    int count;
    struct lval **cell;
} lval;

struct lenv {
    int count;
    char **syms;
    lval **vals;
};

void lval_print(lval *v);

void lval_expr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; ++i) {
        lval_print(v->cell[i]);
        if (i != v->count - 1) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(lval *v) {
    switch(v->type) {
        case LVAL_NUM:
            printf("%li", v->num); break;

        case LVAL_ERR:
            printf("%s", v->err);
            break;

        case LVAL_SYM:
            printf("%s", v->sym);
            break;

        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;

        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        case LVAL_FUN:
            printf("<function>");
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

char *ltype_name(lval_type t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Err";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expresion";
        case LVAL_QEXPR: return "Q-Expresion";
        default: return "Unknown";
    }
}

lenv *lenv_new(void) {
    lenv  *e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

lval *lval_num(long x) {
    lval *v = (lval*) malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval *lval_err(char *fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);

    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);

    return v;
}

lval *lval_sym(char *s) {
    lval *v = (lval*) malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval *lval_sexpr(void) {
    lval *v = (lval*) malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_qexpr(void) {
    lval *v = (lval*) malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_func(lbuiltin func) {
    lval *v = (lval*) malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->func = func;
    return v;
}

lval *lval_add(lval *v, lval *x) {
    if (v == NULL) return x;
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

void lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; ++i) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
        case LVAL_FUN: break;
    }
    free(v);
}

void lenv_del(lenv *e) {
    for (int i = 0; i < e->count; ++i) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

lval *lval_read_num(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_read(mpc_ast_t *t) {
    if (strstr(t->tag, "number")) return lval_read_num(t);
    if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

    lval *x = NULL;

    if (strstr(t->tag, ">")) x = lval_sexpr();
    if (strstr(t->tag, "sexpr")) x = lval_sexpr();
    if (strstr(t->tag, "qexpr")) x = lval_qexpr();

    for (int i = 0; i < t->children_num; ++i) {
        if (strcmp(t->children[i]->contents, "(") == 0) continue;
        if (strcmp(t->children[i]->contents, ")") == 0) continue;
        if (strcmp(t->children[i]->contents, "{") == 0) continue;
        if (strcmp(t->children[i]->contents, "}") == 0) continue;
        if (strcmp(t->children[i]->tag, "regex") == 0) continue;
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

lval *lval_take(lval *v, int i);
lval *lval_eval(lenv *e, lval *v);
lval *lval_pop(lval *v, int i);
lval *lenv_get(lenv *e, lval *k);
lval *builtin(lenv *e, lval *a, char *func);

lval *lval_eval_sexpr(lenv *e, lval *v) {
    for (int i = 0; i < v->count; ++i) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    for (int i = 0; i < v->count; ++i) {
        if (v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
    }

    if (v->count == 0) return v;

    if (v->count == 1) return lval_take(v, 0);

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression does not start with function");
    }

    lval *result = f->func(e, v);
    lval_del(f);
    return result;
}

lval *lval_eval(lenv *e, lval *v) {
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) return lval_eval_sexpr(e, v);
    return v;
}

lval *lval_pop(lval *v, int i) {
    lval *x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - i - 1));

    v->count--;

    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

lval *lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval *lval_copy(lval *v) {
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch(v->type) {
        case LVAL_FUN: x->func = v->func; break;
        case LVAL_NUM: x->num = v->num; break;

        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; ++i) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }
    return x;
}

lval *lenv_get(lenv *e, lval *k) {
    assert(k->type == LVAL_SYM);

    for (int i = 0; i < e->count; ++i) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    return lval_err("Unbound symbol '%s'!", k->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {
    assert(k->type == LVAL_SYM);

    for (int i = 0; i < e->count; ++i) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

lval *builtin_head(lenv *e, lval *a) {
    LASSERT(a, a->count == 1,
            "Function 'head' passed too many arguments. "
            "Got %i, Expected %i.",
            a->count, 1);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head' passed incorrect type for argument 0. "
            "Got %s, Expected %s.",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    LASSERT(a, a->cell[0]->count != 0, "Function '%s' passed {}!", "head");

    lval *v = lval_take(a, 0);

    while (v->count > 1) lval_del(lval_pop(v, 1));

    return v;
}

lval *builtin_tail(lenv *e, lval *a) {
    LASSERT(a, a->count == 1,
            "Function 'tail' passed too many arguments. "
            "Got %i, Expected %i.",
            a->count, 1);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'tail' passed argument of incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    LASSERT(a, a->cell[0]->count != 0, "Function '%s' passed {}!", "tail");

    lval *v = lval_take(a, 0);

    lval_del(lval_pop(v, 0));

    return v;
}

lval *builtin_list(lenv *e, lval *a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a) {
    LASSERT(a, a->count == 1,
            "Function 'eval' passed too many arguments. "
            "Got %i, Expected %i.",
            a->count, 1);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed argument of wrong type. "
            "Got %s, Expected %s.",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval *lval_join(lval *x, lval *y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

lval *builtin_join(lenv *e, lval *a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                "Function 'join' passed argument of incorrect type. "
                "Got %s, Expected %s.",
                ltype_name(a->cell[i]->type), ltype_name(LVAL_QEXPR));
    }

    lval *x = lval_qexpr();

    while(a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval *builtin_op(lenv *e, lval *a, char *op) {
    for (int i = 0; i < a->count; ++i) {
        LASSERT(a, a->cell[i]->type == LVAL_NUM,
                "Function '%s' passed invalid type for argument %i. "
                "Got %s, Expected %s.",
                op, i, ltype_name(a->cell[i]->type), ltype_name(LVAL_NUM));
    }

    lval *x = lval_pop(a, 0);

    if (strcmp(op, "-") == 0 && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        lval *y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) x->num += y->num;
        if (strcmp(op, "-") == 0) x->num -= y->num;
        if (strcmp(op, "*") == 0) x->num *= y->num;
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by zero!");
                break;
            }
            x->num /= y->num;
        }
        lval_del(y);
    }
    lval_del(a);
    return x;
}

lval *builtin_add(lenv *e, lval *a) {
    return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a) {
    return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a) {
    return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a) {
    return builtin_op(e, a, "/");
}

lval *builtin_def(lenv *e, lval *a) {
    assert(a->type == LVAL_SEXPR);
    assert(a->count > 0);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'def' passed incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

    lval *syms = a->cell[0];

    for (int i = 0; i < syms->count; ++i) {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "Function 'def' passed invalid type for argument %i. "
                "Got %s, Expected %s.",
                i, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(a, syms->count == a->count - 1,
            "Incorrect no. of arguments to function 'def'. "
            "Symbols: %i, Values: %i.",
            syms->count, a->count - 1);

    for (int i = 0; i < syms->count; ++i) {
        lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpr();
}

lval *builtin(lenv *e, lval *a, char *func) {
    if (strcmp("list", func) == 0) return builtin_list(e, a);
    if (strcmp("head", func) == 0) return builtin_head(e, a);
    if (strcmp("tail", func) == 0) return builtin_tail(e, a);
    if (strcmp("eval", func) == 0) return builtin_eval(e, a);
    if (strcmp("join", func) == 0) return builtin_join(e, a);
    if (strstr("+-*/", func)) return builtin_op(e, a, func);
    lval_del(a);
    return lval_err("Unknown function!");
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *k = lval_sym(name);
    lval *v = lval_func(func);

    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv *e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "def", builtin_def);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}

int main(int argc, char **argv) {
    printf("Mylisp version 0.0.0\n");
    printf("Press Ctrl-c to Exit\n");

    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
      " \
       number   : /-?[0-9]+/; \
       symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
       sexpr    : '(' <expr>* ')' ; \
       qexpr    : '{' <expr>* '}' ; \
       expr     : <number> | <symbol> | <sexpr> | <qexpr> ; \
       lispy    : /^/ <expr>* /$/ ; \
      ", Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    lenv *e = lenv_new();
    lenv_add_builtins(e);

    while (1) {
        char *input = readline("mylisp> ");
        if (!input) {
            printf("\n");
            break;
        }
        if (*input) add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval *x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    lenv_del(e);

    return 0;
}
