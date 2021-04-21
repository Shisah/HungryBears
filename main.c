#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>

//struct representing the honey pot, bees produce honey and bears eat from it
struct honeyPot{
  int capacity;
  int honey;
  bool isFull;
  bool isEmpty;
};

//shows the honeyPot values, debug purposes for now
void printPotStats(struct honeyPot *pot){
  printf("%d capacity\n%d level\n%s full\n%s empty\n",
  pot->capacity, 
  pot->honey, 
  pot->isFull ? "true" : "false", 
  pot->isEmpty ? "true" : "false");
}

void printArray(int *arr, int size){
  int i;
  for(i = 0; i < size; i++){
    printf("%d ", arr[i]);
  }
  printf("\n");
}

//indicates if a pid is part of an array
bool contains(pid_t *coll, int size, pid_t value){
  int i;
  for(i = 0; i < size; i++){
    if(coll[i] == value){
      return true;
      break;
    }
  }
  return false;
}
//updates the variables of the honeypot adding 1 unit of honey
void addHoney(struct honeyPot *pot){
  pot -> honey++;
  pot -> isFull = pot -> capacity == pot -> honey;
  pot -> isEmpty = pot -> honey == 0;
}

//updates the variables of the honeypot removing 1 unit of honey
void takeHoney(struct honeyPot *pot){
  pot -> honey--;
  pot -> isFull = pot -> capacity == pot -> honey;
  pot -> isEmpty = pot -> honey == 0;
}

//a producer is a bee, it makes honey unless the pot is full
void beeHarvest(struct honeyPot *pot, sem_t *potFullMutex, sem_t *bearSleepMutex, sem_t *potWriteMutex){
  struct timeval seed;
  gettimeofday(&seed, NULL);
  srand(seed.tv_usec);
  while(true){
    sem_wait(potWriteMutex);
    sem_wait(potFullMutex);
    addHoney(pot);
    printf("pot at: %d bee %d produced honey\n",pot->honey, getpid());
    //int value; 
    //sem_getvalue(potFullMutex, &value); 
    //printf("The value of the semaphors is %d\n", value);
    sem_post(potWriteMutex);
    usleep(rand()%4000000);
  }
}

void EATER_TEST(struct honeyPot *pot, sem_t *potFullMutex, sem_t *bearSleepMutex, sem_t *potWriteMutex){
  while(true){
    // if(! pot->isFull){
    //   printf("locking bear %d\n", getpid());
    //   sem_wait(bearSleepMutex);
    //   //printf("locking bear %d\n", getpid());
    //   //sem_post(potFullMutex);
    // }
    // else{
    //   takeHoney(pot);
    //   printf("pot at: %d bear %d ate honey\n",pot->honey, getpid());
    //   sleep(1);
    // }
    sem_wait(potWriteMutex);
    sem_post(potFullMutex);
    takeHoney(pot);
    printf("pot at: %d bear %d ate honey\n",pot->honey, getpid());
    sem_post(potWriteMutex);
    sleep(1);
  }
}

int main(int argc , char *argv[] ) {

  int beeCount = atoi(argv[1]);
  int bearCount =  atoi(argv[2]);

  struct honeyPot *honeyBuffer = mmap(NULL, sizeof(struct honeyPot), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  honeyBuffer -> capacity = atoi (argv[3]);
  honeyBuffer -> isFull = honeyBuffer -> capacity == honeyBuffer -> honey;
  honeyBuffer -> isEmpty = honeyBuffer -> honey == 0;

  //semaphore for stopping bees after reaching a full pot
  // sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  //sem_t potFullMutex;
  sem_t bearSleepMutex;
  //sem_t potSyncMutex;
  sem_t *potSyncMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_init(potFullMutex, 1, 30);
  sem_init(&bearSleepMutex, 1 ,0);
  sem_init(potSyncMutex, 1 ,1);
  
  //2 shared arrays with the PIDs of the processes, used to branch their work
  // pid_t *bees = mmap(NULL, sizeof(pid_t)*beeCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  // pid_t *bears = mmap(NULL, sizeof(pid_t)*bearCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  int i;
  pid_t pid, wpid;
  //pid_t parentID = getpid();

  int status = 0;

  //make n bees and M bears
  for(i = 0; i < beeCount + bearCount; i++){
    pid = fork();
    if(pid < 0) {
      printf("Error\n");
      exit(1);
    }
    else if (pid == 0){
      if(i < beeCount){
        //bees[i] = getpid();
        beeHarvest(honeyBuffer, potFullMutex, &bearSleepMutex, potSyncMutex);
      }
      else{
        //bears[i - beeCount] = getpid();
        EATER_TEST(honeyBuffer, potFullMutex, &bearSleepMutex, potSyncMutex);
      }
      exit(0);
    }
  }

  while ((wpid = wait(&status)) > 0);

  // printArray(bees, beeCount);
  // printArray(bears, bearCount);

  // //if the pot is full, bees stop, if the pot have space, bees harvest
  // sem_t *goHarvest = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // //if the pot is empty, bears wait
  // sem_t *potEmtpyMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // //every bear has to have ate at least 1 portion of honey before letting everyone loose
  // sem_t *everyoneAte = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  //printPotStats(honeyBuffer);
}