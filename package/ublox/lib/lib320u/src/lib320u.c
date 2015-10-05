/*************************************************************************
    > File Nime: serial.c
    > Author: Linhu Ying
    > Mail:  linhu.ying@u-blox.com
    > Created Time: Mon May 18 19:12:08 2015
 ************************************************************************/

#include "lib320u.h"

static timer_t gTimerid;
static struct itimerspec timer;

static char *str_sig_alarm = "sigarlm signal caught!\n";
static char *str_sig_vtalarm = "sigvtalrm signal caught!\n";
static char *str_sig_timeout = "AT command wait timeout!\n";
static char *str_sig_int = "sigint signal caught!\n";

static int modem_fd = 0;

/************************************************/

// Filter all unecessasry /n in the front or at the end of string.
static void string_filter(char *str)
{
  int len = strlen(str);
  int i, j, k, start_byte, stop_byte;
  char *ch = str;
  char *ch_tmp = str;
  char *ch_start = NULL;

  for(i = 0; i < len; i++)
  {
    if(*ch == '\n')
    {
      ch ++;
      continue;
    }
    start_byte = i;
    ch_start = ch;
    break;
  }
  
#ifdef DEBUG_FLAG
  printf("Start byte: %d\n", start_byte);
#endif

  for(k = 0; k < len - start_byte -1; k++)
  {
    ch ++;
  }

  for(j = len - 1; j >= 0; j--)
  {
    if(*ch == '\n')
    {
      ch --;
      continue;
    }
    stop_byte = j;
    break;
  }

#ifdef DEBUG_FLAG
  printf("stop byte: %d\n", stop_byte);
#endif

  if(stop_byte >= start_byte)
  {
    for(k = 0; k < (stop_byte - start_byte + 1); k++)
    {
      *ch_tmp = *ch_start;
      ch_tmp ++;
      ch_start ++;
    }
    *ch_tmp = '\0';
  }
  else
  {
#ifdef DEBUG_FLAG
    printf("A string all with \n, reset all.\n");
#endif
    memset(str, '\0', strlen(str));
  }
#ifdef DEBUG_FLAG
  printf("Filtered string:%s, length:%d\n", str, strlen(str));
#endif
}


/*static internal use function */
static void timer_handler(int signum)
{
  switch(signum) {
    case SIGALRM:
      write(STDOUT_FILENO, str_sig_alarm, strlen(str_sig_alarm));
      break;
    case SIGVTALRM:
      write(STDOUT_FILENO, str_sig_vtalarm, strlen(str_sig_vtalarm));
      break;
    default:
      break;
  }
  write(STDOUT_FILENO, str_sig_timeout, strlen(str_sig_timeout));
  exit(1);
}

static void interrupt_handler(int signum)
{
  switch(signum)
  {
    case SIGINT:
      write(STDOUT_FILENO, str_sig_int, strlen(str_sig_int));
      break;
    default:
      break;
  }
  exit(1);
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

/************************************************/
/*export function */

int open_modem(char *modem_port)
{
  struct termios options;

  if(modem_port == NULL)
  {
    printf("[%s]: Modem Device name is NULL.\n", __FUNCTION__);
    return FALSE;
  }
  
  /* open the port */
  int fd = open(modem_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd == -1)
  {
    printf("[%s]: Unable to open modem port %s, error: %s\n", __FUNCTION__, modem_port, strerror(errno));
    return FALSE;
  }
  else
  {
    printf("Succeed to open modem port %s\n", modem_port); 
    modem_fd = fd;
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

  if(fd < 0)
  {
    printf("[%s]: Error: Invalide device handler.\n", __FUNCTION__);
    return FALSE;
  }

  if(cmd_name == NULL)
  {
    printf("[%s]: Error: Invalid command name.\n", __FUNCTION__);
    return FALSE;    
  }

  if(strlen(cmd_name) > CMD_MAX_LEN)
  {
    printf("[%s]: Error: AT cmd name exceed the maximum length.\n", __FUNCTION__);
    return FALSE;
  }

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

  if(fd < 0)
  {
    printf("[%s]: Error: Invalide device handler.\n", __FUNCTION__);
    return FALSE;
  }

  if(modem_reply_msg == NULL)
  {
    printf("[%s]: Error: modem_reply_msg is NULL.\n", __FUNCTION__);
    return FALSE;    
  }

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
          printf("[%s]: Error: read port error!\n", __FUNCTION__);
          ret = FALSE;
          result = FALSE;
          break;
     }
    }

    if(n == 0)
    {
      printf("[%s]: Error: No data received in serial port buffer.\n", __FUNCTION__);
      ret = FALSE;
      result =FALSE;
      break;
    }

    bufptr += n;

    if(strstr(buffer, ">") != NULL)
    {
      printf("%s\n", buffer);
      strncpy(modem_reply_msg, buffer, USB_BUFFER);
      result = AT_CMD_SMS;
      ret = TRUE;
      break;
    }

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

  string_filter(modem_reply_msg);

  *cmd_result = result;

  modem_answer_timer_stop();
  return ret;
}


