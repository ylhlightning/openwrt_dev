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
#include <netinet/in.h>


#define CLIENT_NUM "{ \"number\": \"3920635677\" }"
#define IP_QUERY "www.google.it"
#define SERVER_ADDR "151.9.34.82"
#define SERVER_PORT 8445
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512
#define FILE_NAME "hello.txt"

int file_download(void)
{
  struct sockaddr_in client_addr;
  bzero(&client_addr,sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = htons(INADDR_ANY);
  client_addr.sin_port = htons(0);

  int client_socket = socket(AF_INET,SOCK_STREAM,0);
  if( client_socket < 0)
  {
    printf("Create Socket Failed!\n");
    return -1;
  }

  if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
  {
    printf("Client Bind Port Failed!\n");
    return -1;
  }


  struct sockaddr_in server_addr;
  bzero(&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  if(inet_aton(SERVER_ADDR,&server_addr.sin_addr) == 0)
  {
    printf("Server IP Address Error!\n");
    return -1;
  }
  server_addr.sin_port = htons(SERVER_PORT);
  socklen_t server_addr_length = sizeof(server_addr);

  if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
  {
    printf("Can Not Connect To %s!\n",SERVER_ADDR);
    return -1;
  }

  char buffer[BUFFER_SIZE];
  bzero(buffer,BUFFER_SIZE);
  strncpy(buffer, FILE_NAME, strlen(FILE_NAME)>BUFFER_SIZE?BUFFER_SIZE:strlen(FILE_NAME));

  send(client_socket,buffer,BUFFER_SIZE,0);

  FILE * fp = fopen(FILE_NAME,"w");
  if(NULL == fp )
  {
    printf("File:\t%s Can Not Open To Write\n", FILE_NAME);
    return -1;
  }

  bzero(buffer,BUFFER_SIZE);
  int length = 0;
  while( length = recv(client_socket,buffer,BUFFER_SIZE,0))
  {
    if(length < 0)
    {
      printf("Recieve Data From Server %s Failed!\n", SERVER_ADDR);
      break;
    }

    int write_length = fwrite(buffer,sizeof(char),length,fp);
    if (write_length<length)
    {
      printf("File:\t%s Write Failed\n", FILE_NAME);
      break;
    }
    bzero(buffer,BUFFER_SIZE);
  }
  printf("Recieve File:\t %s From Server[%s] Finished\n",FILE_NAME, SERVER_ADDR);

  fclose(fp);

  close(client_socket);
  return 0;
}


static int query_dns(char *ip_host)
{
  struct addrinfo hints, *res, *p;
  int status;
  char ipstr[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(ip_host, NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  printf("IP addresses for %s:\n\n", ip_host);

  for(p = res;p != NULL; p = p->ai_next) {
    void *addr;
    char *ipver;

    // get the pointer to the address itself,
    // different fields in IPv4 and IPv6:
    if (p->ai_family == AF_INET) { // IPv4
       struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
       addr = &(ipv4->sin_addr);
       ipver = "IPv4";
    } else { // IPv6
       struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
       addr = &(ipv6->sin6_addr);
       ipver = "IPv6";
    }

    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("  %s: %s\n", ipver, ipstr);
 }

 freeaddrinfo(res); // free the linked list
 return 0;
}


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

    if((num_quatation_mark == 3) && (*tmp_ptr != quatation_mark))
    {
      *addr_str_ptr = *tmp_ptr;
      addr_str_ptr ++;
    }
    tmp_ptr ++;
  }

  if(num_quatation_mark != 4)
  {
     printf("Error format string\n");
     return -1;
  }

  *addr_str_ptr = '\0';
  return 0;
}


int client_cmd_send(int client_cmd, char *client_data, char *client_addr, char *client_msg)
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

  if(client_data != NULL)
  {
    strncpy(msg.ublx_client_data, client_data, strlen(client_data));
  }
    
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
  char client_data[1024];

  if(argc != 2)
  {
    printf("Usage: %s <ip of server> \n",argv[0]);
    exit(1);
  }

  memset(client_ip_addr, 0, strlen(client_ip_addr));

  strncpy(client_ip_addr, argv[1], strlen(argv[1]));

  printf("Active the wwan connection.\n");

  if(client_cmd_send(UBLX_WWAN_OPEN_CONNECTION, NULL, client_ip_addr, client_msg) == -1)
  {
    exit(1);
  }
  
  printf("wwan connection established.\n");

  sleep(5);

  printf("Retrive the wwan public ip address.\n");

  if(client_cmd_send(UBLX_WWAN_GET_ADDR, NULL, client_ip_addr, client_msg) == -1)
  {
    exit(1);
  }

  get_addr_from_string(client_msg, wwan_ip_addr);

  printf("%s\n", wwan_ip_addr);


  strncpy(client_data, CLIENT_NUM, strlen(CLIENT_NUM));

  printf("Send the wwan public ip address to number:%s.\n", client_data);

  if(client_cmd_send(UBLX_WWAN_SEND_ADDR, client_data, client_ip_addr, client_msg) == -1)
  {
    exit(1);
  }

  printf("wwan public ip address message sent.\n");

  printf("Test network connection via wwan.\n");

  query_dns(IP_QUERY);

  printf("Download a file from server :%s\n", SERVER_ADDR);

  if (file_download()==-1)
  {
    printf("Failed to download file.\n");
  }
  else
  {
    printf("File downloaded.\n");
  }

  exit(0);
}





