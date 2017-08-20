#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv) {
    printf("Mylisp version 0.0.0\n");
    printf("Press Ctrl-c to Exit\n");

    while (1) {
        char *input = readline("mylisp> ");
        if (!input) {
            printf("\n");
            break;
        }
        if (*input) add_history(input);
        printf("You entered: %s\n", input);
        free(input);
    }

    return 0;
}
