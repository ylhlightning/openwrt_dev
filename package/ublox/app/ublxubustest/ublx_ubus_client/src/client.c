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


int main()
{
  static struct ubus_request req;
  uint32_t id;
  int ret, ret_ubus_invoke;
  const char *ubus_socket = NULL;
  char *msg = NULL;

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

  /* init message buffer */
  blob_buf_init(&b, 0);
    
  if(msg != NULL)
  {
    if(!blobmsg_add_json_from_string(&b, msg)) {
      printf("Failed to parse message data\n");
      return -1;
    }
  }

  /* invoke a ubus call */
  ret_ubus_invoke = ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result, 0, 3000);

  /* free ubus structure */
  ubus_free(ctx);
  
  return 0;
}
