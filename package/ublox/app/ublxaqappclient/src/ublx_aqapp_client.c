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

static void test_client_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
  fprintf(stderr, "Subscribers active: %d\n", obj->has_subscribers);
}

static struct ubus_object test_client_object = {
  .subscribe_cb = test_client_subscribe_cb,
};

static void test_client_notify_cb(struct uloop_timeout *timeout)
{
  static int counter = 0;
  int err;
  struct timeval tv1, tv2;
  int max = 1000;
  long delta;
  int i = 0;

  blob_buf_init(&b, 0);
  blobmsg_add_u32(&b, "counter", counter++);

  gettimeofday(&tv1, NULL);
  for (i = 0; i < max; i++)
    err = ubus_notify(ctx, &test_client_object, "ping", b.head, 1000);
  gettimeofday(&tv2, NULL);
  if (err)
    fprintf(stderr, "Notify failed: %s\n", ubus_strerror(err));

  delta = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  fprintf(stderr, "Avg time per iteration: %ld usec\n", delta / max);

  uloop_timeout_set(timeout, 1000);
}

enum {
  RETURN_CODE,
  __RETURN_MAX,
};

static const struct blobmsg_policy return_policy[__RETURN_MAX] = {
  [RETURN_CODE] = { .name = "rc", .type = BLOBMSG_TYPE_INT32 },
};

static void test_count_data_cb(struct ubus_request *req,
            int type, struct blob_attr *msg)
{
  struct blob_attr *tb[__RETURN_MAX];
  int rc;
  uint32_t count_to = *(uint32_t *)req->priv;

  blobmsg_parse(return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[RETURN_CODE]) {
    fprintf(stderr, "No return code received from server\n");
    return;
  }
  rc = blobmsg_get_u32(tb[RETURN_CODE]);
  if (rc)
    fprintf(stderr, "Corruption of data with count up to '%u'\n", count_to);
  else
    fprintf(stderr, "Server validated our count up to '%u'\n", count_to);
}

static void test_count(struct uloop_timeout *timeout)
{
  uloop_timeout_set(timeout, 2000);
}

static struct uloop_timeout notify_timer = {
  .cb = test_client_notify_cb,
};

static struct uloop_timeout count_timer = {
  .cb = test_count,
};

static void test_client_fd_data_cb(struct ustream *s, int bytes)
{
  char *data, *sep;
  int len;

  data = ustream_get_read_buf(s, &len);
  if (len < 1)
    return;

  sep = strchr(data, '\n');
  if (!sep)
    return;

  *sep = 0;
  fprintf(stderr, "Got line: %s\n", data);
  ustream_consume(s, sep + 1 - data);
}

static void test_client_fd_cb(struct ubus_request *req, int fd)
{
  static struct ustream_fd test_fd;

  fprintf(stderr, "Got fd from the server, watching...\n");

  test_fd.stream.notify_read = test_client_fd_data_cb;
  ustream_fd_init(&test_fd, fd);
}

static void test_client_complete_cb(struct ubus_request *req, int ret)
{
  fprintf(stderr, "completed request, ret: %d\n", ret);
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
  ubus_invoke(ctx, id, "enable", b.head, NULL, 0, 3000);

  req.fd_cb = test_client_fd_cb;
  req.complete_cb = test_client_complete_cb;
  ubus_complete_request_async(ctx, &req);

  uloop_timeout_set(&count_timer, 2000);

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

  uloop_init();

  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    fprintf(stderr, "Failed to connect to ubus\n");
    return -1;
  }

  ubus_add_uloop(ctx);

  client_main();

  ubus_free(ctx);
  uloop_done();

  return 0;
}

