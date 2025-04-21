#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h> // Include this header for errno and EINTR

#define MAX_ARGS 512
#define MAX_LINE 2049
#define MAX_BG_PROCESSES 100

// Function prototypes
void handle_SIGTSTP(int signo);
void handle_SIGINT(int signo);
void expand_pid(char* line);
void kill_bg_processes();
void print_status();
void check_bg_processes();

// Global variables
int foreground_only_mode = 0;
pid_t bg_pids[MAX_BG_PROCESSES];
int bg_count = 0;
int status = 0;
pid_t foreground_pid = -1; // Track the foreground process PID

int main() {
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    char* args[MAX_ARGS];
    int background = 0;
    char* inputFile = NULL;
    char* outputFile = NULL;

    // Set up the SIGTSTP signal handler
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART; // Restart interrupted system calls
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // Set up the SIGINT signal handler
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = handle_SIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    while (1) {
        check_bg_processes(); // Check the status of background processes

        printf(": ");
        fflush(stdout);

        // Get line from user
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            if (errno == EINTR) {
                clearerr(stdin); // Clear the error
                continue; // Restart the loop
            } else {
                perror("getline");
                exit(1);
            }
        }

        // Check if line is empty or a comment
        if (line[0] == '\n' || line[0] == '#') {
            continue;
        }

        // Remove newline character
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        // Expand $$ to PID
        expand_pid(line);

        // Check if the very last characters are " &"
        if (nread > 1 && line[nread - 2] == '&') {
            if (!foreground_only_mode) {
                background = 1;
                line[nread - 2] = '\0';
            } else {
                background = 0;
                line[nread - 2] = '\0'; // Remove the '&' character
            }
        } else {
            background = 0;
        }

        // Use strtok() to tokenize the string
        int i = 0;
        char* token = strtok(line, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // If the first token is "exit", "status", or "cd", run in foreground
        if (strcmp(args[0], "exit") == 0) {
            kill_bg_processes(); // Kill all background processes before exiting
            return 0;
        }

        if (strcmp(args[0], "status") == 0) {
            print_status(); // Print the status of the last foreground process
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                chdir(getenv("HOME")); // Change to home directory if no argument is provided
            } else {
                if (chdir(args[1]) != 0) { // Change to the specified directory
                    perror("chdir");
                }
            }
            continue;
        }

        // Continue tokenizing and checking for redirection
        inputFile = NULL;
        outputFile = NULL;
        int j;
        for (j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "<") == 0) {
                inputFile = args[j + 1]; // Input redirection
                args[j] = NULL;
            } else if (strcmp(args[j], ">") == 0) {
                outputFile = args[j + 1]; // Output redirection
                args[j] = NULL;
            }
        }

        // Use fork() and execvp() to run the command
        pid_t pid = fork(); // Create a new process
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // Child process
            if (inputFile != NULL) {
                int fd = open(inputFile, O_RDONLY); // Open the input file
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO); // Redirect stdin to the input file
                close(fd);
            }
            if (outputFile != NULL) {
                int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open the output file
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO); // Redirect stdout to the output file
                close(fd);
            }
            if (execvp(args[0], args) == -1) { // Replace the current process image with a new process image
                perror("execvp");
                exit(1);
            }
        } else {
            // Parent process
            if (!background || strcmp(args[0], "status") == 0 || strcmp(args[0], "cd") == 0 || strcmp(args[0], "exit") == 0) {
                foreground_pid = pid; // Track the foreground process PID
                waitpid(pid, &status, 0); // Wait for the foreground process to finish
                foreground_pid = -1; // Reset the foreground process PID
                if (WIFSIGNALED(status)) {
                    printf("Terminated by signal %d\n", WTERMSIG(status));
                }
            } else {
                bg_pids[bg_count++] = pid; // Add the background process to the list
                printf("Background PID: %d\n", pid);
            }
        }
    }

    free(line); // Free the allocated memory for the line
    return 0;
}

// Signal handler for SIGTSTP (Ctrl+Z)
void handle_SIGTSTP(int signo) {
    if (foreground_only_mode == 0) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 49);
        foreground_only_mode = 1;
    } else {
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 29);
        foreground_only_mode = 0;
    }
    // Print the prompt again after handling SIGTSTP
    write(STDOUT_FILENO, "\n: ", 3);
}

// Signal handler for SIGINT (Ctrl+C)
void handle_SIGINT(int signo) {
    if (foreground_pid != -1) {
        kill(foreground_pid, SIGINT); // Forward SIGINT to the foreground process
    }
    // Print the prompt again after handling SIGINT
    write(STDOUT_FILENO, "\n: ", 3);
}

// Function to expand $$ to the PID of the shell
void expand_pid(char* line) {
    char expanded_line[MAX_LINE];
    char pid_str[10];
    snprintf(pid_str, 10, "%d", getpid()); // Get the PID of the shell
    char* pos = line;
    char* next_pos;
    expanded_line[0] = '\0';

    while ((next_pos = strstr(pos, "$$")) != NULL) {
        strncat(expanded_line, pos, next_pos - pos); // Copy the part before $$
        strcat(expanded_line, pid_str); // Replace $$ with the PID
        pos = next_pos + 2;
    }
    strcat(expanded_line, pos); // Copy the remaining part of the line
    strcpy(line, expanded_line); // Update the original line with the expanded line
}

// Function to kill all background processes
void kill_bg_processes() {
    int i;
    for (i = 0; i < bg_count; i++) {
        kill(bg_pids[i], SIGKILL); // Send SIGKILL to each background process
    }
}

// Function to print the status of the last foreground process
void print_status() {
    if (WIFEXITED(status)) {
        printf("Exit status: %d\n", WEXITSTATUS(status)); // Print the exit status
    } else if (WIFSIGNALED(status)) {
        printf("Terminated by signal: %d\n", WTERMSIG(status)); // Print the terminating signal
    }
}

// Function to check and print the status of background processes
void check_bg_processes() {
    int bg_status;
    pid_t bg_pid;
    while ((bg_pid = waitpid(-1, &bg_status, WNOHANG)) > 0) { // Non-blocking wait for background processes
        printf("Background PID %d is done: ", bg_pid);
        if (WIFEXITED(bg_status)) {
            printf("exit value %d\n", WEXITSTATUS(bg_status)); // Print the exit status
        } else if (WIFSIGNALED(bg_status)) {
            printf("terminated by signal %d\n", WTERMSIG(bg_status)); // Print the terminating signal
        }
    }
}
