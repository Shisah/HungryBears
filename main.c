#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

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
void beeHarvest(struct honeyPot *pot){

}

//parse arguments and initializes the honeyPot, bees and bears
void setup(struct honeyPot *pot){

}

int main(void) {

  struct honeyPot *honeyBuffer = mmap(NULL, sizeof(struct honeyPot), PROT_READ | PROT_WRITE, MAP_SHARED |    MAP_ANONYMOUS, -1, 0);

  //if the pot is full, bees stop, if the pot have space, bees harvest
  sem_t *goHarvest = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  sem_t *potFullMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  //if the pot is empty, bears wait
  sem_t *potEmtpyMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  //every bear has to have ate at least 1 portion of honey before letting everyone loose
  sem_t *everyoneAte = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  printPotStats(honeyBuffer);

  return 0;
}