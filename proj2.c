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
#include <string.h>

// makro pre vypocet rendomneho casu
#define mysleep(max) { usleep(1000 * (rand() % (max + 1))); }
// struktura pre uzivatelsky input
typedef struct arguments {
    long int NO;
    long int NH;
    long int TI;
    long int TB;

} args_t;
// deklaracia semaforov
sem_t *print_mutex; // semafor pri tlaceni sprav do suboru
sem_t *proc_mutex; // semafor procesu
sem_t *oxy_sem; // semafor fronty  kyslika
sem_t *hydr_sem; // semafor fronty vodika
sem_t *turnstile; // barierovy semafor
sem_t *turnstile2; // barierovy semafor
sem_t *barier_mutex; // barierovy semafor
// deklaracia zdielannych premennych
int *num_proc = NULL; // pocitadlo zapisanych procesov
int *oxy_cnt = NULL; // pocitadlo aktualnej kyslikovej fronty
int *hydr_cnt = NULL; // pocitadlo aktualnej vodikovej fronty
int *barier_count = NULL; // pocitadlo v bariere
int *molecule_count = NULL; // pocitadlo vytvorenych molekul
int *oxy_all = NULL; // pocitadlo vsetkych kyslikov ktore vosli do procesu
int *hydr_all = NULL; // pocitadlo vsetkych vodikov ktore vosli do procesu

