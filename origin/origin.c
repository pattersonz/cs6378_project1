#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <pthread.h>
#include "unitStructs.h"

PROC *procs;
unsigned totalProcs = 0;

void getProcs();
void *contactProc(void* ptr);
int main()
{
  getProcs();
  pthread_t *threads;
  threads = (pthread_t*)malloc(totalProcs*sizeof(pthread_t));
  unsigned i;
  for (i = 0; i < totalProcs; ++i)
	pthread_create(&(threads[i]), NULL, &contactProc, (void*)&procs[i]);
  for (i = 0; i < totalProcs; ++i)
	pthread_join(threads[i],NULL);
  return 0;
}

void *contactProc(void* ptr)
{
  PROC *p = (PROC*)ptr;
  printProcs(1,p);
  //setup socket connection
  int sock = 0, valread; 
  struct sockaddr_in serv_addr; 
  char buf[1024] = {0}; 
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
  { 
	printf("\n Socket creation error \n"); 
	return (void*)-1; 
  } 
   
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(ORIGIN_PORT); 
       
  // Convert IPv4 and IPv6 addresses from text to binary form
  char* adr = "xxxx.utdallas.edu";
  unsigned i;
  for (i = 0; i < 4; ++i)
	adr[i] = p->mach[i];
  if(inet_pton(AF_INET, adr, &serv_addr.sin_addr)<=0)  
  { 
	printf("\nInvalid address/ Address not supported \n"); 
	return (void*)-1; 
  } 
   
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
  { 
	printf("\nConnection Failed \n"); 
	return (void*)-1; 
  }
  
  //send neighbors
  OMSG o;
  o.id = p->id;
  o.nCount= p->nCount;
  o.neighbors = (char**)malloc(o.nCount*sizeof(char*));
  o.ports = (us*)malloc(o.nCount*sizeof(us));
  for (i = 0; i < o.nCount; ++i)
  {
	us id = p->id;
	o.neighbors[i] = procs[id].mach;
	o.ports[i] = procs[id].port;
  }
  serialize_OMSG(buf,o);
  send(sock , buf , 1024, 0 );
  free(o.neighbors);
  free(o.ports);
  //wait for results
  valread = read( sock , buf, 1024);
  us res;
  deserialize_u_short(buf,&res);
  //print results
  printf("%d:%d\n",p->id,res);
  return (void*)0; 
}

void getProcs()
{
  //read file
  FILE * file;
  file = fopen("config.txt", "r");
  char line[256];
  int phase = 0;
  int numProc = 0;
  int startId = -1, endId = -1, startProc = -1, endProc = -1, startPort = -1, endPort = -1;
  VEC_INT *startN = NULL, *endN = NULL;
  int curN = -1;
  while (fgets(line, sizeof(line), file) && phase < 3)
  {
	unsigned i;
	for ( i = 0; i < 256; ++i)
	{
	  char c = line[i];
	  //num Processes
	  if (phase == 0)
	  {
		if (c >= '0' && c <= '9')
		{
		  numProc *= 10;
		  numProc += c - '0';
		}
	  }
	  //identify processes
	  else if (phase == 1)
	  {
		if(c != ' ' && startId == -1)
		  startId = i;
		else if (c == ' ' && startId != -1 && endId == -1)
		  endId = i;
		else if (c != ' ' && endId != -1 && startProc == -1)
		  startProc = i;
		else if (c == ' ' && startProc != -1 && endProc == -1)
		  endProc = i;
		else if (c != ' ' && endProc != -1 && startPort == -1)
		  startPort = i;
		else if ((c < '0' || c > '9') && startPort != -1 && endPort == -1)
		{
		  endPort = 1;
		  unsigned x = 0;
		  unsigned j;
		  for (j = startId; j < endId; ++j)
			x = (x*10) + (line[j] - '0');
		  char* t = (char*)malloc((endProc - startProc+1)*sizeof(char));
		  for (j = startProc; j < endProc; ++j)
			t[j-startProc] = line[j];
		  t[endProc-startProc] = '\0';
		  unsigned y = 0;
		  for (j = startPort; j < i; ++j)
			y = (y*10) + (line[j] - '0');
		  procs[totalProcs].id = x;
		  procs[totalProcs].mach = t;
		  procs[totalProcs].port = y;
		  totalProcs++;
		}
	  }
	  //get neighbors
	  else
	  {
		if (c == ' ' && curN != -1)
		{
		  if (startN == NULL)
		  {
			startN = (VEC_INT*)malloc(sizeof(VEC_INT));
			startN->data = (unsigned)curN;
			startN->next == NULL;
			endN = startN;
		  }
		  else
		  {
			endN->next = (VEC_INT*)malloc(sizeof(VEC_INT));
			endN = endN->next;
			endN->data = curN;
			endN->next = NULL;
		  }
		  curN = -1;
		}
		if (c >= '0' && c <= '9')
		{
		  if (curN < 0)
			curN = 0;
		  curN = curN*10 + (c - '0');
		}
	  }

	  if (c == '#' || c == '\n' || c == '\0')
	  {
		if (phase == 2)
		{
		  if (totalProcs == numProc)
			phase = 3;
		  else if (startN != NULL)
		  {
			unsigned size = 0;
			VEC_INT* temp = startN;
			while(temp != NULL)
			{
			  size++;
			  temp = temp->next;
			}
			procs[totalProcs].nCount = size;
			procs[totalProcs].neighbors = (us*)malloc(size*sizeof(unsigned));
			size = 0;
			temp = startN;
			while(temp != NULL)
			{
			  procs[totalProcs].neighbors[size] = temp->data;
			  size++;
			  VEC_INT* z = temp;
			  temp = temp->next;
			  free(z);
			}
			totalProcs++;
			startN = NULL;
			endN = NULL;
		  }
		}
		else if (phase == 1)
		{
		  if (totalProcs == numProc)
		  {
			phase = 2;
			totalProcs = 0;
		  }
		  else
		  {
			startId = endId = startProc = endProc = startPort = endPort = -1;
		  }
		}
		else if (phase == 0 && numProc > 0)
		{
		  procs = (PROC *) malloc(numProc * sizeof(PROC));
		  phase = 1;
		}
		break;
	  }
	} 
  }
  fclose(file);
}
