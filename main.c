#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hashing.h"
#include "sockets.h"
#include "functions.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include "requests.h"
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include "threads.h"
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

int main(int argc, char *argv[]){
  int i=1,recordsfile=-1,lines=0,flag,numCountries=0,j,k,err,flag2;
  int age,port;
  int retval,bufferSize;
  fd_set fds;
  FILE *fp;
  char *buffer=NULL,*word,id[15],virus[25],first[15],last[15],country[50],answer[10],date[20],*day,*month,*year,**files,**buffer3;
  size_t size=0;
  srand(time(0));
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  struct dirent *dir;
  DIR *subDir;
  stats *headStats = NULL;
  char hostbuffer[256];
  char buffer5[200];
  int hostname;
  struct hostent *host;
  int socketfd,fd,numThreads;
  int reuse_addr = 1;
  socklen_t  clientlen;
  struct sockaddr_in  server,client;
  struct sockaddr *serverptr = (struct sockaddr *)&server;
  struct sockaddr *clientptr =(struct sockaddr  *)&client;


  for(k=0;k<11;k++){
    if(strcmp(argv[k],"-s")==0){
        bloomsize = atoi(argv[k+1]);
    }
    else if (strcmp(argv[k],"-p")==0){
        port = atoi(argv[k+1]);
    }
    else if (strcmp(argv[k],"-b")==0){
      bufferSize = atoi(argv[k+1]);
    }
    else if (strcmp(argv[k],"-t")==0){
      numThreads = atoi(argv[k+1]);
    }
    else if (strcmp(argv[k],"-c")==0){
      cyclicBuffer = atoi(argv[k+1]);
    }
  }


  files = (char**) malloc(sizeof(char*)); //to save the paths
  for(k=11;k<argc;k++){

    files[k-11] = (char*) malloc(sizeof(char)*(strlen(argv[k])+1));
    strcpy(files[k-11],argv[k]);

    files = (char**) realloc (files,sizeof(char*)*(k-9));

  }
  numCountries = k-11;

  hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  if(hostname == -1){
    perror("Error with gethostname");
    exit(2);
  }
  host = gethostbyname(hostbuffer);
  if(host == NULL){
    perror("Error with gethostbyname");
    exit(2);
  }

  char *IPbuffer;
  IPbuffer = inet_ntoa(*((struct in_addr*) host->h_addr_list[0]));

  /*printf("CHILD Hostname: %s\n", hostbuffer);
  printf("CHILD Host IP: %s port %d\n", IPbuffer,port);
*/
  if((socketfd = socket(PF_INET , SOCK_STREAM , 0)) < 0){
    perror("Error with socket");
    exit(2);
  }

  server.sin_family = AF_INET;
  memcpy (&server.sin_addr , host->h_addr , host->h_length);

  server.sin_port = htons(port);
  if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&reuse_addr, sizeof(reuse_addr))<0){
    perror("Error with setsockopt");
    exit(2);
  }

  if(bind(socketfd , serverptr ,sizeof(server)) < 0){
    perror("Error with bind");
    exit(2);
  }

  if(listen(socketfd , 5) < 0)  {
    perror("Error with listen");
    exit(2);
  }

  clientlen = sizeof(client);
  if(( fd = accept(socketfd , clientptr , &clientlen)) < 0) {
    perror("Error with accept");
    exit(2);
  }



  num_of_items = 0;
  headDate = NULL;
  for(i=0;i<numCountries;i++){

    subDir = opendir(files[i]);
    if(subDir==NULL){
      perror( "Error opening directory\n");
      exit(2);
    }
    while((dir = readdir(subDir))!= NULL){

      if(strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0){
          num_of_items++; //number of files
          char* buffer4 = malloc(sizeof(char) *(strlen(files[i]) + strlen(dir->d_name)+2));
          strcpy(buffer4,files[i]);
          strcat(buffer4,"/");
          strcat(buffer4,dir->d_name);

          fp = fopen(buffer4,"r");
          if(fp==NULL){
            perror( "Error opening file\n");
            exit(2);
          }

          while(getline(&buffer,&size,fp)!=-1){
            lines++;  //count the lines in the files
          }
          free(buffer4);
          fclose(fp);
      }
    }

    closedir(subDir);

  }

  free(buffer);
  if(lines <5){
    lines = 10;
  }
  //initialize citizen hashtable, country hashtable and virus hashtable
  length = initialize(lines,0);
  length2= initialize(200,1);
  length3= initialize(30,2);
  char* buffer4[num_of_items];
  j=0;
  for(i=0;i<numCountries;i++){


    subDir = opendir(files[i]);
    if(subDir==NULL){
      perror( "Error opening directory\n");
      exit(2);
    }
    while((dir = readdir(subDir))!= NULL){

      if(strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0){
          buffer4[j] = malloc(sizeof(char) *(strlen(files[i]) + strlen(dir->d_name)+2));
          strcpy(buffer4[j],files[i]);
          strcat(buffer4[j],"/");
          strcat(buffer4[j],dir->d_name);

          j++;

      }
    }
    closedir(subDir);
  }

  initializeCbuffer(&cbuffer,cyclicBuffer);
  pthread_mutex_init(&mtx, 0);
  pthread_cond_init(&cond_nonempty, 0);
  pthread_cond_init(&cond_nonfull, 0);
  pthread_t *tids = malloc(numThreads * sizeof(pthread_t));
  if ( tids  == NULL ) {
    perror (" malloc ");
    exit (1) ;
  }
