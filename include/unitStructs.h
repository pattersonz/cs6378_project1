#include <stdio.h>
#include <stdlib.h>

typedef struct proc
{
  unsigned id;
  char* mach;
  unsigned port;
  unsigned *neighbors;
  unsigned nCount;
} PROC;

typedef struct vector_int
{
  unsigned data;
  struct vector_int *next;
} VEC_INT;

void printProcs(unsigned count, PROC* p)
{
  for (unsigned i = 0; i < count; ++i)
  {
	printf("proc:%d\n\tmachine:%s\n\tport:%d\n\tNeighbors:",p[i].id,p[i].mach,p[i].port);
	for (unsigned j = 0; j < p[i].nCount; ++j)
	  printf("%d ",p[i].neighbors[j]);
	printf("\n");
  }
}
