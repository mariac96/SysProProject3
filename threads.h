#ifndef THREADS_H_
#define THREADS_H_

#include <pthread.h>
#include <unistd.h>
#include "skiplist.h"

int num_of_items;
int length,length2,length3;
datenode*headDate; //list fot the dates
pthread_mutex_t mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
int cyclicBuffer;
typedef struct cBuffer{
  char** buffer;
  int start;
  int end;
  int count;
} cBuffer;
cBuffer cbuffer;

void initializeCbuffer(cBuffer *cbuffer,int cyclicBuffer);
void *  consumer(void *l);
#endif