//insert the data from the files

  flag = 0;
  flag2 = 0;
  int numItems = num_of_items;
  i=0;
  while (num_of_items > 0){
    if(flag == 1){ //if threads created
      pthread_mutex_lock(&mtx);
      while (cbuffer.count >= cyclicBuffer) {  //if buffer full
        printf(">> Found Buffer Full \n");
        pthread_cond_wait(&cond_nonfull, &mtx);
      }
    }

    cbuffer.end = (cbuffer.end + 1) % cyclicBuffer;
    cbuffer.buffer[cbuffer.end] = malloc(sizeof(char) *(strlen(buffer4[i]) +1));
    strcpy(cbuffer.buffer[cbuffer.end],buffer4[i]);

    printf("PUT IN BUFFER %s\n", cbuffer.buffer[cbuffer.end]);
    cbuffer.count++;
    if(flag == 1){
      pthread_mutex_unlock(&mtx);
    }
    num_of_items--;
    i++;
    if(cbuffer.count == cyclicBuffer && flag == 0){ //if buffer full creat threads
      flag =1;
      for(j=0;j<numThreads;j++){
        if (err = pthread_create(tids+j, NULL, consumer, NULL)) {
          perror2("pthread_create", err);
          exit(1);
        }
        printf("CREATED THREADS %ld i =%d\n",(long)tids+j ,j);
      }

    }

    if(flag == 1){
      //printf("ABOUT TO SIGNAL cond_nonempty\n" );
      pthread_cond_broadcast(&cond_nonempty);
    }

  }


  for(i=0;i<numThreads;i++){

    if (err = pthread_join(*(tids+i), NULL)) {
      perror2("pthread_join", err);
      exit(1);
    }
  }


  printf("DONE WAITING FOR THREADS\n" );

  sendBloom(bufferSize,length3,fd);
  char countryTo[50],countryFrom[50],dateV[20];
  while(2){

    readPipe(bufferSize,0,0,fd,buffer5);

    if(strcmp(buffer5,"REQUEST")==0){

      readPipe(bufferSize,0,0,fd,id);
      readPipe(bufferSize,0,0,fd,date);
      readPipe(bufferSize,0,0,fd,countryFrom);
      readPipe(bufferSize,0,0,fd,countryTo);
      readPipe(bufferSize,0,0,fd,virus);

      if(vaccineStatusVirus(id,virus,length3,dateV,countryFrom)==0){

          if(compareDates(date,dateV)==0){
            insertStat(countryTo,&headStats,virus,1,date);
          }else{
            insertStat(countryTo,&headStats,virus,0,date);
          }

          writePipe(bufferSize,fd,"YES",NULL,0,0);
          writePipe(bufferSize,fd,dateV,NULL,0,0);
      }else{
        writePipe(bufferSize,fd,"NO",NULL,0,0);
        insertStat(countryTo,&headStats,virus,1,date);
      }
    }else if(strcmp(buffer5,"STATUS")==0){
      readPipe(bufferSize,0,0,fd,id);

      vaccineStatus(id,length3,fd,bufferSize);

    }else if(strcmp(buffer5,"EXIT")==0){
      pthread_cond_destroy(&cond_nonempty);
      pthread_cond_destroy(&cond_nonfull);
      pthread_mutex_destroy(&mtx);
      free(tids);
      free(cbuffer.buffer);
      for(i=0;i<numItems;i++){
        free(buffer4[i]);
      }
      ExitChild(files,socketfd,fd,length,length2,length3,&headDate,&headStats,argc);
    }

  }

}
