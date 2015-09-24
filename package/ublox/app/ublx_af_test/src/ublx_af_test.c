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
#define UBLX_AF_PATH "ublxaf"
#define DEFAULT_PIN "1605";

static struct ubus_subscriber ublxaf_test_event;

typedef int (*ublx_api_func)(uint32_t, char *, int);

typedef struct ublx_api
{
  char name[128];
  ublx_api_func api_func;
} ublx_api_t;

static struct blob_buf b;
static int is_async_call = 0;
static char *num = "3920635677";
static char *ubus_object = "ublxaf";
static char *pin = NULL;
static int ublx_test_timeout = 0;

static int ublx_unlock_sim(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_net_home(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_net_list(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_net_connect(uint32_t id, char *ubus_method, int is_async_call);

static int ublx_send_sms(uint32_t id, char *ubus_method, int is_async_call);

ublx_api_t ublx_api_table[] = {
  {"unlock_sim",  ublx_unlock_sim},
  {"net_list",    ublx_net_list},
  {"net_home",    ublx_net_home},
  {"net_connect", ublx_net_connect},
  {"send_sms",    ublx_send_sms}
};

int api_num = sizeof(ublx_api_table)/sizeof(ublx_api_t);

static void timestamp()
{
    struct timeval detail_time;
    struct tm *Tm;
    time_t ltime;

    ltime=time(NULL);

    Tm=localtime(&ltime);

    gettimeofday(&detail_time,NULL);

    printf("\nTimestamp: [%d:%d:%d.%d%d]\n\n",
            Tm->tm_hour,
            Tm->tm_min,
            Tm->tm_sec,
            detail_time.tv_usec /1000,  /* milliseconds */
            detail_time.tv_usec); /* microseconds */
}


static int str_replace(char* str, char* str_src, char* str_des){
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
  int at_format_plus = 0;
  int at_format_esclamation = 0;

  if(string == NULL)
  {
    return;
  }

  if(strstr(string, "at+") != NULL)
  {
    at_format_plus = 1;
  }

  if(strstr(string, "at!") != NULL)
  {
    at_format_esclamation = 1;
  }

  if(at_format_plus == 0 && at_format_esclamation == 0)
  {
    printf("OK\n\n\n");
    return;
  }
  
  strcpy(string_parsed, string);
  str_replace(string_parsed, "\\\\r\\\\r\\\\n", "\n");
  str_replace(string_parsed, "\\\\\\\"", "\"");
  str_replace(string_parsed, "\\\\r\\\\n", "\n");
  str_replace(string_parsed, "\\\\n", "\n");

  if(at_format_plus)
  {
    str_ret = strstr(string_parsed, "at+");
  }
  else if(at_format_esclamation)
  {
    str_ret = strstr(string_parsed, "at!");
  }
  else
  {
    printf("OK\n\n\n");
    return;
  }

  str_replace(string_parsed, "}", "");
  str_replace(string_parsed, "\\\"\\n\"", "");
  printf("%s\n", str_ret);
}

static void
ublxaf_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s,
                   uint32_t id)
{
  printf("Object %08x went away....exit\n", id);

  if(is_async_call)
  {
    uloop_cancelled = true;
  }
  else
  {
    exit(1);
  }
}

static int
ublxaf_notify(struct ubus_context *ctx, struct ubus_object *obj,
            struct ubus_request_data *req, const char *method,
            struct blob_attr *msg)
{
  char *str;

  str = blobmsg_format_json(msg, true);

  printf("Received notification '%s': %s\n", method, str);

  free(str);

  return 0;
}

static int ublx_ubus_subscribe(char *ubus_socket)
{
  uint32_t id;
  int ret = 0;
  struct ubus_context *ctx;

  ublxaf_test_event.remove_cb = ublxaf_handle_remove;
  ublxaf_test_event.cb = ublxaf_notify;

  /* send a request with ubusd. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    return 1;
  }

  ret = ubus_lookup_id(ctx, UBLX_AF_PATH, &id);
  if (ret) {
    printf("[%s] ubus_lookup_id error %d\n", __FUNCTION__, ret);
    return ret;
  }

  ret = ubus_register_subscriber(ctx, &ublxaf_test_event);
  if (ret) {
    printf("[%s] ubus_register_subscriber error %d\n", __FUNCTION__, ret);
    return ret;
  }
  printf("ubus_register_subscriber OK\n");

  ret = ubus_subscribe(ctx, &ublxaf_test_event, id);
  if (ret) {
    printf("[%s] ubus_subscribe error %d\n", __FUNCTION__, ret);
    ubus_unsubscribe(ctx, &ublxaf_test_event, id);
    return ret;
  }
  printf("ubus_subscribe OK\n");

  return 0;
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

  printf("\n***********************************************************************************\n\n\n\n\n\n");
}

static void receive_call_result_sync(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("\n!!! Attention: Received synchrnous call result:\n\n\n");

  if(strstr(str, "OK"))
  {
    string_parsing(str);
  }
  else
  {
    printf("ERROR.\n");
  }
  printf("\n***********************************************************************************\n\n\n\n\n\n"); 
}

static void receive_call_result_async(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }

  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("\n********************** Receive synchrnous call result. ********************\n");

  string_parsing(str);
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
  printf("\nubus synchronized method call with timeout: %d seconds.\n", ublx_test_timeout/1000);

  timestamp();

  /* init message buffer */
  blob_buf_init(&b, 0);

  add_method_parameter(ubus_method);

  printf("\n\nubus sychronous call blocking and wait for remote reply...\n\n\n");

  /* invoke a ubus call */
  ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result_sync, 0, ublx_test_timeout);
}

static void ubus_async_call(struct ubus_context *ctx, uint32_t id, char *ubus_method)
{
  printf("\nubus asynchronized method call.\n");

  timestamp();

  struct ubus_request *req = (struct ubus_request*)malloc(sizeof(struct ubus_request));

  /* init message buffer */
  blob_buf_init(&b, 0);

  add_method_parameter(ubus_method);

  printf("\n\nubus asychronous call and return immediately\n\n\n");

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

  if(!ctx)
  {
    printf("Lookup for Ubus object failed....exit\n");
    return -1;
  }

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

  return 0;
}

static int ublx_net_list(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(!ctx)
  {
    printf("Lookup for Ubus object failed....exit\n");
    return -1;
  }


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

  return 0;

}

static int ublx_net_home(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(!ctx)
  {
    printf("Lookup for Ubus object failed....exit\n");
    return -1;
  }


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

  return 0;

}


static int ublx_net_connect(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(!ctx)
  {
    printf("Lookup for Ubus object failed....exit\n");
    return -1;
  }

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

  return 0;

}


static int ublx_send_sms(uint32_t id, char *ubus_method, int is_async_call)
{
  printf("\n\n\n***************** Start to call %s api function. ********************\n", __FUNCTION__);

  struct ubus_context *ctx = ubus_init(&id, ubus_object, is_async_call);

  if(!ctx)
  {
    printf("Lookup for Ubus object failed....exit\n");
    return -1;
  }

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

  return 0;

}


int main(int argc, char *argv[])
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  int ch;
  int i;
  char *name = NULL;
  char *ubus_sock = NULL;

  while ((ch = getopt(argc, argv, "hat:s:p:")) != -1) {
    switch (ch) {
    case 'a':
      is_async_call = 1;
      break;
    case 't':
      ublx_test_timeout = atoi(optarg);
      break;
    case 'p':
      pin = optarg;
      break;
    case 's':
      ubus_sock = optarg;
      break;
    case 'h':
      printf("Please specify the parameters.\n");
      printf("-a call api function in asychronous mode.\n");
      printf("-t specify the sychronous call method timeout, default value is 120s.\n");
      exit(0);
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

  if(!pin)
  {
    pin = DEFAULT_PIN;
  }

  if(ublx_ubus_subscribe(ubus_sock) != 0)
  {
    printf("ublx af process ubus subscription error.\n");
    exit(1);
  }

  printf("\n\n*****************************************\n");
  printf("**** Start to run ublx process Test. ****\n");
  printf("*****************************************\n\n");

  printf("ubus invoke sequence:\n\n");
  printf("1. unlock_sim\n");
  printf("2. net_list\n");
  printf("3. net_home\n");
  printf("4. net_connect\n");
  printf("5. send_sms\n\n\n\n");
  printf("Please press any keys to continue.....\n\n\n");

  getchar();
  
  ublx_api_t *ublx_api_table_ptr = ublx_api_table;

  for(i = 0; i < api_num; i++)
  {
    name = ublx_api_table_ptr->name;

    if(ublx_api_table_ptr->api_func(id, name, is_async_call) < 0)
    {
      printf("Fatal error happens....exit.\n");
      exit(1);
    }

    ublx_api_table_ptr ++;

    if(i < api_num - 1)
    {
      printf("Wait 10 seconds for next ubus call invoke: %s\n\n\n", ublx_api_table_ptr->name);
    }

    sleep(10);
  }

  if(is_async_call == 1)
  {
    printf("\n\n\n!!!Asychronous call method run in loop and wait for remote message callback.....\n\n\n");
    uloop_run();
    uloop_done();
  }

  return 0;

}


