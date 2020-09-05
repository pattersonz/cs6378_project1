#include <stdio.h>
#include <stdlib.h>
#define ORIGIN_PORT 1000
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
  us nCount;
  char** neighbors;
  us *ports;
} OMSG;

typedef struct vector_int
{
  unsigned data;
  struct vector_int *next;
} VEC_INT;

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
  //us nCount;
  buf = deserialize_u_short(buf,&(o->nCount));
  //char** neighbors;
  buf = deserialize_char_ptr2(buf,&(o->neighbors));
  //us *ports;
  buf = deserialize_u_short_ptr(buf,&(o->ports));
  return buf;
}
