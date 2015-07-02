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

static void test_client_fd_cb(struct ubus_request *req, int fd)
{
  printf("Got fd from the server\n");
}

static void test_client_complete_cb(struct ubus_request *req, int ret)
{
  printf("completed request, ret: %d\n", ret);
}

static void receive_call_result(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  printf("%s\n", str);
  free(str);
}

void ubus_sync_call(uint32_t id, char *ubus_method)
{
  printf("Synchronized method call.\n");
  /* init message buffer */
  blob_buf_init(&b, 0);

  /* invoke a ubus call */
  ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result, 0, 3000);

}

void ubus_async_call(uint32_t id, char *ubus_method)
{
  printf("Asynchronized method call.\n");

  static struct ubus_request req;

  blob_buf_init(&b, 0);
  ubus_invoke_async(ctx, id, ubus_method, b.head, &req);
  req.data_cb = receive_call_result;
  req.complete_cb = test_client_complete_cb;
  ubus_complete_request_async(ctx, &req);

  uloop_run();
}


int main(int argc, char *argv[])
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  const char *ubus_socket = NULL;
  char *ubus_method = "hello";
  int ch;

  while ((ch = getopt(argc, argv, "as:")) != -1) {
    switch (ch) {
    case 's':
      ubus_socket = optarg;
      break;
    case 'a':
      is_async_call = 1;
      break;
    default:
      break;
    }
  }

  /* send a request with ubusd. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    exit(1);
  }

  /* ubus lookup object */
  if (ubus_lookup_id(ctx, "test", &id)) {
    printf("Failed to look up test object\n");
    exit(1);
  }

  ubus_add_uloop(ctx);

  if(is_async_call == 1)
  {
    ubus_async_call(id, ubus_method);
  }
  else
  {
    ubus_sync_call(id, ubus_method);
  }

  /* free ubus structure */
  ubus_free(ctx);
  uloop_done();
  
  return 0;
}
