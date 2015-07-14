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
 * @file server.c
 * 
 * @brief ubus server application template.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include <unistd.h>
#include <signal.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"

static struct ubus_context *ctx;
static struct ubus_subscriber test_event;
static struct blob_buf b;

enum {
  HELLO_ID,
  HELLO_MSG,
  __HELLO_MAX
};

/* ubus method parameters table with 2 parameters
   id (type int) and msg (type string).
*/
static const struct blobmsg_policy hello_policy[] = {
  [HELLO_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
  [HELLO_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};

static int test_hello(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct blob_attr *tb[__HELLO_MAX];
  const char *format = "%s received a message: %s";
  const char *msgstr = "(hello)";
  char data[128];

  /* parsing ubus binary message format. */
  blobmsg_parse(hello_policy, ARRAY_SIZE(hello_policy), tb, blob_data(msg), blob_len(msg));

  /* create a ubus client request structure. */
  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  if (tb[HELLO_MSG])
    msgstr = blobmsg_data(tb[HELLO_MSG]);

  sprintf(data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  /* init a ubus binary message send buffer to be send back to client. */
  blob_buf_init(&b, 0);

  /* add the reply message into buffer. */
  blobmsg_add_string(&b, "message", data);

  /* send ubus reply message */
  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  return 0;
}

/* ubus object method table */
static const struct ubus_method test_methods[] = {
  UBUS_METHOD("hello", test_hello, hello_policy),
};

/* ubus object table */
static struct ubus_object_type test_object_type =
  UBUS_OBJECT_TYPE("test", test_methods);

/* ubus object registration. */
static struct ubus_object test_object = {
  .name = "test",
  .type = &test_object_type,
  .methods = test_methods,
  .n_methods = ARRAY_SIZE(test_methods),
};

static void server_main(void)
{
  int ret;

/* add a new object. */
  ret = ubus_add_object(ctx, &test_object);
  if (ret)
    printf("Failed to add object: %s\n", ubus_strerror(ret));

/* subscriber the test event. */
  ret = ubus_register_subscriber(ctx, &test_event);
  if (ret)
    printf("Failed to add watch handler: %s\n", ubus_strerror(ret));

/* run uloop and wait event indication. */
  uloop_run();
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

  /* init uloop */
  uloop_init();

  /* mask the SIGPIPE signal */
  signal(SIGPIPE, SIG_IGN);

  /* connect to ubusd. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubusd\n");
    return -1;
  }

  /* add to uloop polling context. */
  ubus_add_uloop(ctx);

  server_main();

  /* finish uloop */
  uloop_done();

  /* free client context structure */
  ubus_free(ctx);

  return 0;
}
