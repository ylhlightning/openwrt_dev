/*************************************************************************
    > File Nime: serial.c
    > Author: Linhu Ying
    > Mail:  linhu.ying@u-blox.com
    > Created Time: Mon May 18 19:12:08 2015
 ************************************************************************/

#include "lib320u.h"

static timer_t gTimerid;
static struct itimerspec timer;

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
  close(1);
}

static void modem_answer_timer_create(void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &timer_handler;
  sigaction (SIGVTALRM, &sa, NULL);

  sa.sa_handler = &timer_handler;
  sigaction(SIGALRM, &sa, NULL);

  timer_create(CLOCK_REALTIME, NULL, &gTimerid);
}

static void modem_answer_timer_setup(void)
{
  timer.it_value.tv_sec = WAIT_MODEM_ANSWER_MAX_TIMEOUT;
  timer.it_value.tv_nsec = 0;
  timer.it_interval.tv_sec = WAIT_MODEM_ANSWER_MAX_TIMEOUT;
  timer.it_interval.tv_nsec = 0;
  timer_settime(gTimerid, 0, &timer, NULL);
}

static void modem_answer_timer_stop(void)
{
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_nsec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_nsec = 0;
  timer_settime(gTimerid, 0, &timer, NULL);
}

static void modem_answer_timer_delete(void)
{
  timer_delete(gTimerid);
}

static void modem_interrupt_setup(void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &interrupt_handler;
  sigaction (SIGINT, &sa, NULL);
}

int open_modem(char *modem_port)
{
  struct termios options;
  /* open the port */
  int fd = open(modem_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd == -1)
  {
    printf("Unable to open modem port %s, error: %s\n", modem_port, strerror(errno));
    return FALSE;
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

  modem_answer_timer_create();

  modem_interrupt_setup();

  return fd;
}


int send_cmd_to_modem(int fd, char *cmd_name)
{
  ssize_t write_bytes = 0;
  size_t cmd_len = strlen(cmd_name);
  char ctrl_z = '\x1A';
  char ctrl_z_bit[2];

  if(strlen(cmd_name) > CMD_MAX_LEN)
  {
    printf("Error: AT cmd name exceed the maximum length.\n");
    return FALSE;
  }

  sprintf(ctrl_z_bit, "%c", ctrl_z);

  cmd_name = strncat(cmd_name, "\r", cmd_len + 1);

  /* send an AT command followed by a CR */
  if ((write_bytes = write(fd, cmd_name, cmd_len + 1)) < cmd_len+1)
  {
    printf("write error: %s\n",strerror(errno));
    return FALSE;
  }
  else
  {
    printf("Successful to write commands to modem serial port\n");
  }

  /*flush all read write buffer */
  tcflush(fd, TCIOFLUSH);

  modem_answer_timer_setup();

  return TRUE;
}


int recv_data_from_modem(int fd, int *cmd_result, char *modem_reply_msg)
{
  char buffer[USB_BUFFER];  /* Input buffer */
  char *bufptr;      /* Current char in buffer */
  ssize_t n;       /* Number of bytes read */
  int ret;
  int result;

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
          ret = FALSE;
          result = FALSE;
          break;
     }
    }

    if(n == 0)
    {
      printf("No data received in serial port buffer.\n");
      ret = FALSE;
      result =FALSE;
      break;
    }

    bufptr += n;

    if(strstr(buffer, "OK") != NULL)
    {
      *bufptr = '\0';
      printf("%s\n", buffer);
      strncpy(modem_reply_msg, buffer, USB_BUFFER);
      result =TRUE;
      ret = TRUE;
      break;
    }
    
    if(strstr(buffer, "ERROR") != NULL)
    {
      *bufptr = '\0';
      printf("%s\n", buffer);
      strncpy(modem_reply_msg, buffer, USB_BUFFER);
      result =FALSE;
      ret = TRUE;
      break;
    }    
    
  }
  /* terminate the string and see if we got an OK response */
  *cmd_result = result;

  modem_answer_timer_stop();
  return ret;
}


int close_modem(int fd)
{
   if(close(fd) < 0)
   {
     printf("Unable to open modem, error: %s\n", strerror(errno));
     return FALSE;
   }
   modem_answer_timer_delete();
   return TRUE;
}

