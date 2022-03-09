#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h> 
#include <stdbool.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifndef RUN_H
#define RUN_H

#define READ_END 0 
#define WRITE_END 1

#define CHILD_EXEC_FAIL 255

typedef struct Pipe {
    char* pipeName; 
    bool checked; 
} Pipe;

void run_jobs(struct Job** jobsAvailable, int jobsToRun, struct Pipe** pipes, 
        Data* data, int argc);

void verify_runnable_filename(struct Job** jobsAvailable, int jobsToRun, 
        struct Pipe** pipes); 

void get_invalid_pipes(Job* job, struct Job** jobsAvailable, int totalJobs, 
        int invalidPipeSize, struct Pipe** pipes); 

void create_child(struct Job** jobsAvailable, int jobsToRun, int jobIdx,
        int* pid, int jobNumber);

void create_pipes(void); 

void check_pipe(char* pipeToCheck, int totalJobs, struct Job** jobsAvailable,
        int invalidPipeSize, struct Pipe** pipes, int pipeIndex, int pipeEnd);

int jobs_runnable(struct Job** jobsAvailable, int totalJobs); 

void set_execvp_args(struct Job** jobsAvailable, int jobPos, char** argsVec);

void check_jobs(int jobNumber, int* childPid, struct Job** jobsAvailable); 

void redirect(char* input, char* output); 

bool check_filename(Job* job); 

#endif 
