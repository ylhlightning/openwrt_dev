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

static void test_client_fd_cb(struct ubus_request *req, int fd)
{
  printf("Got fd from the server, watching...\n");
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

void ubus_sync_call(char *msg, uint32_t id)
{
  /* init message buffer */
  blob_buf_init(&b, 0);

  /* invoke a ubus call */
  ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result, 0, 3000);

}

void ubus_async_call(char *msg, uint32_t id)
{
  static struct ubus_request req;

  blob_buf_init(&b, 0);
  blobmsg_add_string(&b, "msg", "blah");
  ubus_invoke_async(ctx, id, ubus_method, b.head, &req);
  req.fd_cb = test_client_fd_cb;
  req.complete_cb = test_client_complete_cb;
  ubus_complete_request_async(ctx, &req);
  uloop_timeout_set(&count_timer, 2000);
}


int main()
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  const char *ubus_socket = NULL;
  char *msg = "hello";

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

  ubus_sync_call(msg, id);

  sleep(10);

  ubus_async_call(msg, id);

  /* free ubus structure */
  ubus_free(ctx);
  
  return 0;
}
