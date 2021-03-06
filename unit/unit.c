#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <pthread.h>
#include "vecLib.h"
#include "unitStructs.h"

us thisId; //the id associated with this process.
us nCount; //number of neighbors
us thisPort; //port this process is listening on
NEIGHBOR* ns; //array of neighbors
us foundNum; //number of found processes
us N; //total number of processes
us curRound; //current round of the processes


us responses; //the number of responses expected
pthread_mutex_t resLock; //lock for editing number of responses
pthread_barrier_t resBar; //barrier to sync threads

VEC vec; //vector of found nodes (id and round)
pthread_mutex_t vecLock; //lock for editing vector of nodes

/*
 * Thread funciton to contact the origin. Opens on start and
 * listens on the designated originPort for information.
 */
void *contactOrigin(void* ptr);

/*
 * opens up sockets to accept requests for communication from
 * neighbors. Can accomodate as many messages as needed. Each
 * new accpet opens an instance of handleMsg
 */
void *recMsg(void* ptr);

/*
 * Once the port has been accepted, the handler gets what round
 * the contacting processes is on and returns what ids it was able
 * to find the previous round. 
 */
void *handleMsg(void* ptr);

/*
 * Attempts to open a connection with a listening neighbor, upon connection
 * a loop is executed until N ids were found. Each round all neighbors send
 * potential candidates. If a candidate is not in the found list, it is added.
 * (this is all done in mutex)
 */
void *sendMsg(void* ptr);

/*
 * Driver
 */
int main()
{
  init(&vec);
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
  printOMSG(o);
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

  pthread_t *threads;
  threads = (pthread_t*)malloc(nCount*sizeof(pthread_t));
  responses = nCount;
  pthread_barrier_init(&resBar, NULL, nCount+1);
  
  for (i = 0; i < nCount; ++i)
	pthread_create(&(threads[i]), NULL, &sendMsg, (void*)&ns[i]);

  while (foundNum < N)
  {
	printf("Round:%d\n",curRound);
	fflush(stdout);
	while(1)
	{
	  pthread_mutex_lock(&resLock);
	  if (responses == 0)
		break;
	  pthread_mutex_unlock(&resLock);
  	}
	curRound++;
	responses = nCount;
	pthread_mutex_unlock(&resLock);
	
	pthread_mutex_lock(&vecLock);
	printECC(&vec);
	pthread_mutex_unlock(&vecLock);
	
	pthread_barrier_wait(&resBar);
  }
  for (i = 0; i < nCount; ++i)
	  pthread_join(threads[i],NULL);
  printf("Complete:%d\n",curRound);
  fflush(stdout);
  
  pthread_mutex_lock(&vecLock);
  serialize_VEC_ECC(buf,&vec);
  pthread_mutex_unlock(&vecLock);
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
  char *ip = NULL;
  struct hostent *adrTrue;
  char adr[17] = "xxxx.utdallas.edu";
  unsigned i;
  while (1)
    {
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
	  printf("\n Socket creation error \n");
	  return (void*)-1;
	}

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(n->port);
      memcpy(adr,n->name,4);
 
      adrTrue = NULL;
      adrTrue = gethostbyname(adr);
      if (adrTrue != NULL && adrTrue->h_addr_list[0] != NULL)
	break;
    }
  while(ip == NULL)
    ip = inet_ntoa(*((struct in_addr*)
                     adrTrue->h_addr_list[0]));

  if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
    {
      printf("\nInvalid address \"%s\" Address not supported \n",ip);
      return (void*)-1;
    }

  int ress = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  while (ress < 0)
    {
      printf("\nConnection Failed retrying\n");
      ress = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    }
  while (foundNum < N)
  {
	serialize_u_short(buf,curRound);
	send(sock , buf , 1024, 0 );
	printf("MsgSent!:%s\n",n->name);
	fflush(stdout);
	
	int readTag = read( sock , buf, 1024);
	if (readTag <= 0)
	  break;
      
	
	printf("MsgRec!:%s\n",n->name);
	fflush(stdout);
	UMSG res;
	deserialize_UMSG(buf,&res);
	pthread_mutex_lock(&vecLock);
	for (i = 0; i < res.count;++i)
	{
	  if (res.ids[i] != thisId && isIn(&vec, (void*)&res.ids[i], EccIDEq) == -1)
	  {
		ECC* e = (ECC*)malloc(sizeof(ECC));
		e->id = res.ids[i];
		e->round = curRound;
		putBack(&vec, e);
		foundNum++;
	  }
	}
	pthread_mutex_unlock(&vecLock);
	pthread_mutex_lock(&resLock);
	responses--;
	pthread_mutex_unlock(&resLock);

	pthread_barrier_wait(&resBar);
  }
  close(sock);
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
       
  if (bind(server_fd, (struct sockaddr *)&address,  
		   sizeof(address))<0) 
  { 
	perror("bind failed"); 
	return (void*)-1;
  } 
  if (listen(server_fd, 5) < 0) 
  { 
	perror("listen"); 
    return (void*)-1; 
  }
  VEC vt;
  init(&vt);
  while (1)
  {
	new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen); 
	if (new_socket < 0)
	{ 
	  perror("new socket error"); 
	  return (void*)-1; 
	}
	THREADDATA* td = (THREADDATA*)malloc(sizeof(THREADDATA));
	td->socket = new_socket;
	pthread_create(&(td->data), NULL, &handleMsg, (void*)&(td->socket));
  	
	putBack(&vt,td);
  }
  us k;
  for (k = 0; k < vt.size;++k)
  {
	THREADDATA* td;
	td = (THREADDATA*)get(&vt,k);
	pthread_join(td->data, NULL);
	free(td);
  }
  clear(&vt);
}

void *handleMsg(void* ptr)
{
  while(1)
    {
      int sock = *((int *) ptr);
      byte buf[1024];
      int readTag;
      readTag = read(sock, buf, 1024);
      if (readTag <= 0)
	break;
      us r;
      deserialize_u_short(buf,&r);
      //we need to wait until the round matches our own
      //however, if foundNum == N, then rounds stop increasing
      //and we accept anyways.
      printf("MsgGot!%d:%d %d:%d t:%ld\n",r,curRound, foundNum, N, pthread_self());
      fflush(stdout);
      while (r > curRound && foundNum < N )
	;
      us *ids;
      us count = 0;
      if (r == 0)
	{
	  count = 1;
	  ids = (us*)malloc(sizeof(us));
	  ids[0] = thisId;
	}
      else
	{
	  pthread_mutex_lock(&vecLock);
	  roundCount(&vec, r - 1, &count);
	  ids = (us*)malloc(count*sizeof(us));
	  fillWithRound(&vec, ids, r - 1);
	  pthread_mutex_unlock(&vecLock);
	}
      UMSG u;
      u.count = count;
      u.ids = ids;
      serialize_UMSG(buf,u);
      printf("MsgResponding!%d:%d %d:%d t:%ld\n",r,curRound, foundNum, N, pthread_self());
      fflush(stdout);
      send(sock , buf , 1024, 0 );
      free(ids);
    }
}
