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
 * @file ublx_at_cmd_test.c
 *
 * @brief test application for lib320u
 *
 * @ingroup UCOM
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib320u.h"

#define CMD_NAME_LEN 128
#define CMD_MSG_LEN 1024

#define DEFAULT_MODEM_PORT "/dev/ttyACM0"

void main(int argc, char *argv[])
{

  char cmd_name[CMD_NAME_LEN];
  char *modem_port_name = NULL;
  char recv_msg[CMD_MSG_LEN];
  int fd, ret;
  int cmd_result;
  int send_sms = 0;
  char sms_msg[CMD_MSG_LEN];
  char *num = "\"3920635677\"";

  int opt;
  while ((opt = getopt (argc, argv, "c:s:p:h")) != -1)
  {
    switch (opt)
    {
      case 'c':
        printf("cmd name: \"%s\"\n", optarg);
        strncpy(cmd_name, optarg, strlen(optarg));
        break;
      case 's':
        send_sms = 1;
        strncpy(sms_msg, optarg, strlen(optarg));
        printf ("sms message: \"%s\"\n", sms_msg);
        break;
      case 'p':
        modem_port_name = optarg;
        printf ("modem port: \"%s\"\n", modem_port_name);
        break;
      case 'h':
        printf("atcmdtest:\n");
        printf("-c: AT command name.\n");
        printf("-s: sms message.\n");
        printf("-p: modem serial port name.\n");
        exit(0);
      default:
        printf("No input parameter.\n");
        exit(1);
    }
  }

  if(modem_port_name == NULL)
  {
    modem_port_name = DEFAULT_MODEM_PORT;
  }

  if(!cmd_name)
  {
    printf("please insert a AT command name.\n");
    exit(1);
  }

  ret = at_send_cmd(modem_port_name, cmd_name, recv_msg);

/*
  fd = open_modem(modem_port_name);
  if(fd < 0)
  {
    exit(1);
  }

  if(send_sms == 0)
  {
    printf("Command to be sent to serial port: %s\n", cmd_name);
    ret = send_cmd_to_modem(fd, cmd_name);
    if(ret < 0)
    {
      printf("Failed to send command to modem\n");
      exit(1);
    }
  }
  else
  {
     printf("Message %s to be sent to number: %s\n", sms_msg, num);
     send_sms_to_modem(fd, num, sms_msg);
  }

  ret = recv_data_from_modem(fd, &cmd_result, recv_msg);

  if(ret < 0)
  {
    printf("Failed to receive message from modem\n");
    exit(1);
  }

  close_modem(fd);
*/
  sleep(3);

  exit(0);
}


