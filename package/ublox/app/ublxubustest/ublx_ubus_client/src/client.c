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
 * @file client.c
 * 
 * @brief ubus client application template.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#include <sys/time.h>
#include <unistd.h>

#include <libubox/ustream.h>

#include "libubus.h"

static struct ubus_context *ctx;
static struct blob_buf b;
static char *ubus_object = "test";
static char *ubus_method = "hello";
static int is_async_call = 0;

/* Asynchronous callback to notify that the method call has been finished. */
static void test_client_complete_cb(struct ubus_request *req, int ret)
{
  printf("completed request, ret: %d\n", ret);
}

/* Asynchronous user defined callback, to be invoked by usbud to pass the results of method call. */
static void receive_call_result(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }

  /* Parsing ubus message from json format.*/
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("%s\n", str);
  free(str);
}

void ubus_sync_call(uint32_t id, char *ubus_method)
{
  printf("Synchronized method call.\n");
  /* init a blob joson message buffer, where contains the user parameters.*/
  blob_buf_init(&b, 0);

  /* invoke a ubus call */
  ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result, 0, 3000);
}

void ubus_async_call(uint32_t id, char *ubus_method)
{
  printf("Asynchronized method call.\n");

  static struct ubus_request req;

  /* mark ubus context as asynchronize call method.*/
  ubus_add_uloop(ctx);

  /* init a blob joson message buffer, where contains the user parameters.*/
  blob_buf_init(&b, 0);

  /* invoke an asynchronized ubus call. */
  ubus_invoke_async(ctx, id, ubus_method, b.head, &req);

  /* install handle callback function, to be called once some event happens. */
  req.data_cb = receive_call_result;
  req.complete_cb = test_client_complete_cb;

  /*Mark this instance as asychronized method.*/
  ubus_complete_request_async(ctx, &req);

  /* application run in infinite loop and polling for some UBUS events comes */
  uloop_run();
}


int main(int argc, char *argv[])
{
  uint32_t id;
  int ret;
  const char *ubus_socket = NULL;
  int ch;

  while ((ch = getopt(argc, argv, "as:")) != -1) {
    switch (ch) {
    case 's':
    /*pass a user specified Unix domain socket path.*/
      ubus_socket = optarg;
      break;
    case 'a':
      is_async_call = 1;
      break;
    default:
      break;
    }
  }

  /* send a request to ubusd. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    exit(1);
  }

  /* ubus lookup object, return ubus registed id if succeed.*/
  if (ubus_lookup_id(ctx, ubus_object, &id)) {
    printf("Failed to look up test object\n");
    exit(1);
  }

  if(is_async_call == 1)
  {
    /*Asynchronized method call example*/
    ubus_async_call(id, ubus_method);
  }
  else
  {
    /*Synchronized method call example*/
    ubus_sync_call(id, ubus_method);
  }

  if(is_async_call == 1)
  {
    uloop_done();
  }

  /* free ubus structure */
  ubus_free(ctx);

  return 0;
}


