#include "jobrunner.h"
#include "run.h"

#include <stdio.h> 
#include <string.h> 
#include <ctype.h> 
#include <unistd.h> 
#include <stdbool.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

/**
 * Starts the child creation process
 *
 * @param jobsAvailable array of job structs 
 * @param jobsToRun total number of jobs to run 
 * @param pipes array of invalid pipe structs 
 */ 
void run_jobs(struct Job** jobsAvailable, int jobsToRun, struct Pipe** pipes, 
        Data* data, int argc) {
    pid_t pid[jobsToRun]; 
    int jobNumber = 1;

    //check job runnabiltiy 
    verify_runnable_filename(jobsAvailable, jobsToRun, pipes); 
    
    for (int jobIdx = 0; jobIdx < jobsToRun; jobIdx++) {
        if (jobsAvailable[jobIdx]->runnable) { //run runnable jobs
            if (data->v) { //verbose mode 
                printf("%d:%s:%s:%s:%d:%s\n", jobNumber, 
                        jobsAvailable[jobIdx]->program, 
                        jobsAvailable[jobIdx]->stdinput, 
                        jobsAvailable[jobIdx]->stdoutput, 
                        jobsAvailable[jobIdx]->timeout, 
                        *jobsAvailable[jobIdx]->args); 
                fflush(stderr); 
            }
            create_child(jobsAvailable, jobsToRun, jobIdx, pid, jobNumber);
            jobNumber++; 
        }
    } 
    check_jobs(jobsToRun, pid, jobsAvailable); 
}

/**
 * Check if jobs are runnable: check for invalid filename and invalid pipe 
 * usage 
 *
 * @param jobsAvailable array of Job structs 
 * @param jobsToRun total number of jobs 
 * @param pipes array of invalid pipe structs 
 */ 
void verify_runnable_filename(struct Job** jobsAvailable, int jobsToRun, 
        struct Pipe** pipes) {
    const int totalJobs = jobsToRun; 
    //check for invalid filename for stdin and stdout 
    for (int jobIdx = 0; jobIdx < totalJobs; jobIdx++) {
        if (!check_filename(jobsAvailable[jobIdx])) { 
            jobsAvailable[jobIdx]->runnable = false;  
        }
    }
    int invalidPipeSize = 0; 
    //Retrieves all invalid pipes 
    for (int jobIdx = 0; jobIdx < totalJobs; jobIdx++) {
        get_invalid_pipes(jobsAvailable[jobIdx], jobsAvailable, totalJobs, 
                invalidPipeSize, pipes);
    }
    for (int jobIdx = 0; jobIdx < totalJobs; jobIdx++) {
        int i = 0; //counter to iterate through invalid pipes array 
        while (pipes[i] != NULL) { 
            //checks pipename in a job is invalid or not
            if (strcmp(jobsAvailable[jobIdx]->stdinput
                        , pipes[i]->pipeName) == 0 && (!pipes[i]->checked)) {
                char* invalReadPipe = pipes[i]->pipeName + 1; 
                fprintf(stderr, "Invalid pipe usage \"%s\"\n", 
                        invalReadPipe);
                pipes[i]->checked = true;
                jobsAvailable[jobIdx]->runnable = false; 
            } else if (strcmp(jobsAvailable[jobIdx]->stdoutput
                        , pipes[i]->pipeName) == 0 && (!pipes[i]->checked)) {
                char* invalWritePipe = pipes[i]->pipeName + 1; 
                fprintf(stderr, "Invalid pipe usage \"%s\"\n", 
                        invalWritePipe); 
                pipes[i]->checked = true;
                jobsAvailable[jobIdx]->runnable = false; 
            } 
            i++;
        }
    }
    //check if there are runnable jobs available 
    if (jobs_runnable(jobsAvailable, totalJobs) < 1) { 
        free_jobs(jobsAvailable); 
        exit_program(NO_RUNNABLE_JOBS, NULL, 0);  
    }
}

