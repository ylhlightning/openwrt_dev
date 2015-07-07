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

#include <libubox/blobmsg_json.h>
#include "libubus.h"

char ubus_object[128] = "ublxat";
char ubus_method[128] = "at_send_cmd";
char cmd[128] = "at!scpaddr=1";

static struct blob_buf b_local;

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
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


static int client_ubus_process(char *ubus_object, char *ubus_method, char *argv)
{
  static struct ubus_request req;
  uint32_t id;
  int ret, ret_ubus_invoke;
  const char *ubus_socket = NULL;
  struct ubus_context *ctx_local;

  ctx_local = ubus_connect(ubus_socket);
  if (!ctx_local) {
    printf("Failed to connect to ubus\n");
    return -1;
  }

  if (ubus_lookup_id(ctx_local, ubus_object, &id)) {
    printf("Failed to look up test object\n");
    return -1;
  }
  blob_buf_init(&b_local, 0);

  blobmsg_add_string(&b_local, "cmd", argv);

  ret_ubus_invoke = ubus_invoke(ctx_local, id, ubus_method, b_local.head, receive_call_result_data, 0, 3000);
 
  if(ret_ubus_invoke == 0 || ret_ubus_invoke == 7)
  {
    ret = 0;
  }
  else
  {
    ret = -1;
  }

  ubus_free(ctx_local);
  return ret;
}



int main()
{
  client_ubus_process(ubus_object, ubus_method, cmd);
  return 0;
}

