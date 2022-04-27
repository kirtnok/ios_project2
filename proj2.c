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

sem_t *print_mutex;
sem_t *proc_mutex;
sem_t *oxy_sem;
sem_t *hydr_sem;
sem_t *turnstile;
sem_t *turnstile2;
sem_t *barier_mutex;
#define mysleep(max) { usleep(1000 * (rand() % (max + 1))); }
int *num_proc =NULL;
int *oxy_cnt = NULL;
int *hydr_cnt =NULL;
int *mol_id =NULL;
int *barier_count =NULL;
FILE *f;
int shared_init(){
    num_proc = mmap(NULL, sizeof(*num_proc), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == num_proc) {
        printf("Could not map memory\n");
        return 1;
    }
    oxy_cnt = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    hydr_cnt = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    mol_id = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    barier_count =  mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //todo error handling
    *num_proc=1;
    *oxy_cnt=0;
    *hydr_cnt=0;
    *mol_id=1;
    *barier_count=0;
    return 0;
}
void shared_dest(){
    munmap(num_proc, sizeof(int*));
    munmap(oxy_cnt, sizeof(int*));
    munmap(hydr_cnt, sizeof(int*));
    munmap(mol_id, sizeof(int*));
    munmap(barier_count, sizeof(int*));
}
int semafor_init(){
    print_mutex = sem_open("/xkontr.print_mutex", O_CREAT | O_EXCL, 0644, 1);
    proc_mutex = sem_open("/xkontr.proc_mutex", O_CREAT | O_EXCL, 0644, 1);
    oxy_sem = sem_open("/xkontr.oxy_sem", O_CREAT | O_EXCL, 0644, 0);
    hydr_sem = sem_open("/xkontr.hydr_sem", O_CREAT | O_EXCL, 0644, 0);
    turnstile = sem_open("/xkontr.turnstile", O_CREAT | O_EXCL, 0644, 0);
    turnstile2 = sem_open("/xkontr.turnistile2", O_CREAT | O_EXCL, 0644, 1);
    barier_mutex = sem_open("/xkontr.barier_mutex", O_CREAT | O_EXCL, 0644, 1);
    return 0;

}
void delete_semafores()
{
    sem_close(print_mutex);
    sem_unlink("/xkontr.print_mutex");
    sem_close(proc_mutex);
    sem_unlink("/xkontr.proc_mutex");
    sem_close(oxy_sem);
    sem_unlink("/xkontr.oxy_sem");
    sem_close(hydr_sem);
    sem_unlink("/xkontr.hydr_sem");
        sem_close(turnstile);
    sem_unlink("/xkontr.turnstile");
        sem_close(turnstile2);
    sem_unlink("/xkontr.turnistile2");
        sem_close(barier_mutex);
    sem_unlink("/xkontr.barier_mutex");
}
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
void my_print( const char * format, ... ){
    sem_wait(print_mutex);
    va_list args;
    va_start (args, format);
    fprintf(f,"%d: ",(*num_proc)++);
    vfprintf (f, format, args);
    va_end (args);
    sem_post(print_mutex);
}
void oxygen_function(args_t*args,int index){
    my_print("O %d: started\n",index);
    mysleep(args->TI);
    //my_print("O %d: going to queue\n",index);
    
    sem_wait(proc_mutex);
    my_print("O %d: going to queue\n",index);
    (*oxy_cnt) += 1;
    if((*hydr_cnt) >= 2){
        sem_post(hydr_sem);
        sem_post(hydr_sem);
        (*hydr_cnt) -=2;
        sem_post(oxy_sem);
        (*oxy_cnt) -=1;
    }
    else{
        sem_post(proc_mutex);
    }
    sem_wait(oxy_sem);
    my_print("voda sa vytvara\n");
    //barier
    sem_wait(barier_mutex);
        (*barier_count) +=1;
        if ((*barier_count) == 3){
            sem_wait(turnstile2);
            sem_post(turnstile);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile);
    sem_post(turnstile);
    my_print("voda spravena\n");
    sem_wait(barier_mutex);
        (*barier_count) -= 1;
        if((*barier_count)==0){
            sem_wait(turnstile);
            sem_post(turnstile2);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile2);
    sem_post(turnstile2);
    //end of barier
    sem_post(proc_mutex);

}
void hydrogen_function(args_t*args, int index){
    my_print("H %d: started\n",index);
    mysleep(args->TI);

    
    sem_wait(proc_mutex);
    my_print("H %d: going to queue\n",index);
    (*hydr_cnt) +=1;
    if((*hydr_cnt)>=2 && (*oxy_cnt)>=1){
        sem_post(hydr_sem);
        sem_post(hydr_sem);
        (*hydr_cnt) -=2;
        sem_post(oxy_sem);
        (*oxy_cnt) -=1;
    }
    else{
        sem_post(proc_mutex);
    }
    sem_wait(hydr_sem);

    my_print("voda sa vytvara\n");
    sem_wait(barier_mutex);
        (*barier_count) += 1;
        if ((*barier_count) == 3){
            sem_wait(turnstile2);
            sem_post(turnstile);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile);
    sem_post(turnstile);
    my_print("voda spravena\n");
    sem_wait(barier_mutex);
        (*barier_count) -=1;
        if((*barier_count)==0){
            sem_wait(turnstile);
            sem_post(turnstile2);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile2);
    sem_post(turnstile2);

}
int main(int argc, char **argv){
    args_t args;
    sem_close(oxy_sem);
    if (arg_manage(&args, argv, argc)){
        fprintf(stderr, "Wrong argument format!\n");
        return 1;
    }
    f = fopen("proj2.out", "w+");
    setbuf(f, NULL);
    shared_init();
    semafor_init();

    for(int i = 1; i<=args.NO;i++){
        pid_t pid = fork();
        if(pid==0){
            oxygen_function(&args, i);
            exit(0);
        }
        else if (pid==-1){
            fprintf(stderr, "ERROR");
            fclose(f);
            shared_dest();
            delete_semafores();
            exit(1);
        }
    }
    for(int i = 1; i<=args.NH;i++){
        pid_t pid = fork();
        if(pid==0){
            hydrogen_function(&args, i);
            exit(0);
        }
        
        else if (pid==-1){
            fprintf(stderr, "ERROR");
            fclose(f);
            shared_dest();
            delete_semafores();
            exit(1);
        }
    }
    while(wait(NULL)>0);
    shared_dest();
    delete_semafores();
    fclose(f);
    return 0;
}