/*Name : Dev RahulBhai Patel
    UIN: 931007975
    Date: 03/02/2023
*/


#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Tokenizer.h"


// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

string findpath() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return string(buffer);
    } else {
        return string("");
    }
}

int main () {
    int invar = dup(0);
    int outvar = dup(1);

    vector<string> cd;
    vector<pid_t> pids;

    for (;;) {
        dup2(invar, 0);
        dup2(outvar, 1);


        
        // need date/time, username, and absolute path to current dir
        time_t current_time = time(0);
        tm* local_time = localtime(&current_time);

        string month_name = "";
        string month_names[] = {"Jan", "Feb", "Mar","Apr","May", "Jun", "Jul","Aug","Sep","Oct","Nov","Dec"};

        // get the current day of the month, hour, minute, and second
        int day_of_month = local_time->tm_mday;
        int hour = local_time->tm_hour;
        int minute = local_time->tm_min;
        int second = local_time->tm_sec;

        // get the username of the current user and the current path
        string user_name = getenv("USER");
        string current_path = findpath();

        // print the default prompt with the current time, username, and path
        cout << YELLOW << month_names[local_time->tm_mon] << " "  << day_of_month << " " << hour << ":" << minute << ":" << second << " "  << user_name << ":" << current_path << "$" << NC << " ";  
            
        // get user inputted command

        
        int pid_count = pids.size();
        for (int i = 0; i < pid_count; i++) {
            int wait_status = 0;

            if (waitpid(pids[i], &wait_status, WNOHANG) == pids[i]) {
                pids[i] = pids[pid_count-1];
                pids.pop_back();
                pid_count--;
            }
        }

        // Read user input from the command line
        string input;
        getline(cin, input);

        // If the user enters "exit", exit the shell
        if (input == "exit") {
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }


        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
        // for (auto cmd : tknr.commands) {
        //     for (auto str : cmd->args) {
        //         cerr << "|" << str << "| ";
        //     }
        //     if (cmd->hasInput()) {
        //         cerr << "in< " << cmd->in_file << " ";
        //     }
        //     if (cmd->hasOutput()) {
        //         cerr << "out> " << cmd->out_file << " ";
        //     }
        //     cerr << endl;
        // }



        // Get the number of commands
        int num = tknr.commands.size();

        // If the first command is a "cd" command, handle it
        if (num > 0 && tknr.commands[0]->args[0] == "cd") {
            string path = ".";
            size_t num_args = tknr.commands[0]->args.size();

            // If there are no arguments, change to the home directory
            if (num_args == 1) {
                path = "/home/";
            }

            // If there is one argument, handle it
            else if (num_args == 2) {
                string arg = tknr.commands[0]->args[1];

                // If the argument is "-", change to the previous directory in the history
                if (arg == "-") {
                    if (!cd.empty()) {
                        path = cd.back();
                        cd.pop_back();
                    } else {
                        path = ".";
                    }
                }

                // Otherwise, change to the specified directory and add the current directory to the history
                else {
                    path = arg;
                    cd.push_back(findpath());
                }
            }

            // Change the current directory
            chdir(path.c_str());
        }


        else {// if child, exec to run command
        // run single commands with no arguments
        // Loop through each command in the tokenized input
                for (int commandIndex = 0; commandIndex < num; commandIndex++) {
                        // Create a pipe to connect the current command to the next one
                        int pipefd[2];
                        if (pipe(pipefd) < 0) {
                                perror("pipe");
                                exit(2);
                        }
                        // Convert the command's arguments from a vector of strings to a char array
                        char** commandArgs = new char*[tknr.commands[commandIndex]->args.size() + 1];
                        int argsSize = tknr.commands[commandIndex]->args.size();
                        for (int i = 0; i < argsSize; i++) {
                                char* arg = const_cast<char *>(tknr.commands[commandIndex]->args[i].c_str());
                                commandArgs[i] = arg;
                        }
                        commandArgs[tknr.commands[commandIndex]->args.size()] = NULL;
                        // Create a new process to execute the command
                        pid_t childPid = fork();


                        if (childPid < 0) {
                                perror("fork");
                                exit(2);
                        }


                        // If this is the child process, set up the input/output redirection and execute the command



                        if (childPid == 0) {
                                if ((!tknr.commands[commandIndex]->hasInput()) && (tknr.commands[commandIndex]->hasOutput())) {


                                // Redirect output to a file
                                        int fd = open(tknr.commands[commandIndex]->out_file.c_str(), O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                                        dup2(fd, 1);
                                        close(fd);


                                }
                                else if ((tknr.commands[commandIndex]->hasInput()) && (tknr.commands[commandIndex]->hasOutput())) {
                                // Redirect both input and output
                                        int inputFd, outputFd;
                                        inputFd = open(tknr.commands[commandIndex]->in_file.c_str(), O_RDONLY);
                                        outputFd = open(tknr.commands[commandIndex]->out_file.c_str(), O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                                        dup2(inputFd, 0);
                                        dup2(outputFd, 1);
                                        close(inputFd);
                                        close(outputFd);
                                }
                                else if ((tknr.commands[commandIndex]->hasInput()) && (!tknr.commands[commandIndex]->hasOutput())) {
                                // Redirect input from a file
                                        int fd = open(tknr.commands[commandIndex]->in_file.c_str(), O_RDONLY,S_IRUSR|S_IRGRP|S_IROTH);
                                        dup2(fd, 0);
                                        close(fd);
                                }
                                // If there is a next command, redirect output to the pipe
                                if (commandIndex < (num - 1)) {

                                        dup2(pipefd[1], 1);
                                        close(pipefd[0]);

                                }
                                // Execute the command
                                if (execvp(commandArgs[0], commandArgs) < 0) {

                                        perror("execvp");
                                        exit(2);
                                        
                                }
                        }

                        else {// if parent, wait for child to finish
                            if ((commandIndex == (num - 1)) && (!tknr.commands[commandIndex]->isBackground())) {
                                int status = 0;
                                waitpid(childPid, &status, 0);
                                if (status > 1) {  // exit if child didn't exec properly
                                    exit(status);
                                }
                            }
                            if (tknr.commands[commandIndex]->isBackground()) {
                                pids.push_back(childPid);   
                            }
                            dup2(pipefd[0], 0);
                            close(pipefd[1]);
                        }
                delete[] commandArgs;
            }
            dup2(invar, 0);
            dup2(outvar, 1);
        }

    }
}

