#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "mpc/mpc.h"

typedef struct {
    int type;
    long num;
    int err;
} lval;

enum {
    LVAL_NUM,
    LVAL_ERR
};

enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

void lval_print(lval v) {
    switch(v.type) {
        case LVAL_NUM:
            printf("%li", v.num); break;

        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division by zero!");
            } else if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid operator!");
            } else if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid number!");
            }
            break;
    }
}

void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

lval eval_unary_op(char *op, lval x) {
    if (x.type == LVAL_ERR) return x;

    if (strcmp(op, "+") == 0) return x;
    if (strcmp(op, "-") == 0) return lval_num(-x.num);
    if (strcmp(op, "*") == 0) return x;
    if (strcmp(op, "/") == 0) return x.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(1 / x.num);
    return lval_err(LERR_BAD_OP);
}

lval eval_op(lval x, char *op, lval y) {
    if (x.type == LVAL_ERR) return x;
    if (y.type == LVAL_ERR) return y;

    if (strcmp(op, "+") == 0) return lval_num(x.num + y.num);
    if (strcmp(op, "-") == 0) return lval_num(x.num - y.num);
    if (strcmp(op, "*") == 0) return lval_num(x.num * y.num);
    if (strcmp(op, "/") == 0) {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t) {
    // If tagged as number return directly
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = atoi(t->contents);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char *op = t->children[1]->contents;

     lval x = eval(t->children[2]);

    if (t->children_num == 4) return eval_unary_op(op, x);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char **argv) {
    printf("Mylisp version 0.0.0\n");
    printf("Press Ctrl-c to Exit\n");

    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
      " \
       number   : /-?[0-9]+/; \
       operator : '+' | '-' | '/' | '*'; \
       expr     : <number> | '(' <operator> <expr>+ ')'; \
       lispy    : /^/ <operator> <expr>+ /$/ | /^\\s*$/ ; \
      ", Number, Operator, Expr, Lispy);

    while (1) {
        char *input = readline("mylisp> ");
        if (!input) {
            printf("\n");
            break;
        }
        if (*input) add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval_println(eval(r.output));
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
