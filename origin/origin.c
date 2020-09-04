#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "unitStructs.h"

PROC *procs;
unsigned totalProcs = 0;

void getProcs();
int main()
{
  getProcs();
  printProcs(totalProcs, procs);
  //send neighbors
  //wait for results
  //print results
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
	for (int i = 0; i < 256; ++i)
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
		  for (unsigned j = startId; j < endId; ++j)
			x = (x*10) + (line[j] - '0');
		  char* t = (char*)malloc((endProc - startProc+1)*sizeof(char));
		  for (unsigned j = startProc; j < endProc; ++j)
			t[j-startProc] = line[j];
		  t[endProc-startProc] = '\0';
		  unsigned y = 0;
		  for (unsigned j = startPort; j < i; ++j)
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
			procs[totalProcs].neighbors = (unsigned*)malloc(size*sizeof(unsigned));
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
