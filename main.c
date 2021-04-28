#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>

#define BEAR_RAND_SLEEP 4000000
#define BEAR_MAX_HUNGER 10
#define BEE_RAND_SLEEP 4000000

//struct holding the memory shared variables
struct honeyPot{
  int capacity;
  int honey;
  int bearCount;
  int beeCount;
  bool isFull;
  bool isEmpty;
  int cycleCount;
};

//shows the honeyPot values
void printPotStats(struct honeyPot *pot){
  printf("%3d capacity\n%3d bears\n%3d bees\n\n",
  pot->capacity, 
  pot-> bearCount, 
  pot-> beeCount);
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
//once the semaphore potFullMutex is locked, the last bee unlocks capacity slots for
//the bears to eat
void bee(struct honeyPot *pot, sem_t *potFullMutex, sem_t *bearSleepMutex, sem_t *potWriteMutex){
  struct timeval seed;
  gettimeofday(&seed, NULL);
  srand(seed.tv_usec);
  while(true){
    sem_wait(potFullMutex);
    usleep(rand()%BEE_RAND_SLEEP);
    sem_wait(potWriteMutex);
    if(pot -> bearCount <= 0){
        printf("stopping bee\n");
        sem_post(potWriteMutex);
        sem_post(potFullMutex);
        exit(0);
    }
    addHoney(pot);
    printf("honey level: %3d bee %3d produced honey\n",pot->honey, getpid());
    if(pot -> isFull){
      printf("\npot is full, bee %3d wakes the bears\n\n", getpid());
      int i;
      for(i = 0; i < pot -> capacity; i++){
        sem_post(bearSleepMutex);//repeat capacity times
      }
    }
    sem_post(potWriteMutex);
  }
}

void bear(struct honeyPot *pot, sem_t *potFullMutex, sem_t *bearSleepMutex, sem_t *potWriteMutex){
  struct timeval seed;
  gettimeofday(&seed, NULL);
  srand(seed.tv_usec);
  //make bear had a random between 1 and BEAR_MAX_HUNGER level of hunger
  int hunger = (rand() % BEAR_MAX_HUNGER) + 1;
  int ate = 0;
  while(true){
    sem_wait(bearSleepMutex);
    usleep(rand()%BEAR_RAND_SLEEP);
    sem_wait(potWriteMutex);
    takeHoney(pot);
    //sem_post(potFullMutex); //post to release bees everytime a portion is eaten
    ate++;
    printf("honey level: %3d; bear %d ate honey; eaten %3d\n",pot->honey, getpid(), ate);
    //frees up capacity slots for bees to refill the pot
    if(pot -> isEmpty){
      pot -> cycleCount++;
      printf("\npot is empty, bear %3d wakes the bees\n\n", getpid());
      int i;
      for(i = 0; i < pot -> capacity; i++){
        sem_post(potFullMutex);//repeat capacity times
      }
    }
    sem_post(potWriteMutex);
    if bear is full, exit
    if(hunger == ate){
      pot -> bearCount--;
      printf("bear %3d is full, %3d bears remain hungry\n", getpid(), pot -> bearCount);
      if(pot -> bearCount == 0){
        int i;
        for(i = 0; i < pot -> capacity; i++){
          sem_post(potFullMutex); //repeat capacity times
        }
      }
      exit(0);
    }
    usleep(rand()%BEAR_RAND_SLEEP);
  }
}

int main(int argc , char *argv[] ) {

  

  struct honeyPot *honeyBuffer = mmap(NULL, sizeof(struct honeyPot), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  honeyBuffer -> capacity = atoi (argv[3]);
  honeyBuffer -> isFull = honeyBuffer -> capacity == honeyBuffer -> honey;
  honeyBuffer -> isEmpty = honeyBuffer -> honey == 0;
  honeyBuffer -> beeCount = atoi(argv[1]);
  honeyBuffer -> bearCount =  atoi(argv[2]);
  honeyBuffer -> cycleCount = 1; //bees counts like humans

  if(honeyBuffer -> capacity < honeyBuffer -> bearCount){
    printf("the number of bears must be equal or lower than the capacity of the honey pot\n");
    exit(1);
  }

  //semaphore for stopping bees after reaching a full pot
  // sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_t *bearSleepMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_t *potSyncMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sem_init(potFullMutex, 1, honeyBuffer -> capacity);
  sem_init(bearSleepMutex, 1 ,0);
  sem_init(potSyncMutex, 1 ,1);
  
  int i;
  pid_t pid, wpid;
  int status = 0;
  //make n bees and M bears
  for(i = 0; i < honeyBuffer -> beeCount + honeyBuffer -> bearCount; i++){
    pid = fork();
    //fork error exits the program
    if(pid < 0) {
      printf("Error\n");
      exit(1);
    }
    //successful fork creates n bees and then M bears and passes the mutex
    else if (pid == 0){
      if(i < honeyBuffer -> beeCount){
        bee(honeyBuffer, potFullMutex, bearSleepMutex, potSyncMutex);
      }
      else{
        bear(honeyBuffer, potFullMutex, bearSleepMutex, potSyncMutex);
      }
      exit(0);
    }
  }

  while ((wpid = wait(&status)) > 0);

}