#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 80 

char **parse_input(char *input) {
    char **args = malloc(MAX_LINE * sizeof(char *));
    char *token = strtok(input, " \n");
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL;
    return args;
}

int main(void) {
    char input[MAX_LINE];
    char **args;
    int should_run = 1;

    while (should_run) {
        printf("uinxsh> ");
        fflush(stdout);
        if (fgets(input, MAX_LINE, stdin) == NULL) break;
        
        args = parse_input(input);
        
        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            should_run = 0;
        }
        
        free(args);
    }
    return 0;
}
