#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <errno.h>
#include <map>
#define ADDRESS "239.255.24.25"
#define PORT 23456
#define PORT2 23457

std::map <std::string, std::string> table;

struct Params {
    char* username;
    int s;
    struct sockaddr_in myaddr;
};


void *anounce(void *input)
{
  struct Params *params = (struct Params*)input;
  std::string username(params->username);
   std::string message = "ANNOUNCE " + username;
   struct timespec tim;
   tim.tv_sec = 60;
   tim.tv_nsec = 0;
    while(1){
      if ((sendto(params->s, message.c_str(), message.size(), 0, (struct sockaddr *) &params->myaddr, sizeof(params->myaddr))) < 0){
        perror("Error in number of bytes");
        exit(1);
      }
      //std::cout << "Sent message. Waiting 60 seconds. My username is " << params->username << std::endl;

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
  int s = params->s;
  struct sockaddr_in addr = params->myaddr;
  socklen_t addrlen=sizeof(params->myaddr);

  while (1) {
    addrlen=sizeof(addr);
    if ((nbytes=recvfrom(s,msgbuf,MSGBUFSIZE,0,(struct sockaddr *) &addr  ,&addrlen)) < 0) {
        perror("recvfrom");
        exit(1);
    }
    msgbuf[nbytes] = 0;
    if(strncmp(msgbuf,"ANNOUNCE ",9) == 0){
      char name[nbytes - 8];
      memcpy( name, &msgbuf[9], nbytes - 9 );
      name[nbytes - 8] = '\0';
      std::string temp_addr(inet_ntoa(addr.sin_addr));
      std::string temp_name(name);
      table[temp_name] = temp_addr;
      std::cout << "announce: " << name << std::endl;

    }
    else {
    std::cout << "message: " << msgbuf << std::endl;
  }
  }
  pthread_exit(NULL);
}

void *privateread(void *input){
  struct sockaddr_in si_me, si_other;
  int s;
  socklen_t slen=sizeof(si_other);
  int BUFLEN = 256;
  char buf[BUFLEN];
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    perror("cannot create socket");
  int reuse = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
        fprintf(stderr, "setsockopt: %d\n", errno);
        return 0;
    }
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(PORT2);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (struct sockaddr *) &si_me, sizeof(si_me))==-1)
    perror("cannot bind");
  if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)==-1)
    perror("error receiving");
  printf("Received packet from %s:%d\nData: %s\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

  return 0;
}

int main(int argc , char *argv[])
{

  if (argc !=  2){
    std::cout << "bad arguments" << std::endl;
    return 1;
  }
  char* username = argv[1];
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
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
        fprintf(stderr, "setsockopt: %d\n", errno);
        return 1;
    }


  if (bind(s, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
  	perror("bind failed");
  	return 0;
  }
  if(setsockopt(s,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0){
    std::cout << "error" << std::endl;
  }


  pthread_t anouncer;
  pthread_t reader;
  pthread_t privatereader;
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
  rc = pthread_create(&privatereader, NULL, privateread, &params);
  if (rc){
   std::cout << "Error:unable to create thread," << rc << std::endl;
   exit(-1);
  }
  const char* prefix = "FROM:";
  const char* suffix = " ";
  while(1){
    char input[256];
    std::cin.getline (input,256);
    char* p = input;
    int newsize = sizeof(input) + 5 + sizeof(username) + 1;
    char newmessage[newsize];
    char * p2 = newmessage;
    strcpy(p2,prefix);
    strcat(p2,username);
    strcat(p2,suffix);
    strcat(p2,p);
    std::cout << p2 << std::endl;
    if(strncmp(p,"/",1) == 0){
      //private message

      //parse username
      char * pch;
      pch = strtok(p," ");
      char name[sizeof(pch) - 1];
      memcpy( name, &pch[1], sizeof(pch) - 1 );
      std::string temp_name(name);
      std::map<std::string, std::string>::iterator it = table.find(temp_name);
      if (it == table.end())
        std::cout << "Not in table" << std::endl;
      else
      {
        //if username in db
        //send p2 to address
        struct sockaddr_in si_other;
        int s;
        socklen_t slen=sizeof(si_other);
        int buflen = 256;
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
          std::cout << "Socket error" << std::endl;
        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(PORT2);
        if (inet_aton(it->second.c_str(), &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
        }
        if (sendto(s, p2, buflen, 0, (struct sockaddr *) &si_other, slen)==-1)
          std::cout << "send error" << std::endl;

    }
    }
    else {
      if ((sendto(s, p2, newsize, 0, (struct sockaddr *) &params.myaddr, sizeof(params.myaddr))) < 0){
          perror("Error in number of bytes");
          exit(1);
        }
    }
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
  rc = pthread_join(privatereader, &status);
        if (rc){
           std::cout << "Error:unable to join," << rc << std::endl;
           exit(-1);
        }
        std::cout << "main thread ended" << std::endl;

 }
