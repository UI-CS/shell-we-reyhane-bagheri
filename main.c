#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <readline/readline.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define SIZE 9
#define SUBGRID_SIZE 3

/* = SUDOKU VALIDATOR = */
typedef struct {
    int row;
    int column;
} sudoku_params;

int board[SIZE][SIZE];
int valid[11];

void *validate_rows(void *arg) {
    for (int row = 0; row < SIZE; row++) {
        bool seen[10] = {false};
        for (int col = 0; col < SIZE; col++) {
            int num = board[row][col];
            if (num < 1 || num > 9 || seen[num]) {
                valid[0] = 0;
                pthread_exit(NULL);
            }
            seen[num] = true;
        }
    }
    valid[0] = 1;
    pthread_exit(NULL);
}
void *validate_cols(void *arg) {
    for (int col = 0; col < SIZE; col++) {
        bool seen[10] = {false};
        for (int row = 0; row < SIZE; row++) {
            int num = board[row][col];
            if (num < 1 || num > 9 || seen[num]) {
                valid[1] = 0;
                pthread_exit(NULL);
            }
            seen[num] = true;
        }
    }
    valid[1] = 1;
    pthread_exit(NULL);
}

void *validate_subgrid(void *arg) {
    sudoku_params *p = (sudoku_params *)arg;
    bool seen[10] = {false};
    
    for (int row = p->row; row < p->row + SUBGRID_SIZE; row++) {
        for (int col = p->column; col < p->column + SUBGRID_SIZE; col++) {
            int num = board[row][col];
            if (num < 1 || num > 9 || seen[num]) {
                int subgrid_index = (p->row / SUBGRID_SIZE) * SUBGRID_SIZE + (p->column / SUBGRID_SIZE);
                valid[2 + subgrid_index] = 0;
                pthread_exit(NULL);
            }
            seen[num] = true;
        }
    }
        
    int subgrid_index = (p->row / SUBGRID_SIZE) * SUBGRID_SIZE + (p->column / SUBGRID_SIZE);
    valid[2 + subgrid_index] = 1;
    pthread_exit(NULL);
}

int run_sudoku(void) {
    for (int i = 0; i < 11; i++) {
        valid[i] = 1;
    }

    int sample_board[SIZE][SIZE] = {
        {5, 3, 4, 6, 7, 8, 9, 1, 2},
        {6, 7, 2, 1, 9, 5, 3, 4, 8},
        {1, 9, 8, 3, 4, 2, 5, 6, 7},
        {8, 5, 9, 7, 6, 1, 4, 2, 3},
        {4, 2, 6, 8, 5, 3, 7, 9, 1},
        {7, 1, 3, 9, 2, 4, 8, 5, 6},
        {9, 6, 1, 5, 3, 7, 2, 8, 4},
        {2, 8, 7, 4, 1, 9, 6, 3, 5},
        {3, 4, 5, 2, 8, 6, 1, 7, 9}
    };

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = sample_board[i][j];
        }
    }

    pthread_t threads[11];

    pthread_create(&threads[0], NULL, validate_rows, NULL);
    pthread_create(&threads[1], NULL, validate_cols, NULL);

    for (int i = 0; i < 9; i++) {
        sudoku_params *p = malloc(sizeof(sudoku_params));
        p->row = (i / SUBGRID_SIZE) * SUBGRID_SIZE;
        p->column = (i % SUBGRID_SIZE) * SUBGRID_SIZE;
        pthread_create(&threads[2 + i], NULL, validate_subgrid, (void *)p);
    }

    for (int i = 0; i < 11; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < 11; i++) {
        if (valid[i] == 0) {
            printf("Sudoku is Invalid\n");
            return 0;
        }
    }

    printf("Sudoku is Valid\n");
    return 0;
}
/* = PI CALCULATOR = */

typedef struct {
    long points_per_worker;
    int worker_id;
} pi_params;

long points_inside = 0;
pthread_mutex_t pi_mutex = PTHREAD_MUTEX_INITIALIZER;

void *monte_carlo_worker(void *arg) {
    pi_params *params = (pi_params *)arg;
    long local_inside = 0;
    
    unsigned int seed = (unsigned int)(time(NULL) ^ (params->worker_id << 16));
    
    for (long i = 0; i < params->points_per_worker; i++) {
        double x = (2.0 * rand_r(&seed) / RAND_MAX) - 1.0;
        double y = (2.0 * rand_r(&seed) / RAND_MAX) - 1.0;
        
        if (x * x + y * y <= 1.0) {
            local_inside++;
        }
    }
    
    pthread_mutex_lock(&pi_mutex);
    points_inside += local_inside;
    pthread_mutex_unlock(&pi_mutex);
    
    free(params);
    pthread_exit(NULL);
}