FILE *f;
// mapovanie zdielannych premennych
int shared_init(){
    num_proc = mmap(NULL, sizeof(*num_proc), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == num_proc) {
        return 1;
    }
    oxy_cnt = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == oxy_cnt) {
        return 1;
    }
    hydr_cnt = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == hydr_cnt) {
        return 1;
    }
    barier_count =  mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == barier_count) {
        return 1;
    }
    molecule_count =  mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == molecule_count) {
        return 1;
    }
    oxy_all = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == oxy_all) {
        return 1;
    }
    hydr_all = mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE,  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ( MAP_FAILED == hydr_all) {
        return 1;
    }
    // inicializacia zdielanych premennych
    *num_proc=1;
    *oxy_cnt=0;
    *hydr_cnt=0;
    *barier_count=0;
    *molecule_count=1;
    *oxy_cnt = 0;
    *hydr_cnt = 0;
    return 0;
}
// destructor zdielanych premennych
void shared_dest(){
    munmap(num_proc, sizeof(int*));
    munmap(oxy_cnt, sizeof(int*));
    munmap(hydr_cnt, sizeof(int*));
    munmap(barier_count, sizeof(int*));
    munmap(molecule_count, sizeof(int*));
    munmap(oxy_all, sizeof(int*));
    munmap(hydr_all, sizeof(int*));
}
// otvaranie semaforov
int semafor_init(){
    print_mutex = sem_open("/xkontr.print_mutex", O_CREAT | O_EXCL, 0644, 1);
    if (print_mutex == SEM_FAILED){
        return 1;
    }
    proc_mutex = sem_open("/xkontr.proc_mutex", O_CREAT | O_EXCL, 0644, 1);
    if (proc_mutex == SEM_FAILED){
        return 1;
    }
    oxy_sem = sem_open("/xkontr.oxy_sem", O_CREAT | O_EXCL, 0644, 0);
    if (oxy_sem == SEM_FAILED){
        return 1;
    }
    hydr_sem = sem_open("/xkontr.hydr_sem", O_CREAT | O_EXCL, 0644, 0);
    if (hydr_sem == SEM_FAILED){
        return 1;
    }
    turnstile = sem_open("/xkontr.turnstile", O_CREAT | O_EXCL, 0644, 0);
    if (turnstile == SEM_FAILED){
        return 1;
    }
    turnstile2 = sem_open("/xkontr.turnistile2", O_CREAT | O_EXCL, 0644, 1);
    if (turnstile2 == SEM_FAILED){
        return 1;
    }
    barier_mutex = sem_open("/xkontr.barier_mutex", O_CREAT | O_EXCL, 0644, 1);
    if (barier_mutex == SEM_FAILED){
        return 1;
    }
    return 0;
}
// zatvaranie a vymazovanie semaforov
void delete_semafores(){
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
// spracovanie vstupu od uzivatela
int arg_manage(args_t* args, char ** argv, int argc) {
    if(argc != 5){
        return 1;
    }
    if(argv[1][0]=='\0' || argv[2][0]=='\0'  ||argv[3][0]=='\0'  ||argv[4][0]=='\0'  ){
        return 1;
    }
    // kontrola validity argumentov
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
    if (args->NH == 0 && args->NO == 00){
        return 1;
    }
    return 0;
}
// vlastna funkcia
void my_print( const char * format, ... ){
    sem_wait(print_mutex);
    va_list args;
    va_start (args, format);
    fprintf(f,"%d: ",(*num_proc)++);
    vfprintf (f, format, args);
    va_end (args);
    sem_post(print_mutex);
}
// kyslikovy proces
void oxygen_function(args_t*args,int index){
    my_print("O %d: started\n",index);
    mysleep(args->TI);
    my_print("O %d: going to queue\n",index);
    sem_wait(proc_mutex);
    (*oxy_all)++;
    (*oxy_cnt)++;
    if((*hydr_cnt) >= 2){ // pokial je dost vodikov otvoria sa creating semafory
        sem_post(hydr_sem);
        sem_post(hydr_sem);
        (*hydr_cnt) -=2;
        sem_post(oxy_sem);
        (*oxy_cnt) -=1;
    }
    else{
        sem_post(proc_mutex); // ak nie je dost ide dalsi kyslik do q
    }
    // pokial je viac kyslikov ako dostupnych vodikov printuje sa error
    if (!((*oxy_all) * 2 <= args->NH)) {
        my_print("O %d: not enough H\n",index);
        shared_dest();
        delete_semafores();
        fclose(f);
        exit(0);
    }
    sem_wait(oxy_sem);
    my_print("O %d: creating molecule %d\n",index, (*molecule_count));
    mysleep(args->TB);
    //bariera
    sem_wait(barier_mutex);
        (*barier_count) +=1;
        if ((*barier_count) == 3){
            sem_wait(turnstile2);
            sem_post(turnstile);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile);
    sem_post(turnstile);
    my_print("O %d: molecule %d created\n",index, (*molecule_count));
    sem_wait(barier_mutex);
        (*barier_count) -= 1;
        if((*barier_count)==0){
            sem_wait(turnstile);
            sem_post(turnstile2);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile2);
    sem_post(turnstile2);
    //koniec bariery
    //inkrementovanie molekuly
    (*molecule_count)++;
    sem_post(proc_mutex);

}
//vodikovy proces
void hydrogen_function(args_t*args, int index){
    my_print("H %d: started\n",index);
    mysleep(args->TI);
    my_print("H %d: going to queue\n",index);
    sem_wait(proc_mutex);
    (*hydr_all)++;
    (*hydr_cnt)++;
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
    // kontrola poctu vodikov voci ostatnym vodikom a kyslikom
    if (((*hydr_all) > args->NO * 2 ||
        ((*hydr_all) % 2 != 0 && (*hydr_all) + 1 > args->NH))) {
        my_print("H %d: not enough O or H\n", index);
        shared_dest();
        delete_semafores();
        fclose(f);
        exit(0);
    }
    sem_wait(hydr_sem);
    my_print("H %d: creating molecule %d\n",index, (*molecule_count));
    //bariera
    sem_wait(barier_mutex);
        (*barier_count) += 1;
        if ((*barier_count) == 3){
            sem_wait(turnstile2);
            sem_post(turnstile);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile);
    sem_post(turnstile);
    my_print("H %d: molecule %d created\n",index, (*molecule_count));
    sem_wait(barier_mutex);
        (*barier_count) -=1;
        if((*barier_count)==0){
            sem_wait(turnstile);
            sem_post(turnstile2);
        }
    sem_post(barier_mutex);
    sem_wait(turnstile2);
    sem_post(turnstile2);
    //koniec bariery
}

int main(int argc, char **argv){
    args_t args;
    if (arg_manage(&args, argv, argc)){
        fprintf(stderr, "Wrong argument format!\n");
        return 1;
    }
    f = fopen("proj2.out", "w+");
    // nastavenie printovacieho buffera
    setbuf(f, NULL);
    if(shared_init()){
        fprintf(stderr, "Cannot alocate shared memory!\n");
        return 1;
    }
    if(semafor_init()){
        fprintf(stderr, "Cannot open semaphores!\n");
        return 1;
    }
    // vyravarnie procesov pre kyslik
    for(int i = 1; i<=args.NO;i++){
        pid_t pid = fork();
        if(pid==0){
            oxygen_function(&args, i);
            shared_dest();
            delete_semafores();
            fclose(f);
            exit(0);
        }
        else if (pid==-1){
            fprintf(stderr, "Fork error!\n");
            shared_dest();
            delete_semafores();
            fclose(f);
            exit(1);
        }
    }
    // vytvaranie procesov pre vodik
    for(int i = 1; i<=args.NH;i++){
        pid_t pid = fork();
        if(pid==0){
            hydrogen_function(&args, i);
            delete_semafores();
            shared_dest();
            fclose(f);
            exit(0);
        }
        
        else if (pid==-1){
            fprintf(stderr, "Fork error!\n");
            fclose(f);
            shared_dest();
            delete_semafores();
            exit(1);
        }
    }
    // cakanie na ukoncenie procesov
    while(wait(NULL)>0);
    shared_dest();
    delete_semafores();
    fclose(f);
    return 0;
}
