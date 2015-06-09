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
 * @file ublx_aqapp_client.c
 * 
 * @brief ublx aqapp clent for ubus communication.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#include "ublx_aqapp_client.h"

static struct ubus_context *ctx;
static struct blob_buf b;
static bool simple_output = false;

static void test_client_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
  fprintf(stderr, "Subscribers active: %d\n", obj->has_subscribers);
}


static struct ubus_object test_client_object = {
  .subscribe_cb = test_client_subscribe_cb,
};

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
    return;

  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, simple_output ? -1 : 0);
  
  printf("%s\n", str);
  free(str);
}

static void client_main(void)
{
  static struct ubus_request req;
  uint32_t id;
  int ret;

  ret = ubus_add_object(ctx, &test_client_object);
  if (ret) {
    fprintf(stderr, "Failed to add_object object: %s\n", ubus_strerror(ret));
    return;
  }

  if (ubus_lookup_id(ctx, "wwan", &id)) {
    fprintf(stderr, "Failed to look up test object\n");
    return;
  }

  blob_buf_init(&b, 0);
  blobmsg_add_u32(&b, "id", test_client_object.id);
  ubus_invoke(ctx, id, "enable", b.head, receive_call_result_data, 0, 3000);
}

int main(int argc, char **argv)
{
  const char *ubus_socket = NULL;
  int ch;

  while ((ch = getopt(argc, argv, "cs:")) != -1) {
    switch (ch) {
    case 's':
      ubus_socket = optarg;
      break;
    default:
      break;
    }
  }

  argc -= optind;
  argv += optind;

  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    fprintf(stderr, "Failed to connect to ubus\n");
    return -1;
  }

  client_main();

  ubus_free(ctx);

  return 0;
}

