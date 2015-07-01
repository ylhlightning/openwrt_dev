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

static const struct blobmsg_policy hello_policy[] = {
  [HELLO_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
  [HELLO_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};

struct hello_request {
  struct ubus_request_data req;
  struct uloop_timeout timeout;
  int fd;
  int idx;
  char data[];
};

static void test_hello_fd_reply(struct uloop_timeout *t)
{
  struct hello_request *req = container_of(t, struct hello_request, timeout);
  char *data;

  data = alloca(strlen(req->data) + 32);
  sprintf(data, "msg%d: %s\n", ++req->idx, req->data);
  if (write(req->fd, data, strlen(data)) < 0) {
    close(req->fd);
    free(req);
    return;
  }

  uloop_timeout_set(&req->timeout, 1000);
}

static void test_hello_reply(struct uloop_timeout *t)
{
  struct hello_request *req = container_of(t, struct hello_request, timeout);
  int fds[2];

  blob_buf_init(&b, 0);
  blobmsg_add_string(&b, "message", req->data);
  ubus_send_reply(ctx, &req->req, b.head);

  if (pipe(fds) == -1) {
    fprintf(stderr, "Failed to create pipe\n");
    return;
  }
  ubus_request_set_fd(ctx, &req->req, fds[0]);
  ubus_complete_deferred_request(ctx, &req->req, 0);
  req->fd = fds[1];

  req->timeout.cb = test_hello_fd_reply;
  test_hello_fd_reply(t);
}

static int test_hello(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct hello_request *hreq;
  struct blob_attr *tb[__HELLO_MAX];
  const char *format = "%s received a message: %s";
  const char *msgstr = "(hello)";

  blobmsg_parse(hello_policy, ARRAY_SIZE(hello_policy), tb, blob_data(msg), blob_len(msg));

  if (tb[HELLO_MSG])
    msgstr = blobmsg_data(tb[HELLO_MSG]);

  hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1);
  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = test_hello_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

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
    fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));

/* subscriber the test event. */
  ret = ubus_register_subscriber(ctx, &test_event);
  if (ret)
    fprintf(stderr, "Failed to add watch handler: %s\n", ubus_strerror(ret));

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
  signal(SIGPIPE, SIG_IGN);

  /* connect to ubus. */
  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    fprintf(stderr, "Failed to connect to ubus\n");
    return -1;
  }

  /* add to uloop polling context. */
  ubus_add_uloop(ctx);

  server_main();

  ubus_free(ctx);
  uloop_done();

  return 0;
}
