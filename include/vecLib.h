#ifndef VECLIB_H
#define VECLIB_H
#include <stdlib.h>
typedef struct vec_data
{
  void* data;
  struct vec_data* next;
} VECDATA;

typedef struct vec_container
{
  VECDATA* container;
  unsigned int size;
}VEC;

int isIn(VEC* v, void*cmp,int (*compare)(void*,void*));
void putBack(VEC* v, void* d);
void put(VEC* v, unsigned short at, void* d);
void* get(VEC* v, unsigned short i);
void removeBack(VEC* v);
void removeAt(VEC* v, unsigned short at);
void clear(VEC* v);
void init(VEC* v);
int equalsUS(void* a, void* b);
#endif