int send_sms_to_modem(int fd, char *number, char *sms_msg)
{
  ssize_t write_bytes;
  char cmd_name[CMD_MAX_LEN] = AT_SEND_SMS_CMD;
  size_t cmd_len;
  size_t msg_len = strlen(sms_msg);
  char ctrl_z = '\x1A';
  char ctrl_z_bit[2];

  if(fd < 0)
  {
    printf("[%s]: Error: Invalid device handler.\n", __FUNCTION__);
    return FALSE;
  }

  if(number == NULL)
  {
    printf("[%s]: Error: cellular number is null.\n", __FUNCTION__);
    return FALSE;    
  }

  if(sms_msg == NULL)
  {
    printf("[%s]: Error: sms message is null.\n", __FUNCTION__);
    return FALSE;    
  }

  sprintf(ctrl_z_bit, "%c", ctrl_z);

  strncat(cmd_name, number, strlen(number) + 1);

  cmd_len = strlen(cmd_name);

  strncat(cmd_name, "\r", cmd_len + 1);

  /* send an AT command followed by a CR */
  if ((write_bytes = write(fd, cmd_name, cmd_len + 1)) < cmd_len+1)
  {
    printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
    return FALSE;
  }
  else
  {
    printf("Successful to write %d to modem serial port\n", write_bytes);
  }

  if(cmd_len)
  {
      sleep(2);

      /* send a message */
      if ((write_bytes = write(fd, sms_msg, msg_len + 1)) < msg_len+1)
      {
        printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
        return FALSE;
      }
      else
      {
        printf("Successful to send %d byte messages [%s] to modem serial port\n", write_bytes, sms_msg);
      }

      sleep(1);

      /* send a message followed by CTRL-Z  */
      if ((write_bytes = write(fd, ctrl_z_bit, 1)) < 1)
      {
        printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
        return FALSE;
      }
      else
      {
        printf("Successful to send CTRL-Z\n");
      }

  }

  /*flush all read write buffer */
  tcflush(fd, TCIOFLUSH);

  return TRUE;
}


int send_sms_to_modem_with_cmd(int fd, char *cmd_name, char *sms_msg)
{
  ssize_t write_bytes;
  size_t cmd_len;
  size_t msg_len = strlen(sms_msg);
  char ctrl_z = '\x1A';
  char ctrl_z_bit[2];

  if(fd < 0)
  {
    printf("[%s]: Error: Invalid device handler.\n", __FUNCTION__);
    return FALSE;
  }
  
  if(cmd_name == NULL)
  {
    printf("[%s]: Error: AT command name is null.\n", __FUNCTION__);
    return FALSE;    
  }
  
  if(sms_msg == NULL)
  {
    printf("[%s]: Error: sms message is null.\n", __FUNCTION__);
    return FALSE;    
  }

  sprintf(ctrl_z_bit, "%c", ctrl_z);

  cmd_len = strlen(cmd_name);

  strncat(cmd_name, "\r", cmd_len + 1);

  /* send an AT command followed by a CR */
  if ((write_bytes = write(fd, cmd_name, cmd_len + 1)) < cmd_len+1)
  {
    printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
    return FALSE;
  }
  else
  {
    printf("Successful to write %d to modem serial port\n", write_bytes);
  }

  if(cmd_len)
  {
      sleep(2);

      /* send a message */
      if ((write_bytes = write(fd, sms_msg, msg_len + 1)) < msg_len+1)
      {
        printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
        return FALSE;
      }
      else
      {
        printf("Successful to send %d byte messages [%s] to modem serial port\n", write_bytes, sms_msg);
      }

      sleep(1);

      /* send a message followed by CTRL-Z  */
      if ((write_bytes = write(fd, ctrl_z_bit, 1)) < 1)
      {
        printf("[%s]: write error: %s\n", __FUNCTION__, strerror(errno));
        return FALSE;
      }
      else
      {
        printf("Successful to send CTRL-Z\n");
      }

  }

  /*flush all read write buffer */
  tcflush(fd, TCIOFLUSH);

  return TRUE;
}


int at_send_cmd(char *modem_port_name, char *cmd_name, char *modem_reply_msg)
{
  int ret;
  int cmd_result;
  char recv_msg[CMD_MSG_MAX_LEN];

  if(modem_port_name == NULL)
  {
    printf("[%s]: Modem Device name is NULL.\n", __FUNCTION__);
    return FALSE;
  }

  if(cmd_name == NULL)
  {
    printf("[%s]: AT command name is NULL.\n", __FUNCTION__);
    return FALSE;
  }

  if(modem_reply_msg == NULL)
  {
    printf("[%s]: Modem reply message is NULL.\n", __FUNCTION__);
    return FALSE;
  }

  int fd = open_modem(modem_port_name);
  if(fd < 0)
  {
    return FALSE;
  }

  printf("Command to be sent to serial port: %s\n", cmd_name);
  ret = send_cmd_to_modem(fd, cmd_name);
  if(ret < 0)
  {
    printf("[%s]: Failed to send command to modem\n", __FUNCTION__);
    return FALSE;
  }

  ret = recv_data_from_modem(fd, &cmd_result, recv_msg);
  if(ret < 0)
  {
    printf("[%s]: Failed to receive message from modem\n");
    return FALSE;
  }

  strncpy(modem_reply_msg, recv_msg, strlen(recv_msg));
  close_modem(fd);
  return cmd_result;
}


int close_modem(int fd)
{
   if(close(fd) < 0)
   {
     printf("Unable to open modem, error: %s\n", strerror(errno));
     return FALSE;
   }
   modem_answer_timer_delete();
   modem_fd = 0;
   return TRUE;
}



