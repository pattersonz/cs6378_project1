#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define ORIGIN_PORT 1024
typedef unsigned char byte;
typedef unsigned short us;
typedef struct proc
{
  us id;
  char* mach;
  us port;
  us *neighbors;
  us nCount;
} PROC;

typedef struct origin_message
{
  us id;
  us port;
  us nCount;
  char** neighbors;
  us *ports;
  us totalProc;
} OMSG;

typedef struct unit_message
{
  us count;
  us *ids;
} UMSG;

typedef struct neighbor_pair
{
  us port;
  char *name;
} NEIGHBOR;


typedef struct vector_int
{
  unsigned data;
  struct vector_int *next;
} VEC_INT;

typedef struct vector_eccen
{
  us id;
  us round;
  struct vector_eccen *next;
} VEC_ECC;

typedef struct vector_pthread
{
  pthread_t data;
  struct vector_pthread *next;
} VEC_THREAD;


void printProcs(unsigned count, PROC* p)
{
  unsigned i,j;
  for (i = 0; i < count; ++i)
  {
	printf("proc:%d\n\tmachine:%s\n\tport:%d\n\tNeighbors:",p[i].id,p[i].mach,p[i].port);
	for (j = 0; j < p[i].nCount; ++j)
	  printf("%d ",p[i].neighbors[j]);
	printf("\n");
  }
}

void printOMSG(OMSG o)
{
  printf("id:%d port:%d neighbors:%d, N:%d\n", o.id, o.port, o.nCount, o.totalProc);
  unsigned i;
  for (i = 0; i < o.nCount; ++i)
  {
	printf("\tn:%d name:%s port:%d\n", i, o.neighbors[i], o.ports[i]);
  }
  fflush(stdout);
}

byte *serialize_u_short(byte *buf, us v)
{
  buf[0] = v >> 8;
  buf[1] = v;
  return buf + 2;
}

byte *serialize_u_short_ptr(byte *buf, us *v, us size)
{
  buf = serialize_u_short(buf,size);
  unsigned i;
  for (i = 0; i < size; ++i)
	buf = serialize_u_short(buf,v[i]);
  return buf;
}

byte *serialize_char(byte *buf, char c)
{
  buf[0] = c;
  return buf + 1;
}

byte *serialize_char_ptr(byte* buf, char* c, us size)
{
  buf = serialize_u_short(buf,size);
  unsigned i;
  for (i = 0; i < size; ++i)
	buf = serialize_char(buf,c[i]);
  return buf;
}

byte *serialize_char_ptr2(byte* buf, char** c, us *size, us size2)
{
  buf = serialize_u_short(buf,size2);
  unsigned i;
  for (i = 0; i < size2; ++i)
	buf = serialize_char_ptr(buf,c[i],size[i]);
  return buf;
}

byte *serialize_OMSG(byte* buf, OMSG o)
{
  //us id;
  buf = serialize_u_short(buf,o.id);
  //us port;
  buf = serialize_u_short(buf,o.port);
  //us nCount;
  buf = serialize_u_short(buf,o.nCount);
  //char** neighbors;
  us *temp = (us*)malloc(o.nCount*sizeof(us));
  unsigned i;
  for (i = 0; i < o.nCount; ++i)
	temp[i] = 4;
  buf = serialize_char_ptr2(buf,o.neighbors,temp,o.nCount);
  free(temp);
  //us *ports;
  buf = serialize_u_short_ptr(buf,o.ports,o.nCount);
  //us totalProc
  buf = serialize_u_short(buf,o.totalProc);
  return buf;
}

byte *serialize_UMSG(byte* buf, UMSG u)
{
  //us count
  buf = serialize_u_short(buf,u.count); 
  //us *ids;
  buf = serialize_u_short_ptr(buf,u.ids, u.count);
  return buf;
}

byte *serialize_VEC_ECC(byte* buf, VEC_ECC *v, us size)
{
  buf = serialize_u_short(buf, size);
  VEC_ECC *temp = v;
  while(temp != NULL)
  {
	buf = serialize_u_short(buf, temp->id);
	buf = serialize_u_short(buf, temp->round);
	temp = temp->next;
  }
  return buf;
}

byte *deserialize_u_short(byte *buf, us* v)
{
  (*v) = buf[0];
  (*v) = (*v) << 8;
  (*v) = (*v) + buf[1];
  return buf + 2;
}

