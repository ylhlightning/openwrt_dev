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

char ublx_wwan_public_ip_addr_msg[UBLX_CLIENT_MSG_LEN];

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

static void receive_call_result_address(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, simple_output ? -1 : 0);

  memset(ublx_wwan_public_ip_addr_msg, 0, strlen(ublx_wwan_public_ip_addr_msg));

  strncpy(ublx_wwan_public_ip_addr_msg, str, strlen(str));

  free(str);
}

static int ubus_invoke_do(struct ubus_context *ctx, uint32_t obj, const char *method,
                struct blob_attr *msg, ubus_data_handler_t cb, void *priv, int timeout)
{
  return ubus_invoke(ctx, obj, method, msg, cb, priv, timeout);
}


static int client_ubus_process(char *ubus_object, char *ubus_method, char *argv)
{
  static struct ubus_request req;
  uint32_t id;
  int ret, ret_ubus_invoke;
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

  if(argv != NULL)
  {
    if(!blobmsg_add_json_from_string(&b, argv)) {
      printf("Failed to parse message data\n");
      return -1;
    }
  }

  if(strcmp(ubus_method, "getaddr") == 0)
  {
    ret_ubus_invoke = ubus_invoke_do(ctx, id, ubus_method, b.head, receive_call_result_address, 0, 3000);
  }
  else
  {
    ret_ubus_invoke = ubus_invoke_do(ctx, id, ubus_method, b.head, receive_call_result_data, 0, 3000);
  }

  if(ret_ubus_invoke == 0 || ret_ubus_invoke == 7)
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

int wwan_connection_open(char *argv)
{
  int ret;
  char *wwan_object = "wwan";
  char *wwan_method_enable = "enable";
  char *wwan_method_connect = "connect";

  printf("Enter UBLX_WWAN_OPEN_CONNECTION handler function.\n");

  ret = client_ubus_process(wwan_object, wwan_method_enable, NULL);
  if(ret == FALSE)
    return FALSE;
  
  ret = client_ubus_process(wwan_object, wwan_method_connect, NULL);
  if(ret == FALSE)
    return FALSE;

  return TRUE;
}

int wwan_get_addr(char *argv)
{
  int ret;
  char *wwan_object = "wwan";
  char *wwan_method_get_addr = "getaddr";

  printf("Enter UBLX_WWAN_GET_ADDR handler function.\n");

  ret = client_ubus_process(wwan_object, wwan_method_get_addr, NULL);
  if(ret == FALSE)
    return FALSE;

  return TRUE;
}

int wwan_send_addr(char *argv)
{
  int ret;
  char *wwan_object = "wwan";
  char *wwan_method_get_addr = "sendaddr";

  printf("Enter UBLX_WWAN_SEND_ADDR handler function.\n");

  ret = client_ubus_process(wwan_object, wwan_method_get_addr, argv);

  if(ret == FALSE)
    return FALSE;

  return TRUE;
}



