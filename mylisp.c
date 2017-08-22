#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "mpc/mpc.h"

long eval_unary_op(char *op, long x) {
    if (strcmp(op, "+") == 0) return x;
    if (strcmp(op, "-") == 0) return -x;
    if (strcmp(op, "*") == 0) return x;
    if (strcmp(op, "/") == 0) return 1 / x;
    return 0;
}

long eval_op(long x, char *op, long y) {
    if (strcmp(op, "+") == 0) return x + y;
    if (strcmp(op, "-") == 0) return x - y;
    if (strcmp(op, "*") == 0) return x * y;
    if (strcmp(op, "/") == 0) return x / y;
    return 0;
}

long eval(mpc_ast_t *t) {
    // If tagged as number return directly
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    char *op = t->children[1]->contents;

    long x = eval(t->children[2]);

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
            printf("%ld\n", eval(r.output));
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