int run_pi(int num_threads, long total_points) {
    if (num_threads <= 0 || total_points <= 0) {
        fprintf(stderr, "Error: num_threads and total_points must be positive\n");
        return 1;
    }
    
    points_inside = 0;
    
    time_t start_time = time(NULL);
    clock_t start_clock = clock();
    
    pthread_t threads[num_threads];
    
    long points_per_worker = total_points / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        pi_params *params = malloc(sizeof(pi_params));
        if (!params) {
            fprintf(stderr, "Error: malloc failed\n");
            return 1;
        }
        params->points_per_worker = points_per_worker;
        params->worker_id = i;
        
        if (pthread_create(&threads[i], NULL, monte_carlo_worker, (void *)params) != 0) {
            fprintf(stderr, "Error: pthread_create failed\n");
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error: pthread_join failed\n");
            return 1;
        }
    }
    
    clock_t end_clock = clock();
    time_t end_time = time(NULL);
    
    double pi_estimate = 4.0 * points_inside / total_points;
    double elapsed_time = (double)(end_clock - start_clock) / CLOCKS_PER_SEC;
    
    printf("Threads: %d\n", num_threads);
    printf("Total Points: %ld\n", total_points);
    printf("Points Inside Circle: %ld\n", points_inside);
    printf("Estimated Pi: %.6f\n", pi_estimate);
    printf("Actual Pi:    %.6f\n", M_PI);
    printf("Error:        %.6f\n", fabs(pi_estimate - M_PI));
    printf("Time Elapsed: %.3f seconds\n", elapsed_time);
    
    return 0;
}

/* = UNIX SHELL = */
static char last_command[MAX_LINE] = "";

static int is_builtin(char **args) {
    if (!args || !args[0]) return 0;
    return strcmp(args[0], "cd") == 0 || strcmp(args[0], "pwd") == 0 || strcmp(args[0], "help") == 0 || strcmp(args[0], "exit") == 0 || strcmp(args[0], "echo") == 0 || strcmp(args[0], "clr") == 0;
}

static void handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        char *path = args[1] ? args[1] : getenv("HOME");
        if (chdir(path) != 0) {
            perror("cd failed");
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd failed");
        }
    } else if (strcmp(args[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd [path]  - Change directory\n");
        printf("  pwd        - Print working directory\n");
        printf("  help       - Show this help message\n");
        printf("  exit       - Exit the shell\n");
        printf("  echo ...   - Echo arguments\n");
        printf("  clr        - Clear the screen\n");
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i]; i++) {
            printf("%s%s", args[i], args[i + 1] ? " " : "");
        }
        printf("\n");
    } else if (strcmp(args[0], "clr") == 0) {
        printf("\033[H\033[J");
    }
}

static void parse_input(char *input, char **args, int *is_background) {
    *is_background = 0;
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        *is_background = 1;
        args[i - 1] = NULL;
    }
}

static void execute_command(char **args, int is_background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        perror("Execution failed");
        _exit(1);
    }

    if (is_background) {
        printf("[%d]\n", pid);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

static void execute_piped_command(char **args_input, char **args_output, int is_background) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args_input[0], args_input);
        perror("Execution failed");
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args_output[0], args_output);
        perror("Execution failed");
        _exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if (is_background) {
        printf("[%d|%d]\n", pid1, pid2);
        return;
    }

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
}

int run_shell(void) {
    char *args[MAX_ARGS];
    char *args_pipe[MAX_ARGS];

    while (1) {
        while (waitpid(-1, NULL, WNOHANG) > 0) {}

        char *input = readline("unixsh> ");
        if (!input) break;

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        if (strcmp(input, "!!") == 0) {
            if (strlen(last_command) == 0) {
                printf("No commands in history.\n");
                free(input);
                continue;
            }
            free(input);
            input = strdup(last_command);
            if (!input) {
                perror("history alloc failed");
                continue;
            }
            printf("%s\n", input);
        } else {
            strncpy(last_command, input, sizeof(last_command) - 1);
            last_command[sizeof(last_command) - 1] = '\0';
        }

        int is_background = 0;
        int has_pipe = 0;
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            has_pipe = 1;
            *pipe_pos = '\0';
            char *right = pipe_pos + 1;

            parse_input(input, args, &is_background);
            parse_input(right, args_pipe, &is_background);
        } else {
            parse_input(input, args, &is_background);
        }

        if (!has_pipe && (!args[0])) {
            free(input);
            continue;
        }

        if (!has_pipe && strcmp(args[0], "exit") == 0) {
            free(input);
            break;
        }

        if (has_pipe) {
            execute_piped_command(args, args_pipe, is_background);
        } else if (is_builtin(args)) {
            handle_builtin(args);
        } else {
            execute_command(args, is_background);
        }

        free(input);
    }

    return 0;
}

/* = DISPATCHER = */
int main(int argc, char *argv[]) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "shell") == 0)) {
        return run_shell();
    } else if (argc == 2 && strcmp(argv[1], "sudoku") == 0) {
        return run_sudoku();
    } else if (argc >= 3 && strcmp(argv[1], "pi") == 0) {
        int num_threads = atoi(argv[2]);
        long total_points = atol(argv[3]);
        return run_pi(num_threads, total_points);
    } else {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s [shell]           - Run Unix shell (default)\n", argv[0]);
        fprintf(stderr, "  %s sudoku            - Run Sudoku validator\n", argv[0]);
        fprintf(stderr, "  %s pi <threads> <points> - Run Pi calculator\n", argv[0]);
        return 1;
    }
}
