#include "vecLib.h"

int isIn(VEC* v, void*cmp,int (*compare)(void*,void*))
{
  VECDATA *temp = v->container;
  unsigned short at = 0;
  while (temp != NULL)
  {
	if (compare(cmp,temp->data))
	  return at;
	temp = temp->next;
	at++;
  }
  return -1;
}

void putBack(VEC* v, void* d)
{
  if (v->container == NULL)
  {
	v->container = (VECDATA*)malloc(sizeof(VECDATA));
	v->container->data = d;
	v->container->next = NULL;
	v->size=1;
	return;
  }
  VECDATA *temp = v->container;
  while (temp->next != NULL)
	temp = temp->next;
  temp->next = (VECDATA*)malloc(sizeof(VECDATA));
  temp->next->data = d;
  temp->next->next = NULL;
  v->size++;
  return;
}

void put(VEC* v, unsigned short at, void* d)
{
  if (at >= v->size)
	putBack(v,d);
  VECDATA *temp = v->container;
  unsigned int counter = 0;
  while (counter < at)
  {
	temp = temp->next;
	counter++;
  }
  VECDATA* spot = (VECDATA*)malloc(sizeof(VECDATA));
  spot->next = temp->next;
  spot->data = temp->data;
  temp->next = spot;
  temp->data = d;
  v->size++;
  return;
}

void* get(VEC* v, unsigned short i)
{
  if (i >= v->size)
	return NULL;
  VECDATA *temp = v->container;
  unsigned short counter = 0;
  while (counter < i)
  {
	temp = temp->next;
	counter++;
  }
  return temp->data;
}

void removeBack(VEC* v)
{
  if (v->size == 0)
	return;
  if (v->size == 1)
  {
	free(v->container);
	v->container = NULL;
	v->size--;
	return;
  }
  VECDATA* temp = v->container;
  while(temp->next->next != NULL)
	temp = temp->next;
  free(temp->next);
  temp->next = NULL;
  v->size--;
  return;
}

void removeAt(VEC* v, unsigned short at)
{
  if (at == v->size - 1)
  {
	removeBack(v);
	return;
  }
  if (at >= v->size)
	return;
  VECDATA* temp = v->container;
  unsigned short i = 0;
  while(i < at-1)
	temp = temp->next;
  VECDATA* t = temp->next;
  temp->next = t->next;
  free(t);
  v->size--;
  return;
}

void clear(VEC* v)
{
  while(v->size > 0)
	removeBack(v);
  return;
}

void init(VEC* v)
{
  v->size = 0;
  v->container = NULL;
}

int equalsUS(void* a, void* b)
{
  return *((unsigned short *)a) == *((unsigned short *)b);
}
