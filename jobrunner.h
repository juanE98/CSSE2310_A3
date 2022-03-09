#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h> 
#include <unistd.h> 
#include <stdbool.h> 
#include <csse2310a3.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef JOBRUNNER_H 
#define JOBRUNNER_H 

#define READ_END 0 
#define WRITE_END 1 

#define INVALID_CMD_ARGS 1 
#define INVALID_JOBSPEC_FILE 2 
#define INVALID_JOBSPEC_LINE 3 
#define NO_RUNNABLE_JOBS 4

#define JOB_BUFFER_SIZE 20

typedef struct Data {
    bool v; 
    char** jobSpecFiles; 
} Data; 

typedef struct Job {
    char* program; 
    char* stdinput; 
    char* stdoutput; 
    int timeout; 
    char** args;
    bool runnable; 
} Job; 

int parse_cmd_args(int argc, char** argv, Data* data); 

void exit_program(int exitStatus, char* filename, int lineNum);

int read_jobfile(FILE* jobfile, char* filename, struct Job** jobsAvailable);

void parse_jobdetails(char** jobDetails, char* filename, int lineNum, 
        struct Job** jobsAvailable);

void check_mand_fields(char* program, char* stdinput, char* stdoutput, 
        char* filename, int lineNum, struct Job** jobsAvailable);

int count_fields(char** jobDetails); 

bool check_timeout(char* timeoutVal);

int numArgsCounter(Job* job); 

void free_jobs(struct Job** jobsAvailable);

void free_data(Data* data, int argc);

#endif 
