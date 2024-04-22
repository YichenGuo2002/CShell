#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

pid_t suspendedJobIdList[100];
char *suspendedJobList[100];
int suspendedJobIndex = 0;

/*写了一个空的signal handler*/
// I learned how to write a signal handler from module 2 slides
void handler() {
}

/*一个用来检测PId是否存在于suspendedJobIdList里的程序*/
// I learned how to check if a string exist in an array from https://stackoverflow.com/questions/13677890/how-to-check-if-a-string-is-in-an-array-of-strings-in-c
bool isElement(pid_t element){
    int i = 0;
    while(suspendedJobIdList[i] && i < suspendedJobIndex)
    {
        if(suspendedJobIdList[i] == element)
        {
            return true;
        }
        i++;
    }
    return false;
}

/*一个用来把某个job从suspendedJob里删除的程序，包括删除PId和删除command*/
void deleteJob(int index){
    int i = index;
    while(suspendedJobIdList[i] && suspendedJobIdList[i + 1] && i + 1 <= suspendedJobIndex){
        suspendedJobIdList[i] = suspendedJobIdList[i + 1];
        i++;
    }
    i = index;
    while(suspendedJobList[i] && suspendedJobList[i + 1] && i + 1 <= suspendedJobIndex){
        suspendedJobList[i] = strdup(suspendedJobList[i + 1]);
        i++;
    }
    suspendedJobList[i] = NULL;
}

