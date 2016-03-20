#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/mman.h>
#include <sys/uio.h>
#include <pthread.h>
#include <time.h>

#define ADDRESS "239.255.24.25"
#define PORT 23456

struct Entry {
  char * username;
  char * address;
};

struct Params {
    std::string username;
    int s;
    struct sockaddr_in myaddr;
    struct Entry Table[100];
};


void *anounce(void *input)
{
  struct Params *params = (struct Params*)input;
   std::string temp = "Anounce " + params->username;
   char* message = strdup((temp).c_str());
   struct timespec tim;
   tim.tv_sec = 5;
   tim.tv_nsec = 0;
   int send_len = strlen(message);
    while(1){
      if ((sendto(params->s, message, send_len, 0, (struct sockaddr *) &params->myaddr, sizeof(params->myaddr))) != send_len){
        perror("Error in number of bytes");
        exit(1);
      }
      std::cout << "Sent message. Waiting 60 seconds. My username is " << params->username << std::endl;

      if(nanosleep(&tim , NULL) < 0 )
     {
        printf("Nano sleep system call failed \n");
     }
 }

   pthread_exit(NULL);
}

void *read(void *input){
  struct Params *params = (struct Params*)input;
  int MSGBUFSIZE = 256;
  char msgbuf[MSGBUFSIZE];
  int nbytes;
  socklen_t addrlen=sizeof(params->myaddr);

  while (1) {
    addrlen=sizeof(params->myaddr);
    if ((nbytes=recvfrom(params->s,msgbuf,MSGBUFSIZE,0,(struct sockaddr *) &params->myaddr,&addrlen)) < 0) {
        perror("recvfrom");
        exit(1);
    }
    msgbuf[nbytes] = 0;
    std::cout << "message:" << msgbuf << std::endl;
  }
  pthread_exit(NULL);
}

int main(int argc , char *argv[])
{

  if (argc !=  2){
    std::cout << "bad arguments" << std::endl;
    return 1;
  }
  std::string username = argv[1];
  int s;
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("cannot create socket");
	return 0;
}
  struct sockaddr_in myaddr;
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = inet_addr(ADDRESS);
  myaddr.sin_port = htons(PORT);

  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr=inet_addr(ADDRESS);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  int reuse = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
        fprintf(stderr, "setsockopt: %d\n", errno);
        return 1;
    }
  setsockopt(s,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));


  if (bind(s, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
  	perror("bind failed");
  	return 0;
  }


  pthread_t anouncer;
  pthread_t reader;
  int rc;
  struct Params params;
  params.username = username;
  params.s = s;
  params.myaddr = myaddr;
  rc = pthread_create(&anouncer, NULL, anounce, &params);
  if (rc){
   std::cout << "Error:unable to create thread," << rc << std::endl;
   exit(-1);
}
  rc = pthread_create(&reader, NULL, read, &params);
  if (rc){
   std::cout << "Error:unable to create thread," << rc << std::endl;
   exit(-1);
  }

  void* status;
  rc = pthread_join(anouncer, &status);
        if (rc){
           std::cout << "Error:unable to join," << rc << std::endl;
           exit(-1);
        }
  rc = pthread_join(reader, &status);
        if (rc){
           std::cout << "Error:unable to join," << rc << std::endl;
           exit(-1);
        }
        std::cout << "main thread ended" << std::endl;

 }
