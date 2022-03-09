#include "jobrunner.h"
#include "run.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h>
#include <unistd.h> 
#include <stdbool.h> 
#include <csse2310a3.h> 
#include <sys/types.h>
#include <sys/wait.h>

/* Main function */ 
int main(int argc, char** argv) {
    int jobfileCount = 0;
    Data data; 
    jobfileCount = parse_cmd_args(argc, argv, &data); 
    struct Job** jobsAvailable = calloc(JOB_BUFFER_SIZE, sizeof(struct Job*));
    struct Pipe** pipes = calloc(JOB_BUFFER_SIZE, sizeof(struct Pipe*)); 
    int jobsToRun = 0; 
    //Tries to open each jobfile 
    for (int i = 0; i < jobfileCount; i++) {
        char* jobFileName = data.jobSpecFiles[i];
        FILE* jobFile = fopen(jobFileName, "r"); 
        if (jobFile == NULL) {
            free(jobsAvailable);
            exit_program(INVALID_JOBSPEC_FILE, jobFileName, 0); 
        }
        //read each jobFile 
        jobsToRun += 
                read_jobfile(jobFile, jobFileName, jobsAvailable); 
        fclose(jobFile);
    }
    if (jobsToRun < 1) { //checks if there are jobs to run
        free(jobsAvailable); 
        exit_program(NO_RUNNABLE_JOBS, NULL, 0); 
    } 
    run_jobs(jobsAvailable, jobsToRun, pipes, &data, argc); //runs jobs
    free_jobs(jobsAvailable);
    free(pipes); 
    return 0; 
}

/*
 * Processes command line arguments 
 *
 * @param argc Number of arguments given 
 * @param argv Arguments passed 
 * @param data Data struct 
 * @return number of jobfiles arguments   
 */ 
int parse_cmd_args(int argc, char** argv, Data* data) {
    if (argc < 2) {
        exit_program(INVALID_CMD_ARGS, NULL, 0); 
    }
    data->v = false;
    data->jobSpecFiles = malloc(sizeof(char*) * argc - 1);
    for (int i = 0; i < (argc - 1); i++) {
        data->jobSpecFiles[i] = malloc(sizeof(char)); 
    }
    int jobfileCount = 0; 
    if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0) {
            exit_program(INVALID_CMD_ARGS, NULL, 0);
        } else {
            data->jobSpecFiles[0] = argv[1];
            jobfileCount++; 
            return jobfileCount; 
        } 
    } else {
        int jobFileIndex = 1; 
        if (strcmp(argv[1], "-v") == 0) {
            data->v = true;
            jobFileIndex++; 
        }
        for (int i = jobFileIndex; i < argc; i++) {
            data->jobSpecFiles[jobfileCount] = argv[i];
            jobfileCount++; 
            //'-v' is in any other position other than 1st 
            if (strcmp(argv[i], "-v") == 0) {
                exit_program(INVALID_CMD_ARGS, NULL, 0); 
            }
        }
    }
    return jobfileCount; 
}

/**
 * Exits the program following invalid command line input 
 * Prints message to stderr 
 *
 * @param exitStatus cause for the program to exit 
 * @param filename name of jobfile 
 * @param lineNum Line Number of jobfile 
 * @exits with exitStatus 
 */
void exit_program(int exitStatus, char* filename, int lineNum) {
    switch (exitStatus) {
        //usage error 
        case INVALID_CMD_ARGS: 
            fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n"); 
            break; 
        case INVALID_JOBSPEC_FILE: 
            fprintf(stderr, "jobrunner: file \"%s\" can not be opened\n", 
                    filename);
            break; 
        case INVALID_JOBSPEC_LINE:
            fprintf(stderr, "jobrunner: invalid job specification" 
                    " on line %d of \"%s\"\n", lineNum, filename); 
            break;
        case NO_RUNNABLE_JOBS: 
            fprintf(stderr, "jobrunner: no runnable jobs\n");

        default: 
            break; 
    }
    exit(exitStatus); 
}

/**
 * Reads each jobfile and tries to parse valid lines into fields. Adds each
 * runnable job into jobsAvailable array. 
 * 
 * @param jobfile FILE of jobfile 
 * @param filename name of jobfile 
 * @param jobsAvailable array of job structs that are runnable 
 *
 */ 
int read_jobfile(FILE* jobfile, char* filename, struct Job** jobsAvailable) {
    int lineNum = 1;
    char* line;
    int numJobs = 0; 
    line = read_line(jobfile);  
    while (line != NULL) { 
        //ignores comments and empty lines 
        if (line[0] != '#' && strlen(line) > 0) {
            //Parse Line contents
            char** jobDetails = split_by_commas(line);
            parse_jobdetails(jobDetails, filename, lineNum, jobsAvailable);
            numJobs++;
            free(jobDetails); 
        } 
        lineNum++;
        line = read_line(jobfile); 
    } 
    free(line);
    return numJobs; 
}

