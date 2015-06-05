/*************************************************************************
    > File Name: serial.c
    > Author: Linhu Ying
    > Mail:  linhu.ying@u-blox.com
    > Created Time: Mon May 18 19:12:08 2015
 ************************************************************************/

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_add

#define CRTL_Z 26
#define WAIT_MODEM_ANSWER_MAX_TIMEOUT 120
#define UBLX_SEND_IP_ADDR_VIA_SMS 1
#define UBLX_CLIENT_MSG_LEN 1024

static int fd;
static char *modem_port = "/dev/ttyUSB3";

typedef struct ublx_client_msg
{
  unsigned int ublx_client_func;
  unsigned char ublx_client_data[UBLX_CLIENT_MSG_LEN];
  char ublx_client_reply_msg[UBLX_CLIENT_MSG_LEN];
} ublx_client_msg_t;

typedef struct cmd
{
  char cmd_name[128];
  char cmd_msg[1024];
} cmd_t;


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

  addr_str_ptr = '\0';
  return 0;
}


static void timer_handler(int signum)
{
  switch(signum) {
    case SIGALRM:
      printf("sigarlm signal caught!\n");
      break;
    case SIGVTALRM:
      printf("sigvtalrm signal caught!\n");
      break;
    default:
      break;
  }
  printf("AT command wait timeout!\n");
  close(fd);
  exit(1);
}

static void interrupt_handler(int signum)
{
  switch(signum)
  {
    case SIGINT:
      printf("sigint signal caught!");
      break;
    default:
      break;
  }
  close(fd);
  close(1);
}

static void modem_answer_timer_setup(void)
{
  timer_t gTimerid;
  struct sigaction sa;
  struct itimerspec timer;
  struct itimerspec new_timer;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &timer_handler;
  sigaction (SIGVTALRM, &sa, NULL);

  sa.sa_handler = &timer_handler;
  sigaction(SIGALRM, &sa, NULL);

  timer.it_value.tv_sec = WAIT_MODEM_ANSWER_MAX_TIMEOUT;
  timer.it_value.tv_nsec = 0;
  timer.it_interval.tv_sec = WAIT_MODEM_ANSWER_MAX_TIMEOUT;
  timer.it_interval.tv_nsec = 0;

  timer_create(CLOCK_REALTIME, NULL, &gTimerid);
  timer_settime(gTimerid, 0, &timer, NULL);

}

static void modem_interrupt_setup(void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &interrupt_handler;
  sigaction (SIGINT, &sa, NULL);
}

int open_modem(void)
{
  struct termios options;
  /* open the port */
  fd = open(modem_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd == -1)
  {
    printf("Unable to open modem port %s, error: %s\n", modem_port, strerror(errno));
    return -1;
  }
  else
  {
    printf("Succeed to open modem port %s\n", modem_port); 
  }

  /* get the current options */
  tcgetattr(fd, &options);

  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);

  /* set raw input, 1 second timeout */
  options.c_cflag |= (CLOCAL | CREAD);

  //No parity
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  /* set the options */
  tcsetattr(fd, TCSANOW, &options);

  modem_answer_timer_setup();

  modem_interrupt_setup();

  return 0;
}

int recv_data_from_modem(int fd, char *modem_reply_msg)
{
  char buffer[UBLX_CLIENT_MSG_LEN];  /* Input buffer */
  char *bufptr;      /* Current char in buffer */
  ssize_t n;       /* Number of bytes read */

  memset(buffer, 0, sizeof(buffer));

  /* read characters into our string buffer until we get a CR or NL */
  bufptr = buffer;

  while(1)
  {
    n = read(fd, bufptr, 1);
    if(n == -1)
    {
      switch(errno)
      {
        case EAGAIN:
          continue;
        default:
          printf("read port error!\n");
          break;
     }
    }

    if(n == 0)
      break;

    bufptr += n;

    if(strstr(buffer, "OK") != NULL || strstr(buffer, "ERROR") != NULL)
      break;
  }

   /* nul terminate the string and see if we got an OK response */
  *bufptr = '\0';
  printf("%s\n", buffer);

  strncpy(modem_reply_msg, buffer, UBLX_CLIENT_MSG_LEN);
  return 0;
}


int send_cmd_to_modem(int fd, cmd_t *cmd)
{
  ssize_t write_bytes;
  size_t cmd_len = strlen(cmd->cmd_name);
  size_t msg_len = strlen(cmd->cmd_msg);
  char *cmd_name = cmd->cmd_name;
  char ctrl_z = '\x1A';
  char ctrl_z_bit[2];

  sprintf(ctrl_z_bit, "%c", ctrl_z);

  cmd_name = strncat(cmd_name, "\r", cmd_len + 1);

  /* send an AT command followed by a CR */
  if ((write_bytes = write(fd, cmd->cmd_name, cmd_len + 1)) < cmd_len+1)
  {
    printf("write error: %s\n",strerror(errno));
    return -1;
  }
  else
  {
    printf("Successful to write %d bytes to modem serial port\n", write_bytes);
  }

  if(strlen(cmd->cmd_msg))
  {
      sleep(2);

      /* send a message */
      if ((write_bytes = write(fd, cmd->cmd_msg, msg_len + 1)) < msg_len+1)
      {
        printf("write error: %s\n",strerror(errno));
        return -1;
      }
      else
      {
        printf("Successful to write %d byte messages [%s] to modem serial port\n", write_bytes, cmd->cmd_msg);
      }

      sleep(1);

      /* send a message followed by CTRL-Z  */
      if ((write_bytes = write(fd, ctrl_z_bit, 1)) < 1)
      {
        printf("write error: %s\n",strerror(errno));
        return -1;
      }
      else
      {
        printf("Successful to send CTRL-Z\n");
      }

  }

  /*flush all read write buffer */
  tcflush(fd, TCIOFLUSH);

  return 0;
}