/**
 * Counts the number of jobs that are runnable. 
 *
 * @param jobsAvailable array of Job structs 
 * @param totalJobs total number of jobs 
 *
 * @return the number of runnable jobs 
 */ 
int jobs_runnable(struct Job** jobsAvailable, int totalJobs) {
    int runnableJobs = 0;
    for (int i = 0; i < totalJobs; i++) {
        if (jobsAvailable[i]->runnable) {
            runnableJobs++; 
        }
    }
    return runnableJobs; 
}

/**
 * Gets all the invalid pipenames and stores it to pipes array
 *
 * @param job specific job 
 * @param jobsAvailable array of job structs 
 * @param totalJobs total numbner of jobs available 
 * @param invalidPipeSize size of the invalid pipes array
 * @param pipes array of invalid pipe structs 
 */ 
void get_invalid_pipes(Job* job, struct Job** jobsAvailable, int totalJobs, 
        int invalidPipeSize, struct Pipe** pipes) {

    int pipeIndex = 0;

    //checks read pipes 
    while (pipes[pipeIndex] != NULL) pipeIndex++; 
    check_pipe(job->stdinput, totalJobs, jobsAvailable, invalidPipeSize, pipes,
            pipeIndex, 0);

    //checks write pipes 
    while(pipes[pipeIndex] != NULL) pipeIndex++; 
    check_pipe(job->stdoutput, totalJobs, jobsAvailable, invalidPipeSize, 
            pipes, pipeIndex, 1); 
}

/** 
 * Checks the pipe given the pipename if it is valid or not. 
 *
 * @param pipeToCheck the pipename of the pipe to be checked for validity 
 * @param totalJobs total number of jobs 
 * @param jobsAvailable array of job structs 
 * @param invalidPipeSize number of invalid pipes in the array 
 * @param pipeIndex counter to iterate through the invalid pipes array 
 * @param pipeEnd 0 if read end of the pipe, 1 if write end
 */ 
void check_pipe(char* pipeToCheck, int totalJobs, struct Job** jobsAvailable, 
        int invalidPipeSize, struct Pipe** pipes, int pipeIndex, 
        int pipeEnd) {

    if (strncmp(pipeToCheck, "@", 1) == 0) {
        int pipeMatch = 0; 
        for (int jobIdx = 0; jobIdx < totalJobs; jobIdx++) {
            if (pipeEnd == 0) {
                if (strcmp(pipeToCheck, 
                        jobsAvailable[jobIdx]->stdoutput) == 0) {
                    pipeMatch++; 
                }
            } else {
                if (strcmp(pipeToCheck, 
                        jobsAvailable[jobIdx]->stdinput) == 0) {
                    pipeMatch++; 
                }
            }
        }
        //check if pipe has exactly one reader and exactly one writer 
        if (pipeMatch != 1) {
            struct Pipe* invalidPipe = calloc(1, sizeof(Pipe));
            invalidPipe->pipeName = pipeToCheck; 
            invalidPipe->checked = false;
            invalidPipeSize++; 
            pipes[pipeIndex] = invalidPipe; 
        }
    }
}

/**
 * Creates a child process for each job. 
 *
 * @param jobsAvailable array of job structs 
 * @param jobsToRun total number of jobs to run 
 * @param jobIdx index counter of jobsAvailable array  
 * @param pid int array containing pid of child processes  
 */ 
void create_child(struct Job** jobsAvailable, int jobsToRun, int jobIdx,
        int* pid, int jobNumber) { 
    char* argsVec[3]; 
    set_execvp_args(jobsAvailable, jobIdx, argsVec);
    if ((pid[jobIdx] = fork()) == 0) { //child process
        int devNull = open("/dev/null", O_WRONLY); 
        dup2(devNull, STDERR_FILENO);

        char* inputRedir = jobsAvailable[jobIdx]->stdinput; 
        char* outputRedir = jobsAvailable[jobIdx]->stdoutput;
        redirect(inputRedir, outputRedir);

        execvp(jobsAvailable[jobIdx]->program, jobsAvailable[jobIdx]->args);
        
        exit(255); //exec fails  
    } else if (pid[jobIdx] > 0) { //parent process

    }
}

