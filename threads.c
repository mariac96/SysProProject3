#include "threads.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "functions.h"

void initializeCbuffer(cBuffer *cbuffer,int cyclicBuffer) {
    cbuffer->start = 0;
    cbuffer->end = -1;
    cbuffer->count = 0;
    cbuffer->buffer = (char**) malloc(sizeof(char*)*cyclicBuffer);
    if(cbuffer->buffer == NULL){
      perror("Error with malloc");
    }
}


void * consumer(void *l)
{

    while (num_of_items > 0 || cbuffer.count > 0){ //not every file read
      pthread_mutex_lock(&mtx);
      while (cbuffer.count <= 0 && num_of_items > 0 ) { //buffer empty but producer hasn't finished
      //  printf(">> Found Buffer Empty thread  %ld\n",pthread_self());
        pthread_cond_wait(&cond_nonempty, &mtx);
      }
      if( cbuffer.count > 0){

        readfile(cbuffer.buffer[cbuffer.start],length,length2,length3,&headDate);
        printf ("READ from cyclicBuffer %s thread %ld\n",cbuffer.buffer[cbuffer.start],(long) pthread_self());
        free(cbuffer.buffer[cbuffer.start]);
        cbuffer.start = (cbuffer.start + 1) % cyclicBuffer;
        cbuffer.count--;

        pthread_mutex_unlock(&mtx);
        pthread_cond_signal(&cond_nonfull);
      }else{

        pthread_mutex_unlock(&mtx);
      }

    }
  printf("ABOUT TO EXIT thread %ld\n",pthread_self());
  pthread_exit(0);

}
