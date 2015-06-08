/*************************************************************************
    > File Name: lib320u.h
    > Author: Linhu Ying
    > Mail:  linhu.ying@u-blox.com
    > Created Time: Thu Jun  4 13:47:11 2015
 ************************************************************************/

#ifndef __LIB_320U_H__
#define __LIB_320U_H__

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

#define CMD_MAX_LEN 128
#define CMD_MSG_MAX_LEN 1024
#define USB_BUFFER 1024
#define CRTL_Z 26
#define WAIT_MODEM_ANSWER_MAX_TIMEOUT 120

#define TRUE 0
#define FALSE -1

/*
 * @Function name: open_modem 
 * 
 * @Description: Open a modem interface
 *
 * @param char * modem_name  [in]  Modem USB interface name.
 * 
 * @return int * fd          [out] Modem USB interface handler.
 *         @value >0               Success
 *         @value -1               Fail               
 *                           
 */

int open_modem(char *modem_port);

/*
 * @Function name: send_cmd_to_modem
 *
 * @Description: send AT command to modem
 *   
 * @param char * fd               [in]  modem usb interface handler.
 * @param char * cmd_name         [in]  AT command name  
 * 
 * @return @type int              [out] boolean value.
 *         @value 0               Success
 *         @value 1               Fail           
 *                           
 */

int send_cmd_to_modem(int fd, char *cmd_name);

/*
 * function name: receive message from modem after send a at command
 *   
 * @param char * modem_name       [in]  modem usb interface name.
 * @param char * modem_reply_msg  [in]  modem reply message of at command.
 * 
 * @return @type int              [out] boolean value.
 *         @value 0               Success
 *         @value 1               Fail           
 *                           
 */

int recv_data_from_modem(int fd, int *cmd_result, char *modem_reply_msg);

/*
 * @Function name: close_modem
 *
 * @Description: receive message from modem after send a at command
 *   
 * @param char * modem_name         [in]  modem usb interface name.
 * @param char * modem_reply_msg  [in]  modem cmd execution result pointer.
 * @param char * modem_reply_msg  [in]  modem reply message of at command.
 * 
 * @return @type int              [out] boolean value.
 *         @value 0               Success
 *         @value 1               Fail           
 *                           
 */

int close_modem(int fd);

#endif


