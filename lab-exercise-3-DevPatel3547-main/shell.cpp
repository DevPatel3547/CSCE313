#include <unistd.h>//used for fork, pipr, dup2, execvp, dup, close 
#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>

using namespace std;

int main() {
    char input[1024];//holds user input

    while (true) {
        cout << "Enter a command: ";
        cin.getline(input, 1024);//reads in user input

        if (strcmp(input, "exit") == 0) {
            break;//exits if the user types "exit"
        }

        char* commands[10][10];//array of commands and arguments
        char* cmd = strtok(input, "|");//split the input into individual commands separated by "|"

        int num_commands = 0;//number of commands

        while (cmd != nullptr) {//parse through the commands
            char* arg = strtok(cmd, " ");//split the command and arguments
            int i = 0;

            while (arg != nullptr) {//parse through the arguments
                commands[num_commands][i++] = arg;
                arg = strtok(nullptr, " ");
            }

            commands[num_commands][i] = nullptr;//end of arguments
            num_commands++;//increase the number of commands
            cmd = strtok(nullptr, "|");//move on to the next command
        }

        int original_stdin = dup(STDIN_FILENO);//save original stdin and stdout
        int original_stdout = dup(STDOUT_FILENO);

        int prev_pipe_fds[2] = {-1, -1};//initialize previous pipe fds to -1

        for (int i = 0; i < num_commands; i++) {//loop through each command

            int curr_pipe_fds[2];//current pipe fds

            if (pipe(curr_pipe_fds) == -1) {//create a new pipe
                perror("Error creating pipe");
                exit(EXIT_FAILURE);
            }

            int pid = fork();//fork a new process

            if (pid == -1) {//check for errors
                perror("Error forking process");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {//child process

                if (prev_pipe_fds[0] != -1) {//check if there is a previous pipe
                    dup2(prev_pipe_fds[0], STDIN_FILENO);//redirect input to the read end of the previous pipe
                    close(prev_pipe_fds[1]);//close the write end of the previous pipe
                }

                if (i != num_commands - 1) {//check if there is a next command
                    dup2(curr_pipe_fds[1], STDOUT_FILENO);//redirect output to the write end of the current pipe
                    close(curr_pipe_fds[0]);//close the read end of the current pipe
                }

                execvp(commands[i][0], commands[i]);//execute the command

                // If execvp returns, it means an error occurred
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }

            if (prev_pipe_fds[0] != -1) {//close the previous pipe if it exists
                close(prev_pipe_fds[0]);
                close(prev_pipe_fds[1]);
            }

            prev_pipe_fds[0] = curr_pipe_fds[0];//set the current pipe fds to the previous pipe fds
            prev_pipe_fds[1] = curr_pipe_fds[1];
        }

        //wait for the last child process to finish
        wait(nullptr);

        //reset the input and output file descriptors of the parent
        dup2(original_stdin, STDIN_FILENO);
        dup2(original_stdout, STDOUT_FILENO);
    }

    return 0;
}