static void ublx_modem_send_at(ublx_client_msg_t *client_msg, char *msg)
{
  cmd_t cmd;
  memset(&cmd, 0, sizeof(cmd));
    
  char *cmd_name = "at";
    
  strncpy(cmd.cmd_name, cmd_name, UBLX_CLIENT_MSG_LEN);
  strcpy(cmd.cmd_msg, "");
        
  printf("Command to be sent to serial port: %s\n", cmd.cmd_name);
  printf("Message to be sent to serial port: %s\n", cmd.cmd_msg);
        
  send_cmd_to_modem(fd, &cmd);

  sleep(1);
  
  recv_data_from_modem(fd, msg);
    
  printf("at respond is: %s\n", msg);

}

static void ublx_modem_get_ipaddr(ublx_client_msg_t *client_msg, char *msg)
{
  cmd_t cmd;
  char wwan_addr[20];
  char *ip_msg[20];
  memset(&cmd, 0, sizeof(cmd));

  char *cmd_get_wwan_addr = "at!scpaddr=1";

  strncpy(cmd.cmd_name, cmd_get_wwan_addr, UBLX_CLIENT_MSG_LEN);
  strcpy(cmd.cmd_msg, "");
    
  printf("Command to be sent to serial port: %s\n", cmd.cmd_name);
  printf("Message to be sent to serial port: %s\n", cmd.cmd_msg);
    
  send_cmd_to_modem(fd, &cmd);

  sleep(1);
    
  recv_data_from_modem(fd, msg);

  get_addr_from_string(msg, ip_msg);

  strcpy(msg, "");

  strncpy(msg, ip_msg, strlen(ip_msg));

  msg[strlen(ip_msg)] = '\0';

  printf("wwan pubblic ip address is: %s\n", msg);
    
}


static void ublx_modem_send_sms(ublx_client_msg_t *client_msg, char *msg)
{
  cmd_t cmd;
  memset(&cmd, 0, sizeof(cmd));

  char *sms_cmd = "AT+CMGS=\"3920635677\"";

  strncpy(cmd.cmd_name, sms_cmd, UBLX_CLIENT_MSG_LEN);
  strncpy(cmd.cmd_msg, msg, UBLX_CLIENT_MSG_LEN);
    
  printf("Command to be sent to serial port: %s\n", cmd.cmd_name);
  printf("Message to be sent to serial port: %s\n", msg);
    
  send_cmd_to_modem(fd, &cmd);

  sleep(1);
  
  recv_data_from_modem(fd, msg);
    
}

static void ublx_server_create(void)
{
  int socket_desc, client_sock, socket_size, read_size;
  struct sockaddr_in server, client;
  ublx_client_msg_t client_message;
  char msg[UBLX_CLIENT_MSG_LEN];
     
  //Create socket
  socket_desc = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc == -1)
  {
    printf("Could not create socket\n");
  }

  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(5001);
     
  //Bind
  if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
  {
    //print the error message
    printf("bind failed. Error\n");
    exit(-1);
  }
     
  //Listen
  listen(socket_desc, 3);
     
  //Accept and incoming connection
  printf("Waiting for incoming connections...\n");
  socket_size = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&socket_size);
    if (client_sock < 0)
    {
      printf("accept failed\n");
      exit(-1);
    }
    printf("Connection accepted\n");
     
    //Receive a message from client
    if((read_size = recv(client_sock, &client_message, sizeof(client_message), 0)) > 0)
    {
      printf("Received messages from clients: %d with request: %d\n" , client_sock, client_message.ublx_client_func);      
    }

    if(read_size == 0)
    {
      printf("Client disconnected\n");
      fflush(stdout);
      close(client_sock);
    }
    else if(read_size == -1)
    {
      printf("recv failed\n");
    }

    if(client_message.ublx_client_func == UBLX_SEND_IP_ADDR_VIA_SMS)
    {
      if(open_modem() < 0)
      {
        exit(1);
      }
      ublx_modem_get_ipaddr(&client_message, msg);
      ublx_modem_send_sms(&client_message, msg);
//      ublx_modem_send_at(&client_message, msg);
      close(fd);
    }
    
    strncpy(client_message.ublx_client_reply_msg, msg, UBLX_CLIENT_MSG_LEN);

    if((read_size = send(client_sock, &client_message, sizeof(client_message), 0)) > 0)
    {
      printf("Reply client message: %d with result: %s\n" , client_sock, client_message.ublx_client_reply_msg);
    }
    else
    {
      printf("Reply client message: %d fail\n" , client_sock);
    }
    
  close(client_sock);
}

int main(void)
{
   ublx_server_create();
}


/*
int main(int argc, char *argv[])
{
  cmd_t cmd;
  
  memset(&cmd, 0, sizeof(cmd));

  if (argc < 2)
  {
    printf("No input parameters\n");
    exit(1);
  }

  strncpy(cmd.cmd_name, argv[1], strlen(argv[1]));

  if(argv[2])
  {
     strncpy(cmd.cmd_msg, argv[2], strlen(argv[2]));
  }

  printf("Command to be sent to serial port: %s\n", cmd.cmd_name);
  
  if(open_modem() < 0)
  {
    exit(1);
  }

  send_cmd_to_modem(fd, &cmd);

  sleep(1);

  recv_data_from_modem(fd);

  close(fd);

  exit(0);
}
*/


