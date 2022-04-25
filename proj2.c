#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <semaphore.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "proj2.h"


int arg_manage(args_t* args, char ** argv, int argc) {
    if(argc != 5){
        return 1;
    }
    char *ctl_ptr = '\0';
    args->NO = strtol(argv[1], &ctl_ptr, 10);
    if (args->NO < 0 || *ctl_ptr != '\0'){
        return 1;
    }
    args->NH = strtol(argv[2], &ctl_ptr, 10);
    if (args->NH < 0 || *ctl_ptr != '\0'){
        return 1;
    }
    args->TI = strtol(argv[3], &ctl_ptr, 10);
    if (args->TI < 0 || args->TI > 1000 || *ctl_ptr != '\0'){
        return 1;
    }
    args->TB = strtol(argv[4], &ctl_ptr, 10);
    if (args->TB < 0 || args->TB > 1000 || *ctl_ptr != '\0'){
        return 1;
    }
    return 0;
}

int main(int argc, char **argv){
    args_t args;
    if (arg_manage(&args, argv, argc)){
        fprintf(stderr, "Wrong argument format!\n");
        return 1;
    }
    FILE *f = fopen("proj2.out", "w");
    setbuf(f, NULL);
    pid_t pid = fork();

    if (pid == 0){
        for(int i = 0; i<1000;i++)
        fprintf(f,"Hello world 0\n");
        fclose(f);
        exit(0);
    }
    else if (pid == -1){
        fprintf(stderr, "ERROR");

    }
    else {
        for(int i = 0; i<1000;i++)
        fprintf(f,"Hello world 1\n");
    }
    fprintf(f,"Hello world\n");
    fclose(f);
    return 0;
}