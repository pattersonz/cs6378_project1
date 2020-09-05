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

void *contactOrigin(void* ptr);

int main()
{
  printf("start all\n");
  pthread_t originThread;
  pthread_create(&originThread, NULL, &contactOrigin, NULL);
  pthread_join(originThread, NULL);
  printf("end all\n");
  return -1;
} 

void *contactOrigin(void* ptr)
{
  printf("start Thread\n");
  int server_fd, new_socket, valread; 
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
  char buf[1024] = {0}; 
      
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
  
  printf("read msg\n");
  valread = read( new_socket , buf, 1024);
  printf("complete\n");
  OMSG o;
  deserialize_OMSG(buf,&o);
  serialize_u_short(buf,o.id);
  printf("send msg\n");
  send(new_socket , buf , 1024, 0 );
  printf("complete\n");
  return 0; 
}
