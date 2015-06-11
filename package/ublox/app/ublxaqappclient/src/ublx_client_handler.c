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
 * @file ublx_client_handler.c
 * 
 * @brief ublx aqapp server for ubus communication.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include "ublx_client_handler.h"

static struct ubus_context *ctx;
static struct blob_buf b;
static bool simple_output = false;

/************************************************************
* Function implementations.
************************************************************/

static void test_client_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
  printf("Subscribers active: %d\n", obj->has_subscribers);
}

static struct ubus_object test_client_object = {
  .subscribe_cb = test_client_subscribe_cb,
};

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, simple_output ? -1 : 0);

  printf("%s\n", str);
  free(str);
}

static int client_ubus_process(char *ubus_object, char *ubus_method)
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  const char *ubus_socket = NULL;

  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    return FALSE;
  }

  ret = ubus_add_object(ctx, &test_client_object);
  if (ret) {
    printf("Failed to add_object object: %s\n", ubus_strerror(ret));
    return FALSE;
  }

  if (ubus_lookup_id(ctx, ubus_object, &id)) {
    printf("Failed to look up test object\n");
    return FALSE;
  }

  blob_buf_init(&b, 0);
  blobmsg_add_u32(&b, "id", test_client_object.id);

  if(ubus_invoke(ctx, id, ubus_method, b.head, receive_call_result_data, 0, 3000) == 0)
  {
    ret = TRUE;
  }
  else
  {
    ret = FALSE;
  }

  ubus_free(ctx);
  return ret;
}

int wwan_connection_open(void)
{
  int ret;
  char *wwan_object = "wwan";
  char *wwan_method_enable = "enable";
  char *wwan_method_connect = "connect";

  ret = client_ubus_process(wwan_object, wwan_method_enable);
  if(ret == FALSE)
    return FALSE;
  
  ret = client_ubus_process(wwan_object, wwan_method_connect);
  if(ret == FALSE)
    return FALSE;

  return TRUE;
}

int wwan_get_addr(void)
{
  int ret;
  char *wwan_object = "wwan";
  char *wwan_method_get_addr = "getaddr";

  ret = client_ubus_process(wwan_object, wwan_method_get_addr);
  if(ret == FALSE)
    return FALSE;

  return TRUE;
}


