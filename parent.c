#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sockets.h"
#include "bloom.h"
#include "requests.h"
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char* argv[]){
  int i=0, numMonitors, bufferSize, bloomSize, inputDir,cyclicBuffer,numThreads,connection;
  int *socketfds,status,maxfd=0,j,t,k;
  pid_t childid;
  fd_set fds;
  char *day,*month,*year,**arr;
  char buffer2[50];
  char str[20],str2[200];
  struct timeval tv;
  int retval;
  int sizeofMsg,n;
  unsigned int **bloomFilters;
  bloomNode *headBloom = NULL;
  countryMonitor *headCountry = NULL;
  stats *headStats = NULL;
  char hostbuffer[256];
  int hostname,port;
  struct hostent *host;
  struct sockaddr_in  server;
  struct sockaddr *serverptr = (struct sockaddr *)&server;
  srand(time(0));

  if(argc!=13){
    fprintf(stderr, "Not enough arguments\n" );
    exit(2);
  }

  while (i<argc )
  {
      if(strcmp(argv[i],"-s")==0){
          bloomSize = atoi(argv[i+1]);
      }
      else if (strcmp(argv[i],"-i")==0){
          inputDir=i+1;
      }
      else if (strcmp(argv[i],"-b")==0){
        bufferSize = atoi(argv[i+1]);
      }
      else if (strcmp(argv[i],"-m")==0){
        numMonitors = atoi(argv[i+1]);
      }
      else if (strcmp(argv[i],"-c")==0){
        cyclicBuffer = atoi(argv[i+1]);
      }
      else if (strcmp(argv[i],"-t")==0){
        numThreads = atoi(argv[i+1]);
      }
      i++;
   }



  char buffer[bufferSize];

  //find ip
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
  port = rand() % (65435 - 49152 + 1) + 49152; //choose a port number

    char *IPbuffer;
    IPbuffer = inet_ntoa(*((struct in_addr*) host->h_addr_list[0]));

    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);

  socketfds = (int*)malloc(numMonitors * sizeof(int)); //fds to write the parent
  if (socketfds == NULL) {
      perror("Memory not allocated.\n");
      exit(2);
  }

  divideCountries(numMonitors,argv[inputDir],&headCountry);


  for(i=0;i<numMonitors;i++){

    childid=fork();
    if(childid == 0){ //in child
        break;
    }
    else if( childid ==-1){
      perror("Error with fork");
      exit(2);
    }
    else{ //in parent create sockets

      if((socketfds[i] = socket(PF_INET , SOCK_STREAM , 0)) < 0){
        perror("Error with socket");
        exit(2);
      }
      server.sin_family = AF_INET;
      memcpy (&server.sin_addr , host->h_addr , host->h_length);
      printf("PORT = %d\n",port );
      server.sin_port = htons(port);
      do{
        connection = connect(socketfds[i] , serverptr ,sizeof(server));
      }while(connection<0);


    }

      port++;
  }


  if(childid == 0){ //in child
    arr = (char**) malloc(sizeof(char*) * 11 );
    countryMonitor* current = headCountry;
    arr[0] = (char*) malloc(sizeof(char)*(strlen("./monitorServer")+1));
    strcpy(arr[0],"./monitorServer");
    arr[1] = (char*) malloc(sizeof(char)*3);
    strcpy(arr[1],"-p");
    sprintf(str, "%d", port);
    arr[2] = (char*) malloc(sizeof(char)*(strlen(str)+1));
    strcpy(arr[2],str);
    arr[3] = (char*) malloc(sizeof(char)*3);
    strcpy(arr[3],"-t");
    sprintf(str, "%d", numThreads);
    arr[4] = (char*) malloc(sizeof(char)*(strlen(str)+1));
    strcpy(arr[4],str);
    arr[5] = (char*) malloc(sizeof(char)*3);
    strcpy(arr[5],"-b");
    sprintf(str, "%d", bufferSize);
    arr[6] = (char*) malloc(sizeof(char)*(strlen(str)+1));
    strcpy(arr[6],str);
    arr[7] = (char*) malloc(sizeof(char)*3);
    strcpy(arr[7],"-c");
    sprintf(str, "%d", cyclicBuffer);
    arr[8] = (char*) malloc(sizeof(char)*(strlen(str)+1));
    strcpy(arr[8],str);
    arr[9] = (char*) malloc(sizeof(char)*3);
    strcpy(arr[9],"-s");
    sprintf(str, "%d", bloomSize);
    arr[10] = (char*) malloc(sizeof(char)*(strlen(str)+1));
    strcpy(arr[10],str);

    k = 0;

    while(current != NULL){
      if(current->monitor == i){
        k++;
        arr = (char**) realloc (arr,sizeof(char*)*(11+k));
        strcpy(str2,argv[inputDir]);
        strcat(str2,"/");
        strcat(str2,current->name);
        arr[10+k] = (char*) malloc(sizeof(char)*(strlen(str2)+1));

        strcpy(arr[10+k],str2);
      }
      current = current->next;
    }
    k++;
    arr = (char**) realloc (arr,sizeof(char*)*(11+k));
    arr[10+k] = NULL;

    execv("./monitorServer", arr);
  }


  //in parent
  //reading bloom filters
  int mo[numMonitors];
  for(i=0;i<numMonitors;i++){
    mo[i] = 0;
  }
  int flag = 0;
  while(flag==0){
    FD_ZERO(&fds);

    maxfd = -1;
    for(j=0; j<numMonitors; j++){

      if(mo[j]==0){  //if bloom of child i not read

        if(maxfd<socketfds[j]){
          maxfd = socketfds[j];
        }
        FD_SET(socketfds[j], &fds);
      }
    }

    retval = select(maxfd + 1, &fds, NULL, NULL, NULL);
    if(retval == -1){
      perror("Error with select");
    }
    else if(retval >0){
      for(i=0; i<numMonitors; i++){
       if(FD_ISSET(socketfds[i], &fds)){ //find in which pipe

          readBloomFilters(bufferSize,bloomSize,&headBloom,socketfds[i]);
          mo[i] = 1;
          break;
        }
      }
    }
    flag = 1;
    for(j=0; j<numMonitors; j++){
      if(mo[j]==0){  //if bloom of child i not read

        flag =0;
      }
    }
  }


  //waiting for commands
  char *buffer1=NULL,*word;
  char id[15],date[20],countryFrom[50],countryTo[50],virus[25],date2[20];
  size_t size3=0;

   while(1){

      printf("waiting for requests\n" );

      getline(&buffer1,&size3,stdin);
      word = strtok(buffer1, " ");

      if(strcmp(word,"/travelRequest")==0){

        if((word = strtok(NULL, " "))!=NULL){
          strcpy(id,word);
          if((word = strtok(NULL, " "))!=NULL){
            strcpy(date,word);
            if((word = strtok(NULL, " "))!=NULL){
              strcpy(countryFrom,word);
              if((word = strtok(NULL, " "))!=NULL){
                strcpy(countryTo,word);
                if((word = strtok(NULL, " "))!=NULL){
                  strcpy(virus,word);
                  virus[strlen(virus)-1]='\0';

                  if(travelRequest(socketfds,socketfds,headBloom,headCountry,id,date,countryFrom,countryTo,virus,bloomSize,bufferSize)==-1){
                    insertStat(countryTo,&headStats,virus,1,date);

                  }else{
                    insertStat(countryTo,&headStats,virus,0,date);

                  }
                }
              }
            }
          }
        }
     }else if(strcmp(word,"/exit\n")==0){
        free(buffer1);
        ExitParent(&headStats,argv[inputDir],numMonitors,socketfds,&headBloom,&headCountry,bufferSize);

      }else if(strcmp(word,"/travelStats")==0){
        if((word = strtok(NULL, " "))!=NULL){
          strcpy(virus,word);
          if((word = strtok(NULL, " "))!=NULL){
            strcpy(date,word);
            if((word = strtok(NULL, " "))!=NULL){
              strcpy(date2,word);
              if((word = strtok(NULL, " "))!=NULL){
                strcpy(countryTo,word);
                countryTo[strlen(countryTo)-1]='\0';
                printStats(virus,date,date2,countryTo,headStats);

              }else{
                date2[strlen(date2)-1]='\0';
                printStats(virus,date,date2,NULL,headStats);

              }
            }
          }
        }
      }else if(strcmp(word,"/searchVaccinationStatus")==0){
        if((word = strtok(NULL, " "))!=NULL){
          strcpy(id,word);
          id[strlen(id)-1]='\0';

          searchVaccination(id,socketfds,socketfds,bufferSize,numMonitors);
        }
      }



  }
}
