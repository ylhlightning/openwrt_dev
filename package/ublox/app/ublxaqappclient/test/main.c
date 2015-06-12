/***********************************************************
 *
 * Copyright (C) u-blox Italy S.p.A.
 *
 * u-blox Italy S.p.A.
 * Via Stazione di Prosecco 15
 * 34010 Sgonico - TRIESTE, ITALY
 *
 * All rights reserved.
 *
 * This source file is the sole property of
 * u-blox Italy S.p.A. Reproduction or utilization of
 * this source in whole or part is forbidden
 * without the written consent of u-blox Italy S.p.A.
 *
 ******************************************************************************/
/**
 *
 * @file main.c
 *
 * @brief ublx aqapp client application server test application run in host machine.
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include "ublx_client.h"

static int get_addr_from_string(char *string, char *addr_string)
{
  char quatation_mark = '"';
  int num_quatation_mark = 0;
  char *addr_str_ptr=addr_string;

  char *tmp_ptr = string;

  while(*tmp_ptr != '\0')
  {
    if(*tmp_ptr == quatation_mark)
      num_quatation_mark ++;

    if((num_quatation_mark == 1) && (*tmp_ptr != quatation_mark))
    {
      *addr_str_ptr = *tmp_ptr;
      addr_str_ptr ++;
    }
    tmp_ptr ++;
  }

  if(num_quatation_mark != 2)
  {
     printf("Error format string\n");
     return -1;
  }

  *addr_str_ptr = '\0';
  return 0;
}


int client_cmd_send(int client_cmd, char *client_addr, char *client_msg)
{
  int sockfd = 0, n = 0;
  struct sockaddr_in serv_addr;
  ublx_client_msg_t msg;

  memset(client_msg, 0, strlen(client_msg));

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Error : Could not create socket \n");
    return -1;
  }

  memset(&msg, 0, sizeof(msg));
  msg.ublx_client_func = client_cmd;
    
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(5000);

  if(inet_pton(AF_INET, client_addr, &serv_addr.sin_addr)<=0)
  {
    printf("inet_pton error occured\n");
    return -1;
  }

  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("Error : Connect Failed\n");
    return -1;
  }

  if(send(sockfd, &msg, sizeof(msg), 0) < 0)
  {
    printf("Error: Send messge error\n");
    return -1;
  }
  else
  {
    printf("Message sent to Server\n");
  }

  while(1)
  {
    if((n = recv(sockfd, &msg, sizeof(msg),0)) > 0)
    { 
      printf("Received reply [%s] from Server\n", msg.ublx_client_reply_msg);
      break;
    }

    if(n < 0)
    {
      printf("Read error!\n");
      return -1;
    }
  }

  strncpy(client_msg, msg.ublx_client_reply_msg, strlen(msg.ublx_client_reply_msg));

  return 0;
}

int main(int argc, char *argv[])
{
  char client_ip_addr[128];
  char client_msg[1024];
  char wwan_ip_addr[128];

  if(argc != 2)
  {
    printf("Usage: %s <ip of server> \n",argv[0]);
    exit(1);
  }

  strncpy(client_ip_addr, argv[1], strlen(argv[1]));

  printf("Active the wwan connection.\n");

  if(client_cmd_send(UBLX_WWAN_OPEN_CONNECTION, client_ip_addr, client_msg) == -1)
  {
    exit(1);
  }

  sleep(5);

  printf("Retrive the wwan public ip address.\n");

  if(client_cmd_send(UBLX_WWAN_GET_ADDR, client_ip_addr, client_msg) == -1)
  {
    exit(1);
  }

  get_addr_from_string(client_msg, wwan_ip_addr);

  printf("wwan ip address is : %s\n", wwan_ip_addr);

  exit(0);
}





