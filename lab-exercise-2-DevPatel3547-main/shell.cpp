#include <unistd.h>//used for fork, pipr, dup2, execvp, dup, close 
#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>

using namespace std;

int main() {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};// represents shell  cmd ls -al/
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};//represents shell cmd tr a-z A-Z
    //each char pointer is an array of strings

    // save original stdin and stdout
    //done in case the file descriptors are changed later in the code and need to be restored
    int original_stdin = dup(STDIN_FILENO);
    int original_stdout = dup(STDOUT_FILENO);

    int pipe_fds[2];//creates two file descritors; one for reading and one for writing
    // 0 being read and 1 being write

    if (pipe(pipe_fds) == -1) {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }



    int pid = fork();//creating first process
    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process 1

        // Redirect output to write end of pipe
        dup2(pipe_fds[1], STDOUT_FILENO);
        // Close the read end of the pipe on the child side
        close(pipe_fds[0]);

        // Execute the first command
        execvp(cmd1[0], cmd1);

        // If execvp returns, it means an error occurred
        perror("Error executing command 1");
        exit(EXIT_FAILURE);
    }

    pid = fork();// creating  second child process
    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process 2

        // Redirect input to the read end of the pipe
        dup2(pipe_fds[0], STDIN_FILENO);

        // Close the write end of the pipe on the child side
        close(pipe_fds[1]);

        // Execute the second command
        execvp(cmd2[0], cmd2);

        // If execvp returns, it means an error occurred
        perror("Error executing command 2");
        exit(EXIT_FAILURE);
    }

    // Parent process

    // Close both ends of the pipe
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    // Wait for both child processes to finish
    wait(nullptr);
    wait(nullptr);

    // Reset the input and output file descriptors of the parent
    //Not required in this but a good practice
    dup2(original_stdin, STDIN_FILENO);
    dup2(original_stdout, STDOUT_FILENO);

    return 0;
}
