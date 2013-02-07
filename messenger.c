#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "utstring.h"
#include "uthash.h"
#include "messenger.h"

int messenger_update(struct messenger *messenger,void *data,size_t sz){

  return 0;
}

int messenger_init(struct messenger *messenger){
  int fd, connfd;
  time_t ticks;
  char sendBuff[1025];
  int port;
  char *host;
  struct sockaddr_in serv_addr, clie_addr;
  fd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  host = strtok (messenger->hostporta, ":");
  port = atoi(strtok (NULL, ":"));
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//(messenger->desejo == MASTER ? INADDR_ANY : host);

  if (messenger->desejo == MASTER){
    if (bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      error("ERROR on binding");
    listen(fd,5);
    printf("bind\n");
    while(1){
        connfd = accept(fd, (struct sockaddr*)NULL, NULL);

        sprintf(sendBuff,"Ola\r\n");
        write(connfd, sendBuff, strlen(sendBuff));

        close(connfd);
        sleep(1);
     }

  }
  else {

  }

  return 0;
}