/**
 * Redirects input and output of job process to given valid filename.
 *
 * @param input filename of file to redirect input from 
 * @param output filename of file to redirect output to 
 */ 
void redirect(char* input, char* output) {
    if (strcmp(input, "-") != 0) {
        int newInput = open(input, O_RDONLY); 
        dup2(newInput, 0); 
        close(newInput); 
    } 
    if (strcmp(output, "-") != 0) {
        int newOutput = open(output, O_WRONLY); 
        dup2(newOutput, 1); 
        close(newOutput); 
    }
}

/**
 * Sets execvp arguments, 1st element is the program name, 2nd contains
 * args passed in jobfile, last element is a NULL pointer. 
 *
 *  @param jobsAvailable array of job structs 
 *  @param jobIdx the job position on the jobsAvailable array  
 *  @param argsVec arguments vector passed initially 
 */ 
void set_execvp_args(struct Job** jobsAvailable, int jobIdx, char** argsVec) {
    argsVec[0] = jobsAvailable[jobIdx]->program;
    argsVec[2] = NULL; 
    if (jobsAvailable[jobIdx]->args == NULL) {
        argsVec[1] = NULL;
    } else {
        argsVec[1] = *(jobsAvailable[jobIdx]->args); 
    }
    jobsAvailable[jobIdx]->args = argsVec; 
}

/**
 * Monitors the process of jobs each second. Prints messages about their status
 *
 * @param jobsToRun total number of jobs to run 
 * @param childPid int array containing pid of child processes
 * @param jobsAvailable array of job structs
 * @param status The exit code 
 */ 
void check_jobs(int jobsToRun, int* childPid, struct Job** jobsAvailable) {
    int status;
    for (int jobIdx = 0; jobIdx < jobsToRun; jobIdx++) {
        if (jobsAvailable[jobIdx]->runnable) {
            sleep(1); 
            pid_t result = waitpid(childPid[jobIdx], &status, WNOHANG); 
            if (result < 0) {
                continue; 
            }
            if (WIFEXITED(status)) {
                fprintf(stderr, "Job %d exited with status %d\n", (jobIdx + 1),
                        WEXITSTATUS(status));
                sleep(1); 
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Job %d exited with signal %d\n", (jobIdx + 1),
                        WTERMSIG(status));
                sleep(1); 
            } else {

            }
        }
    }
}

/**
 * Checks the filename of stdout and stdin for a job. If stdin file for 
 * reading or stdout file for writing cannot be opened, the job is not 
 * executed further. 
 *
 * @param job Job containing details for stdin and stdout checking 
 *
 * @return true if filenames can be opened, false otherwise.  
 */ 
bool check_filename(Job* job) {
    bool validFilename = true; 
    char* stdinFilename = job->stdinput;
    char* stdoutFilename = job->stdoutput; 
    int stdinFile = open(stdinFilename, O_RDONLY);
    int stderrFile = open(stdoutFilename, O_RDONLY); 

    if (stdinFile < 0 && (strcmp(stdinFilename, "-") != 0) && 
            strncmp(stdinFilename, "@", 1)) {
        fprintf(stderr, "Unable to open \"%s\" for reading\n", 
                stdinFilename);
        validFilename = false; 
    }

    if (stderrFile < 0 && (strcmp(stdoutFilename, "-") != 0) &&
            strncmp(stdoutFilename, "@", 1)) {
        fprintf(stderr, "Unable to open \"%s\" for writing\n", 
                stdoutFilename);
        validFilename = false; 
    }
    if (!validFilename) {
        return false; 
    }
    return true; 
}
