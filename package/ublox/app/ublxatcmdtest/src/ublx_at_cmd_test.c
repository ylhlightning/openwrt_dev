#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib320u.h"

#define CMD_NAME_LEN 128
#define CMD_MSG_LEN 1024

void main(int argc, char *argv[])
{

  char cmd_name[CMD_NAME_LEN];
  char *modem_port_name = "/dev/ttyUSB3";
  char recv_msg[CMD_MSG_LEN];
  int fd;
  
  if (argc < 2)
  {
    printf("No input parameters\n");
    exit(1);
  }

  strncpy(cmd_name, argv[1], strlen(argv[1]));

  printf("Command to be sent to serial port: %s\n", cmd_name);

  fd = open_modem(modem_port_name);
  if(fd < 0)
  {
    exit(1);
  }

  send_cmd_to_modem(fd, cmd_name);

  recv_data_from_modem(fd, recv_msg);

  close_modem(fd);

  exit(0);
}