/*分解command的程序，upstream代表是否有上源stdin，destination代表是否有下源stdout*/
void parser(char *command, char *upstream, char *destination){
    // I learned how to dynamically copy a string using strdup() from https://www.geeksforgeeks.org/strdup-strdndup-functions-c/
    char *originalCommand = strdup(command);
    // I learned how to search for first occurrence of character from https://www.tutorialspoint.com/c_standard_library/c_function_strchr.htm
    char *pipeLocation = strchr(command, '|');

    /*如果存在pipe，分开前后的command，用fork()创造两个child，一个write to pipe，一个read from pipe，parent wait()这两个child*/
    if(pipeLocation != NULL){
        // I learned how to null-terminate a string from https://www.tutorialspoint.com/what-is-a-null-terminated-string-in-c-cplusplus#:~:text=The%20null%20terminated%20strings%20are,terminated%20strings%20by%20the%20compiler.
        *pipeLocation = '\0';

        char *beforePipe = command;
        char *afterPipe = pipeLocation + 1;
        // I learned how to skip spaces using pointer from https://stackoverflow.com/questions/1726302/remove-spaces-from-a-string-in-c
        while (*afterPipe == ' ') {
            ++afterPipe;
        }
        if(strlen(beforePipe) == 0 || strlen(afterPipe) == 0){
            fprintf(stderr, "%s\n", "Error: invalid command"); 
            return;
        }

        // I learned how to pipe() from the man pipe page https://man7.org/linux/man-pages/man2/pipe.2.html
        int pipefd[2];
        pid_t child1, child2;

        if (pipe(pipefd) == -1) {
            fprintf(stderr, "%s\n", "Error: pipe failed");
            return;
        }

        child1 = fork();
        if (child1 < 0) {
            fprintf(stderr, "%s\n", "Error: fork failed");
            return;
        }
        else if (child1 == 0){ /*writing child process*/
            close(pipefd[0]); /*close unused read end*/
            dup2(pipefd[1], 1); /*move file descriptor to stdout*/
            close(pipefd[1]); 
            parser(beforePipe, NULL, "pipe");
            // I learned how to exit success and error from https://www.geeksforgeeks.org/exit0-vs-exit1-in-c-c-with-examples/#
            exit(0);
        }else {
            child2 = fork();
            if (child2 < 0) {
                fprintf(stderr, "%s\n", "Error: fork failed");
                return;
            }
            else if (child2 == 0){ /*reading child process*/
                close(pipefd[1]); /*close unused write end*/
                dup2(pipefd[0], 0); /*move file descriptor to stdin*/
                close(pipefd[0]); 
                parser(afterPipe, "pipe", NULL);
                exit(0);
            } else {
                close(pipefd[0]);
                close(pipefd[1]);

                int status1, status2;
                // Am I supposed to also monitor if the child processes stop or not?
                waitpid(child1, &status1, 0);
                waitpid(child2, &status2, 0);
            }
        }
    }
    else{
        /*开始parse command，查找是否有上下源*/
        char *savePointer, *upstreamFile = NULL, *destinationFile = NULL;
        // I learned how to use strtok_r from https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        char *token = strtok_r(command, " ", &savePointer);
        char *arguments[1000];
        int argumentCounter = 0;
        int overwrite = 0;

        while(token != NULL){
            // I learned how to compare strings from https://www.programiz.com/c-programming/library-function/string.h/strcmp
            if(strcmp(token, ">") == 0 || strcmp(token, ">>") == 0){
                if(destination != NULL){
                    fprintf(stderr, "%s\n", "Error: invalid command"); 
                    return;
                }else{
                    if(strcmp(token, ">") == 0){
                        overwrite = 1;
                    }else if(strcmp(token, ">>") == 0){
                        overwrite = 0;
                    }
                    token = strtok_r(NULL, " ", &savePointer);
                    if(!token){
                        fprintf(stderr, "%s\n", "Error: invalid command"); 
                        return;
                    }else{
                        // I can also replace malloc() with strdup()
                        destinationFile = (char *)malloc(strlen(token) + 1);
                        if (destinationFile == NULL) {
                            fprintf(stderr,  "%s\n", "Error: malloc failed");
                            return;
                        }
                        else{
                            // I learned first to set the null pointer for allocated strings from https://stackoverflow.com/questions/18838933/why-do-i-first-have-to-strcpy-before-strcat
                            // I learned the difference between double and single quotes from https://www.tutorialspoint.com/single-quotes-vs-double-quotes-in-c-or-cplusplus#:~:text=In%20C%20and%20C%2B%2B%20the,the%20character%20literal%20is%20char.
                            // I learned how to copy string from https://www.tutorialspoint.com/c_standard_library/c_function_strcat.htm
                            destinationFile[0] = '\0';
                            strcat(destinationFile, token);
                            token = strtok_r(NULL, " ", &savePointer);
                        }
                    }
                }
            }
            else if(strcmp(token, "<") == 0){
                if(upstream != NULL){
                    fprintf(stderr, "%s\n", "Error: invalid command"); 
                    return;
                }else{
                    token = strtok_r(NULL, " ", &savePointer);
                    if(!token){
                        fprintf(stderr, "%s\n", "Error: invalid command"); 
                        return;
                    }else{
                        upstreamFile = (char *)malloc(strlen(token) + 1);
                        if (upstreamFile == NULL) {
                            fprintf(stderr,  "%s\n", "Error: malloc failed");
                            return;
                        }
                        else{
                            upstreamFile[0] = '\0';
                            strcat(upstreamFile, token);
                            token = strtok_r(NULL, " ", &savePointer);
                        }
                    }
                }
            }
            else{
                arguments[argumentCounter] = token;
                argumentCounter++;
                // I learned how to proceed to the next occurrence from https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/#
                token = strtok_r(NULL, " ", &savePointer);
            }
        }
        arguments[argumentCounter] = NULL;
        if(arguments[0] == 0 || argumentCounter == 0){ /*If there is no argument*/
            fprintf(stderr, "%s\n", "Error: invalid command"); 
            return;
        }

        /*查找command是否是built-in command*/
        if(strcmp(arguments[0], "cd") == 0){
            if(argumentCounter != 2){
                fprintf(stderr, "%s\n", "Error: invalid command"); 
                return;
            }
            // I learned chdir() from https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/#
            else if(chdir(arguments[1]) != 0){
                fprintf(stderr, "%s\n", "Error: invalid directory");
                return;
            } 
        }
        else if(strcmp(arguments[0], "jobs") == 0){
            if(argumentCounter != 1){
                fprintf(stderr, "%s\n", "Error: invalid command"); 
                return;
            }
            else{
                for(int i = 0; i < suspendedJobIndex; i++){
                    printf("[%d] %s\n", i + 1, suspendedJobList[i]);
                }
            }
        }
        else if(strcmp(arguments[0], "fg") == 0){
            if(argumentCounter != 2){
                fprintf(stderr, "%s\n", "Error: invalid command"); 
                return;
            }
            else{
                // I learned how to convert string to int from https://www.tutorialspoint.com/c_standard_library/c_function_atoi.htm
                int index = atoi(arguments[1]);
                if(index <= 0 || index > suspendedJobIndex){
                    fprintf(stderr, "%s\n", "Error: invalid job"); 
                    return;
                }
                else{
                    // I learned how to use kill() to send a signal from the man kill page https://man7.org/linux/man-pages/man2/kill.2.html
                    kill(suspendedJobIdList[index - 1], SIGCONT);
                    int childId = suspendedJobIdList[index - 1];
                    char *childCommand = strdup(suspendedJobList[index - 1]);

                    suspendedJobIndex--;
                    deleteJob(index - 1);
                    
                    // I learned to waitpid() after resuming a job by attending Mayank's tutoring session at 11:30 am, 10/17/23
                    int status;
                    pid_t w = waitpid(childId, &status, WUNTRACED);
                    if (w == -1) {
                        fprintf(stderr, "%s\n", "Error: waitpid failed"); 
                    }
                    else if (WIFSTOPPED(status) && !isElement(childId)
                    ) {
                        suspendedJobIndex++;
                        suspendedJobIdList[suspendedJobIndex - 1] = childId;
                        suspendedJobList[suspendedJobIndex - 1] = strdup(childCommand); 
                        suspendedJobList[suspendedJobIndex] = NULL;
                    }
                }
            }
        }
        else if(strcmp(arguments[0], "exit") == 0){
            if(argumentCounter != 1){
                fprintf(stderr, "%s\n", "Error: invalid command"); 
                return;
            }
            else if(suspendedJobIndex != 0){
               fprintf(stderr, "%s\n", "Error: there are suspended jobs");
               return;
            }
            else{
                exit(0);
            }
        }
        else
        {
            /*使用exec()程序运行command*/
            // I learned how to search for substrings from https://stackoverflow.com/questions/15098936/simple-way-to-check-if-a-string-contains-another-string-in-c
            // I learned how to copy a string from https://www.programiz.com/c-programming/library-function/string.h/strcpy
            // I learned how to concat 2 strings from https://www.tutorialspoint.com/c_standard_library/c_function_strcat.htm
            // I learned how to locate files in /usr/bin/ by attending Mayank's tutoring session at 11:30 am, 10/17/23
            char operation[100];
            if(strstr(arguments[0], "/") == NULL){
                strcpy(operation, "/usr/bin/");
                strcat(operation, arguments[0]);
            }else{
                strcpy(operation, arguments[0]);
            }

            pid_t pid = fork();
            if(pid < 0){
                fprintf(stderr, "%s\n", "Error: fork failed");
                return;
            }else if(pid == 0){
                if(upstreamFile != NULL){
                    // I learned how to read a file from https://www.delftstack.com/howto/c/c-file-descriptor/
                    int input_fd = open(upstreamFile, O_RDONLY);
                    if(input_fd == -1){
                        fprintf(stderr, "%s\n", "Error: invalid file"); 
                        exit(1);
                    }
                    dup2(input_fd, 0);
                    close(input_fd);
                }
                
                if(destinationFile != NULL){
                    // I learned how to I/O in C and the many modes of reading and writing from https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
                    // I find the modes I should use from https://man7.org/linux/man-pages/man2/open.2.html
                    // I followed the instructions from the slides Chapter 2 page 32
                    int output_fd;
                    if(overwrite == 1){
                        output_fd = open(destinationFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                    }else{
                        output_fd = open(destinationFile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
                    }
                    if (output_fd == -1) {  
                        fprintf(stderr, "%s\n", "Error: invalid command"); 
                        exit(1);
                    }
                    // I learned dup2 from https://stackoverflow.com/questions/24538470/what-does-dup2-do-in-c and man page
                    dup2(output_fd, 1);
                    close(output_fd);
                }

                // I learned how to use execvp() by going to Jason Zhou's tutoring session at 8:30 pm, 10/10/23
                // I learned how to locate files in /usr/bin/ by attending Mayank's tutoring session at 11:30 am, 10/17/23
                execvp(operation, arguments); 

                fprintf(stderr, "%s\n", "Error: invalid program");
                exit(1);
            }else{
                int status;

                // I learned waitpid() from the man waitpid page
                // I learned how to use waitpid from https://stackoverflow.com/questions/21248840/example-of-waitpid-in-use
                // I learned the pid in the waitpid() from https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
                // I learned how this waitpid() command behaves from https://stackoverflow.com/questions/26957415/why-is-waitpid-1-status-0-not-suspending-the-process-without-any-childs-at
                pid_t w = waitpid(pid, &status, WUNTRACED);
                if (w == -1) {
                    fprintf(stderr, "%s\n", "Error: waitpid failed"); 
                }
                else if (WIFSTOPPED(status) && !isElement(pid)) {
                    suspendedJobIndex++;
                    suspendedJobIdList[suspendedJobIndex - 1] = pid;
                    suspendedJobList[suspendedJobIndex - 1] = strdup(originalCommand); 
                    suspendedJobList[suspendedJobIndex] = NULL;
                }
            }
        }
        if(upstreamFile){
            free(upstreamFile);
        }
        if(destinationFile){
            free(destinationFile);
        }
    }
    free(originalCommand);
    return;
}

/*一个用来生成prompt的程序，每次查找当前所在path*/
char * getPrompt(){
    size_t prompt_size;
    char *prompt;
     // I learned how to use the getcwd() system call and PATH_MAX from https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
    char path[PATH_MAX];

    if(getcwd(path, sizeof(path)) == NULL) {
        // I learned how to print to stderr from https://stackoverflow.com/questions/39002052/how-can-i-print-to-standard-error-in-c-with-printf
        fprintf(stderr, "%s\n", "Error: invalid path");
        prompt = "[nyush ]$ ";
    }else{
        // I learned how to split the path and search for "/" from https://www.tutorialspoint.com/c_standard_library/c_function_strrchr.htm
        char *relativePath = strrchr(path, '/');
        if(relativePath != NULL) {
            if (strlen(relativePath) > 1) {
                relativePath++;
            }
        }else{
            relativePath = path;
        }

        // I learned how to concatenate 2 strings from https://www.geeksforgeeks.org/concatenating-two-strings-in-c/#
        // I learned how to assign values to strings from https://stackoverflow.com/questions/3131319/how-can-i-correctly-assign-a-new-string-value
        prompt_size = sizeof("[nyush ") + strlen(relativePath) + sizeof("]$ ");
        prompt = (char *)malloc(prompt_size);
        strcat(prompt, "[nyush ");
        strcat(prompt, relativePath);
        strcat(prompt, "]$ ");
    }
    return prompt;
}

/*主程序，用getline来接受user input*/
int main(){
    // I learned how to set a new signal handler for signals from Module 2 slides
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTSTP, handler);

    char *prompt = getPrompt();
   
    printf("%s", prompt);
    // I learned how to flush stdout from https://stackoverflow.com/questions/1716296/why-does-printf-not-flush-after-the-call-unless-a-newline-is-in-the-format-strin
    fflush(stdout);

    // I learned how to use getline() from https://man7.org/linux/man-pages/man3/getline.3.html
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;
    while((nread = getline(&input, &len, stdin)) != -1){
        // I learned how to remove the next line from the output of getline() from https://stackoverflow.com/questions/13000047/function-to-remove-newline-has-no-effect 
        if(input[nread - 1] == '\n'){
            input[nread - 1] = '\0';
            nread--;
        }
        parser(input, NULL, NULL);

        prompt = getPrompt();
        printf("%s", prompt);
        // I learned how to flush stdout from https://stackoverflow.com/questions/1716296/why-does-printf-not-flush-after-the-call-unless-a-newline-is-in-the-format-strin
        fflush(stdout);
    }

    free(prompt);
    return 0;
}