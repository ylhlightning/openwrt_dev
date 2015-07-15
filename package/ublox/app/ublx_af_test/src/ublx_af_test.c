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
 * @file ublx_at_server.c
 * 
 * @brief ublx at server for ubus communication.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libubox/ustream.h>
#include <libubox/blobmsg_json.h>
#include "libubus.h"

#define UBLX_AF_INVOKE_TIMEOUT 120000
#define MSG_LEN 1024

typedef int (*ublx_api_func)(uint32_t, char *, int);

typedef struct ublx_api
{
  char name[128];
  ublx_api_func api_func;
} ublx_api_t;

static struct blob_buf b;
static int is_async_call = 0;
static char *pin = "9754";
static char *num = "3920635677";
static char *ubus_object = "ublxaf";
static int ublx_test_timeout = 0;

static int ublx_unlock_sim(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_net_home(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_net_list(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_send_sms(uint32_t id, char *ubus_method, int is_async_call);

ublx_api_t ublx_api_table[] = {
  {"unlock_sim", ublx_unlock_sim},
  {"net_list", ublx_net_list},
  {"net_home", ublx_net_home},
  {"send_sms", ublx_send_sms}
};

int api_num = sizeof(ublx_api_table)/sizeof(ublx_api_t);

static int str_replace(char* str,char* str_src, char* str_des){
  char *ptr=NULL;
  char buff[MSG_LEN];
  char buff2[MSG_LEN];
  int i = 0;
 
  if(str != NULL){
    strcpy(buff2, str);
  }
  else
  {
    printf("str_replace err!/n");
    return -1;
  }

  memset(buff, 0x00, sizeof(buff));

  while((ptr = strstr( buff2, str_src)) !=0){
    if(ptr-buff2 != 0) memcpy(&buff[i], buff2, ptr - buff2);
    memcpy(&buff[i + ptr - buff2], str_des, strlen(str_des));
    i += ptr - buff2 + strlen(str_des);
    strcpy(buff2, ptr + strlen(str_src));
  }
  strcat(buff,buff2);
  strcpy(str,buff);
  return 0;
}

static void string_parsing(char *string)
{
  char string_parsed [MSG_LEN];
  char *str_ret = NULL;
  strcpy(string_parsed, string);
  str_replace(string_parsed, "\\\\r\\\\r\\\\n", "\n");
  str_replace(string_parsed, "\\\\\\\"", "\"");
  str_replace(string_parsed, "\\\\r\\\\n", "\n");
  str_ret = strstr(string_parsed, "at+");
  str_replace(string_parsed, "}", "");
  str_replace(string_parsed, "\\\"\\n\"", "");
  printf("%s\n", str_ret);
}

static struct ubus_context *ubus_init(uint32_t *id, char *ubus_object, int is_async_call)
{
  const char *ubus_socket = NULL;
  struct ubus_context *ctx;

  /* send a request with ubusd. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    return NULL;
  }

  /* ubus lookup object */
  if (ubus_lookup_id(ctx, ubus_object, id)) {
    printf("Failed to look up test object\n");
    return NULL;
  }

  if(is_async_call == 1)
  {
    /* add this ubus instance into uloop. */
    ubus_add_uloop(ctx);
  }

  return ctx;
}

static void test_client_complete_cb(struct ubus_request *req, int ret)
{
  if(ret == 0)
  {
    printf("Completed asynchronous call request: OK.\n");
  }
  else
  {
    printf("Completed asynchronous call request: FAIL.\n");
  }

  printf("\n*************************************************************************\n\n\n");
}

static void receive_call_result_sync(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("\n********************** Receive synchrnous call result. ********************\n");

  string_parsing(str);

  free(str);
}

static void receive_call_result_async(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("\n\n\n********************** Receive asynchrnous call result. ********************\n");
  string_parsing(str);
  free(str);
}


static void add_method_parameter(char *ubus_method)
{
  if(!strcmp(ubus_method, "unlock_sim"))
  {
    blobmsg_add_string(&b, "pin", pin);
  }

  if(!strcmp(ubus_method, "send_sms"))
  {
    blobmsg_add_string(&b, "number", num);
  }
}

static void ubus_sync_call(struct ubus_context *ctx, uint32_t id, char *ubus_method)
{
  printf("\nSynchronized method call with timeout: %d seconds.\n", ublx_test_timeout/1000);

  /* init message buffer */
  blob_buf_init(&b, 0);

  add_method_parameter(ubus_method);

  /* invoke a ubus call */
  ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result_sync, 0, UBLX_AF_INVOKE_TIMEOUT);
}

static void ubus_async_call(struct ubus_context *ctx, uint32_t id, char *ubus_method)
{
  printf("\nAsynchronized method call.\n");

  struct ubus_request *req = (struct ubus_request*)malloc(sizeof(struct ubus_request));

  /* init message buffer */
  blob_buf_init(&b, 0);

  add_method_parameter(ubus_method);

  /* invoke an asynchronized ubus call. */
  ubus_invoke_async(ctx, id, ubus_method, b.head, req);

  /* install handle callback function. */
  req->data_cb = receive_call_result_async;
  req->complete_cb = test_client_complete_cb;

  /*Mark this ubus instance as asychronized method.*/
  ubus_complete_request_async(ctx, req);
}

static int ublx_unlock_sim(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(is_async_call == 1)
  {
/*Asynchronized method call example*/
    ubus_async_call(ctx, id, ubus_method);
  }
  else
  {
/*Synchronized method call example*/
    ubus_sync_call(ctx, id, ubus_method);
    ubus_free(ctx);
  }

  return 1;
}

static int ublx_net_list(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(is_async_call == 1)
  {
/*Asynchronized method call example*/
    ubus_async_call(ctx, id, ubus_method);
  }
  else
  {
/*Synchronized method call example*/
     ubus_sync_call(ctx, id, ubus_method);
     ubus_free(ctx);
  }

  return 1;

}

static int ublx_net_home(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(is_async_call == 1)
  {
/*Asynchronized method call example*/
    ubus_async_call(ctx, id, ubus_method);
  }
  else
  {
/*Synchronized method call example*/
    ubus_sync_call(ctx, id, ubus_method);
    ubus_free(ctx);
  }

  return 1;

}

static int ublx_send_sms(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(is_async_call == 1)
  {
/*Asynchronized method call example*/
    ubus_async_call(ctx, id, ubus_method);
  }
  else
  {
/*Synchronized method call example*/
    ubus_sync_call(ctx, id, ubus_method);
    ubus_free(ctx);
  }

  return 1;

}


int main(int argc, char *argv[])
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  int ch;
  int i;
  char *name;

  while ((ch = getopt(argc, argv, "hat:")) != -1) {
    switch (ch) {
    case 'a':
      is_async_call = 1;
      break;
    case 't':
      ublx_test_timeout = atoi(optarg);
    case 'h':
      printf("Please specify the parameters.\n");
      printf("-a call api function in asychronous mode.\n");
      printf("-t specify the sychronous call method timeout, default value is 120s.\n");
    default:
      break;
    }
  }

  if(!ublx_test_timeout)
  {
    ublx_test_timeout = UBLX_AF_INVOKE_TIMEOUT;
  }
  else
  {
    ublx_test_timeout = ublx_test_timeout * 1000;
  }
  
  ublx_api_t *ublx_api_table_ptr = ublx_api_table;

  for(i = 0; i < api_num; i++)
  {
    name = ublx_api_table_ptr->name;
    ublx_api_table_ptr->api_func(id, name, is_async_call);
    ublx_api_table_ptr ++;
    sleep(1);
  }

  if(is_async_call == 1)
  {
    uloop_run();
    uloop_done();
  }

  return 0;

}