byte *deserialize_u_short_ptr(byte *buf, us **v)
{
  us s;
  buf = deserialize_u_short(buf,&s);
  unsigned i;
  (*v) = (us*)malloc(s*sizeof(us));
  for (i = 0; i < s; ++i)
	buf = deserialize_u_short(buf,&((*v)[i]));
  return buf;
}

byte *deserialize_char(byte *buf, char *c)
{
  *c = buf[0];
  return buf + 1;
}

byte *deserialize_char_ptr(byte* buf, char** c)
{
  us s;
  buf = deserialize_u_short(buf,&s);
  (*c) = (char*)malloc(s*sizeof(char));
  unsigned i;
  for (i = 0; i < s; ++i)
	buf = deserialize_char(buf,&((*c)[i]));
  return buf;
}

byte *deserialize_char_ptr2(byte* buf, char*** c)
{
  us s;
  buf = deserialize_u_short(buf,&s);
  (*c) = (char**)malloc(s*sizeof(char*));
  unsigned i;
  for (i = 0; i < s; ++i)
	buf = deserialize_char_ptr(buf,&((*c)[i]));
  return buf;
}

byte *deserialize_OMSG(byte* buf, OMSG *o)
{
  //us id;
  buf = deserialize_u_short(buf,&(o->id));
  //us port;
  buf = deserialize_u_short(buf,&(o->port));
  //us nCount;
  buf = deserialize_u_short(buf,&(o->nCount));
  //char** neighbors;
  buf = deserialize_char_ptr2(buf,&(o->neighbors));
  //us *ports;
  buf = deserialize_u_short_ptr(buf,&(o->ports));
  //us totalProc;
  buf = deserialize_u_short(buf,&(o->totalProc));
  return buf;
}

byte *deserialize_UMSG(byte* buf, UMSG *u)
{
  //us count
  buf = deserialize_u_short(buf,&(u->count)); 
  //us *ids;
  buf = deserialize_u_short_ptr(buf,&(u->ids));
  return buf;
}

byte *deserialize_VEC_ECC(byte* buf, VEC_ECC **v)
{
  us s;
  buf = deserialize_u_short(buf, &s);
  if (s == 0)
	return buf;
  *v = (VEC_ECC*)malloc(sizeof(VEC_ECC));
  buf = deserialize_u_short(buf, &((*v)->id));
  buf = deserialize_u_short(buf, &((*v)->round));
  VEC_ECC* temp = *v;
  us i;
  for (i = 1; i < s; ++i)
  {
	temp->next = (VEC_ECC*)malloc(sizeof(VEC_ECC));
	temp = temp->next;
	buf = deserialize_u_short(buf, &temp->id);
	buf = deserialize_u_short(buf, &temp->round);
	temp->next = NULL;
  }
  return buf;
}



us isIn(VEC_ECC* v, us id)
{
  VEC_ECC *temp = v;
  while (temp != NULL)
  {
	if (id == temp->id)
	  return 1;
	temp = temp->next;
  }
  return 0;
}

void put(VEC_ECC** v, us id, us round)
{
  if (*v == NULL)
  {
	(*v) = (VEC_ECC*)malloc(sizeof(VEC_ECC));
	(*v)->id = id;
	(*v)->round = round;
	(*v)->next = NULL;
  }
  else
  {
	VEC_ECC* temp = (*v);
	while (temp->next != NULL)
	  temp = temp->next;
	temp->next = (VEC_ECC*)malloc(sizeof(VEC_ECC));
	temp = temp->next;
	temp->id = id;
	temp->round = round;
	temp->next = NULL;
  }
}

void roundCount(VEC_ECC* vec, us r, us*c)
{
  VEC_ECC* t = vec;
  *c = 0;
  while (t != NULL)
  {
	if (t->round == r)
	  ++(*c);
	t = t->next;
  }
}

void fillWithRound(VEC_ECC* v, us* ids, us r)
{
  VEC_ECC* t = v;
  us c = 0;
  while (t != NULL)
  {
	if (t->round == r)
	{
	  ids[c] = t->id;
	  ++c;
	}
	t = t->next;
  }
} 

// Returns host information corresponding to host name 
void checkHostEntry(struct hostent * hostentry) 
{ 
    if (hostentry == NULL) 
    { 
        perror("gethostbyname"); 
        exit(1); 
    } 
} 
