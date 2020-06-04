#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define Max_LIMIT_Input 600
#define Max_LIMIT_Command 30
#define Max_LIMIT_num_commands 20
int status = 0;

int execute_commands(char** commands, int num_commands);
int ParseInput(char* input);
char* break_Input(char* input);

int ParseInput(char* input) {
    char** commands = malloc(Max_LIMIT_num_commands*sizeof(char*));
    int num_commands = 0;
    if (strcmp(input, "exit") == 0 || strcmp(input, "Exit") == 0)
    {
        return -1;
    }
    else {
        for(int i = 0;i<Max_LIMIT_num_commands;i++) {
            commands[i] = malloc(Max_LIMIT_Command*sizeof(char));
        }
        int index = 0;
        char* token = strtok(input, " "); //Ref https://www.educative.io/edpresso/splitting-a-string-using-strtok-in-c
        while(token != NULL) {
            commands[index] = token;
            token = strtok(NULL, " ");
            index+=1;
            num_commands+=1;
        }
        execute_commands(commands, num_commands);
    }
    return 1;
}

int execute_commands(char** commands, int num_commands) {

    int p0[2];
    int p1[2];
    int cur_pipe = 0;
    int prev_pipe_flag = 0;


    for(int i = 0;i<num_commands;i++) {
        int append_flag = 0;
        int piping_flag = 0;
        int std_error_to_std_output = 0;
        int std_error_to_file = 0;
        int fd_redirect = 0;
        char* filename = "";
        char* filename_input = "";
        int flag_redirect_output = 0;
        int flag_redirect_input = 0;
        char** command = malloc(Max_LIMIT_num_commands*sizeof(char*));
        for(int i = 0;i<Max_LIMIT_num_commands;i++) {
            command[i] = malloc(Max_LIMIT_Command*sizeof(char));
        }

        if(cur_pipe) {
            if(pipe(p1) < 0) {
                printf("pipe%d unsuccessful\n", 1);
            }
        }
        else {
            if(pipe(p0) < 0) {
                printf("pipe%d unsuccessful\n", 0);
            } 
        }
        int index = 0;
        while(i<num_commands) {
            char* cur_command = commands[i];
            // printf("cur_command%d %s\n",i,cur_command);
            // printf("prev_pipe_flag %d\n",prev_pipe_flag );
            if(strcmp(cur_command, "2>&1") == 0) {
                        std_error_to_std_output = 1;
                        i+=1;
                        continue;
            }
            else if(strstr(cur_command, "1>") != NULL || strstr(cur_command, "2>") != NULL ) {
                char* token = strtok(cur_command, ">"); //Ref https://www.educative.io/edpresso/splitting-a-string-using-strtok-in-c
                if(strcmp(cur_command, "2") == 0) {
                    fd_redirect = 2;
                }
                else {
                    fd_redirect = 1;
                }
                // printf("fd_redirect %d\n", fd_redirect); 
                while(token != NULL) {
                    token = strtok(NULL, ">");
                    if(token!=NULL)
                        filename = token;
                }
                flag_redirect_output = 1;
                i+=1;
                continue;
            }


            if(strcmp(commands[i], ">") == 0 || strcmp(commands[i], ">>") == 0) {
                if(strcmp(commands[i], ">>") == 0) {
                    append_flag = 1;
                }
                i+=1;
                filename = commands[i];
                flag_redirect_output = 1;
                fd_redirect = 1;
                i+=1;
                continue;
            }

            if(strcmp(commands[i], "<") == 0) {
                i+=1;
                filename_input = commands[i];
                flag_redirect_input = 1;
                i+=1;
                continue;
            }

            // printf("filename %s\n",filename );

            if(strcmp(commands[i], "|") == 0) {
                piping_flag = 1;
                break;
            }


            if(i < num_commands) {
                command[index] = commands[i];            
                // printf("added to list %s\n",command[index]);
                index+=1;
            }
            i++;
        }
        command[index] = NULL;
        int pid = fork();
        if(pid == 0) {
            // printf("%s\n","Child process");
            // printf("cur pipe %d\n",cur_pipe );
            if(flag_redirect_output == 1) {
                // printf("%s\n","Redirected Output" );
                close(fd_redirect);
                if(append_flag)
                    open(filename, O_RDWR|O_CREAT|O_APPEND, 0600);
                else
                    open(filename, O_RDWR|O_CREAT|O_TRUNC, 0600);
            }

            if(flag_redirect_input) {
                // printf("%s\n","Redirected input" );
                close(0);
                open(filename_input, O_RDWR|O_CREAT|O_APPEND, 0600);
            }

            if(prev_pipe_flag) {
                // printf("prev_pipe_flag set \n");
                close(0);
                if(cur_pipe) {
                    dup(p0[0]);
                }
                else {
                    dup(p1[0]);
                }
            }

            if(piping_flag) {
                // printf("%s\n","Piping flag" );
                close(1);
                if(cur_pipe) {
                    dup(p1[1]);
                }
                else {
                    dup(p0[1]);
                }

            }
            if(std_error_to_std_output) {
                // printf("%s\n","Redirected std error to std Output" );
                close(2);
                dup(1);
            }
            execvp(*command, command);
            exit(0);

        }
        else {
            waitpid(pid, NULL, 0);
            if(prev_pipe_flag) {
                if(cur_pipe) {
                    close(p0[0]);
                }
                else {
                    close(p1[0]);
                }
            }

            if(piping_flag) {
                if(cur_pipe) {
                    close(p1[1]);
                }
                else {
                    close(p0[1]);
                }

            }

            if(prev_pipe_flag) {
                prev_pipe_flag = 0;
            }
            if(piping_flag) {
                prev_pipe_flag = 1;
            }
            cur_pipe = 1 - cur_pipe;
        }
    }
    pid_t wpid;
    while ((wpid = wait(&status)) > 0);
    return 0;
}


char* break_Input(char* input) {
    int length_instruction = strcspn(input, "\n");
    char* parsed_instr = malloc(length_instruction*sizeof(char));
    memcpy(parsed_instr, input, length_instruction);
    return parsed_instr;
}

void print_start_message(void){
    printf("Shell made by Pranjal Kaura 2017079\n");
    printf("Please enter commands in shell in the following format:\n");
    printf("command\n");
    printf("command > filename\n");
    printf("command >> filename\n");
    printf("command < filename\n");
    printf("1>filename\n");
    printf("2>filename\n");
    printf("2>&1\n");
    printf("command1 | command2\n");
    printf("================================================================================\n");
    printf("                WELCOME\n");
    printf("================================================================================\n");
}

void print_directory() {
    char dir[Max_LIMIT_Input]; 
    getcwd(dir, sizeof(dir));
    printf("%s> ", dir);
}
//referred my code from previous year for above 2 functions

int main (int argc, char *argv[])
{
    int OS_output = 1;
    print_start_message();
    while(OS_output == 1) { 
        char* input = malloc(Max_LIMIT_Input*sizeof(char));
        print_directory();
        fgets(input, Max_LIMIT_Input, stdin);
        input = break_Input(input);
        OS_output = ParseInput(input);
    }
    printf("EXITING SHELL\n");
    return 0;
}





