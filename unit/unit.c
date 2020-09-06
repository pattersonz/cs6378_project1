#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <pthread.h>
#include "unitStructs.h"

us thisId;
us nCount;
us thisPort;
NEIGHBOR* ns;
us foundNum;
us N;
us curRound;

VEC_ECC* vec;
pthread_mutex_t vecLock; 

void *contactOrigin(void* ptr);
void *recMsg(void* ptr);
void handleMsg(int sock);
void *sendMsg(void* ptr);
int main()
{
  vec = NULL;
  pthread_t originThread;
  pthread_create(&originThread, NULL, &contactOrigin, NULL);
  pthread_join(originThread, NULL);
  return -1;
} 

void *contactOrigin(void* ptr)
{
  int server_fd, new_socket, valread; 
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
  byte buf[1024] = {0}; 
      
  // Creating socket file descriptor 
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
  { 
	perror("socket failed"); 
	return (void*)-1; 
  } 
       
  // Forcefully attaching socket to the port 8080 
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
				 &opt, sizeof(opt))) 
  { 
	perror("setsockopt"); 
	return (void*)-1; 
  } 
  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons( ORIGIN_PORT ); 
       
  // Forcefully attaching socket to the port 8080 
  if (bind(server_fd, (struct sockaddr *)&address,  
		   sizeof(address))<0) 
  { 
	perror("bind failed"); 
	return (void*)-1;
  } 
  if (listen(server_fd, 3) < 0) 
  { 
	perror("listen"); 
    return (void*)-1; 
  } 
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
						   (socklen_t*)&addrlen))<0) 
  { 
	perror("accept"); 
    return (void*)-1; 
  }
  
  valread = read( new_socket , buf, 1024);
  OMSG o;
  deserialize_OMSG(buf,&o);
  nCount = o.nCount;
  ns = (NEIGHBOR*)malloc(nCount*sizeof(NEIGHBOR));
  unsigned i;
  for (i = 0; i < nCount; ++i)
  {
	ns[i].port = o.ports[i];
	ns[i].name = o.neighbors[i];
  }
  thisPort = o.port;
  thisId = o.id;
  N = o.totalProc - 1;
  foundNum = 0;
  curRound = 0;
  
  //find eccentricity
  //set up nCount senders and a reciever
  pthread_t recT;
  pthread_create(&recT, NULL, &recMsg, NULL);
  while (foundNum < N)
  {
	printf("Round:%d\n",curRound);
	fflush(stdout);
	pthread_t *threads;
	threads = (pthread_t*)malloc(nCount*sizeof(pthread_t));
	for (i = 0; i < nCount; ++i)
	  pthread_create(&(threads[i]), NULL, &sendMsg, (void*)&ns[i]);
	for (i = 0; i < nCount; ++i)
	  pthread_join(threads[i],NULL);
	curRound++;
  }
  serialize_VEC_ECC(buf,vec, N);
  send(new_socket , buf , 1024, 0 );
  pthread_join(recT, NULL);
  return 0; 
}

void *sendMsg(void *ptr)
{
  NEIGHBOR* n = (NEIGHBOR*)ptr;
  int sock = 0;
  struct sockaddr_in serv_addr;
  byte buf[1024] = {0};

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return (void*)-1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(n->port);
  char adr[17] = "xxxx.utdallas.edu";
  unsigned i;
  memcpy(adr,n->name,4);

  char *ip;
  struct hostent *adrTrue;
  adrTrue = gethostbyname(adr);
  checkHostEntry(adrTrue);
  ip = inet_ntoa(*((struct in_addr*)adrTrue->h_addr_list[0]));

  if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
  {
    printf("\nInvalid address \"%s\" Address not supported \n",ip);
    return (void*)-1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection Failed \n");
    return (void*)-1;
  }
  
  serialize_u_short(buf,curRound);
  send(sock , buf , 1024, 0 );
  printf("MsgSent!:%s\n",n->name);
	fflush(stdout);
  read( sock , buf, 1024);
  printf("MsgRec!:%s\n",n->name);
	fflush(stdout);
  UMSG res;
  deserialize_UMSG(buf,&res);

  pthread_mutex_lock(&vecLock);
  for (i = 0; i < res.count;++i)
	if (res.ids[i] != thisId && isIn(vec, res.ids[i]) == 0)
	{
	  put(&vec, res.ids[i], curRound);
	  foundNum++;
	}
  pthread_mutex_unlock(&vecLock);
  
  return (void*)0;
}

void *recMsg(void* ptr)
{
  int server_fd, new_socket, valread; 
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
  byte buf[1024] = {0}; 
      
  // Creating socket file descriptor 
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
  { 
	perror("socket failed"); 
	return (void*)-1; 
  } 
       
  // Forcefully attaching socket to the port 8080 
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
				 &opt, sizeof(opt))) 
  { 
	perror("setsockopt"); 
	return (void*)-1; 
  } 
  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons( thisPort ); 
       
  // Forcefully attaching socket to the port 8080 
  if (bind(server_fd, (struct sockaddr *)&address,  
		   sizeof(address))<0) 
  { 
	perror("bind failed"); 
	return (void*)-1;
  } 
  if (listen(server_fd, nCount) < 0) 
  { 
	perror("listen"); 
    return (void*)-1; 
  }
  while (1)
  {
	new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen); 
	if (new_socket < 0)
	{ 
	  perror("new socket error"); 
	  return (void*)-1; 
	}
	int pid = fork();
	pid = fork();
	if (pid < 0)
	{
	  perror("ERROR on fork");
	}
	if (pid == 0)  {
	  close(server_fd);
	  handleMsg(new_socket);
	  exit(0);
	}
	else close(new_socket);
  }
}

void handleMsg(int sock)
{
  byte buf[1024];
  read(sock, buf, 1024);
  us r;
  deserialize_u_short(buf,&r);
  //we need to wait until the round matches our own
  //however, if foundNum == N, then rounds stop increasing
  //and we accept anyways.
  printf("MsgGot!:\n");
  fflush(stdout);
  while (r > curRound && foundNum < N )
	;
  us *ids;
  us count;
  if (curRound == 0)
  {
	count = 1;
	ids = (us*)malloc(sizeof(us));
	ids[0] = thisId;
  }
  else
  {
	pthread_mutex_lock(&vecLock);
	roundCount(vec, curRound - 1, &count);
	ids = (us*)malloc(count*sizeof(us));
	fillWithRound(vec, ids, curRound - 1);
	pthread_mutex_unlock(&vecLock);
  }
  UMSG u;
  u.count = count;
  u.ids = ids;
  serialize_UMSG(buf,u);
  printf("MsgResponding!\n");
  fflush(stdout);
  send(sock , buf , 1024, 0 );
  free(ids);
}