/** 
 * Parses jobDetails and checks for invalid lines. 
 * Adds valid jobs to jobsAvailable array. 
 *
 * @param jobDetails details of line being parsed 
 * @param filename name of file 
 * @param lineNum line number referring to current line being parsed 
 * @param jobsAvailable array of Job structs
 */ 
void parse_jobdetails(char** jobDetails, char* filename, int lineNum, 
        struct Job** jobsAvailable) { 
    int fieldCounter, jobsArrayIndex = 0; 
    fieldCounter = count_fields(jobDetails);
    if (fieldCounter < 3) {
        free(jobsAvailable); 
        exit_program(INVALID_JOBSPEC_LINE, filename, lineNum); 
    }
    while(jobsAvailable[jobsArrayIndex] != NULL) jobsArrayIndex++; 
    struct Job* newJob = calloc(1, sizeof(Job)); 
    newJob->program = jobDetails[0];
    newJob->stdinput = jobDetails[1]; 
    newJob->stdoutput = jobDetails[2];
    newJob->runnable = true; 
    //check for empty mandatory fields 
    check_mand_fields(newJob->program, newJob->stdinput, newJob->stdoutput, 
            filename, lineNum, jobsAvailable); 
    char* timeout = jobDetails[3]; 
    if (fieldCounter > 3) {
        if (*timeout == '\0') {
            timeout = "0"; 
        } else {
            if (!check_timeout(timeout)) {
                free(jobsAvailable); 
                exit_program(INVALID_JOBSPEC_LINE, filename, lineNum); 
            }
        }
        newJob->timeout = atoi(timeout);
        if (newJob->timeout < 0) { //check for non-negative integer 
            free(jobsAvailable); 
            exit_program(INVALID_JOBSPEC_LINE, filename, lineNum); 
        }
    }
    if (fieldCounter > 4) { //optional arguments..
        int numArgs = fieldCounter - 4;
        newJob->args = malloc(sizeof(char*) * numArgs);
        int numArgsParsed = 0;
        int numArgsIdx = 4; 
        while (numArgsParsed < numArgs) {
            newJob->args[numArgsParsed] = malloc(sizeof(char*) * 
                    sizeof(jobDetails[numArgsIdx])); 
            newJob->args[numArgsParsed] = jobDetails[numArgsIdx];
            numArgsIdx++; 
            numArgsParsed++; 
        }
    } else {
        newJob->args = malloc(sizeof(char*) * 1); 
        newJob->args[0] = NULL; 
    } 
    jobsAvailable[jobsArrayIndex] = newJob; //add to jobsAvailable array
}

/**
 * Helper function to check if there are mandatory fields missing
 *
 * @param program name of program arg
 * @param stdinput name of stdinput arg 
 * @param stdoutput name of stdoutput arg
 * @param filename name of file 
 * @param lineNum line number currently being parsed 
 * @param jobsAvailable array of job structs 
 */ 
void check_mand_fields(char* program, char* stdinput, char* stdoutput, 
        char* filename, int lineNum, struct Job** jobsAvailable) {
    if ((program == NULL || *program == '\0') ||
            (stdinput == NULL || *stdinput == '\0') ||
            (stdoutput == NULL || *stdoutput == '\0')) { 
        //free(jobsAvailable); 
        exit_program(INVALID_JOBSPEC_LINE, filename, lineNum); 
    }
}

/**
 * Helper function to count number of fields in jobDetails array  
 *
 * @param jobsDetails the array to be counted 
 */ 
int count_fields(char** jobDetails) {
    int fieldCounter = 0; 
    while (jobDetails[fieldCounter]) {
        fieldCounter++; 
    }
    return fieldCounter; 
}

/**
 * Returns true if timeout field is valid, false otherwise. 
 *
 * @param timeoutVal the timeout string to be checked 
 */  
bool check_timeout(char* timeoutVal) {
    //check for leading or trailing spaces OR numerical integer  
    for (int i = 0; timeoutVal[i] != '\0'; i++) {
        if (isspace(timeoutVal[i]) != 0 || isdigit(timeoutVal[i]) == 0) {
            return false; 
        } 
    } 
    return true; 
}

/**
 * Frees memory associated with Job struct and Data struct 
 *
 * @param jobsAvailable array of jobs structs 
 * @param data data struct 
 *
 */ 
void free_jobs(struct Job** jobsAvailable) {
    int i = 0; //index for each job in jobsAvaialble array 
    while (jobsAvailable[i] != NULL) {
        if (jobsAvailable[i] == NULL) {
            return; 
        } 
        free(jobsAvailable[i]);  
        i++; 
    }
    free(jobsAvailable);
}

/**
 * Free data struct 
 *
 * @param data The data struct to be freed of memory 
 * @argc number of arguments parsed in command line
 */ 
void free_data(Data* data, int argc) {
    for (int i = 0; i < (argc - 1); i++) {
        free(data->jobSpecFiles[i]); 
    }
    free(data->jobSpecFiles); 
}
