/*
 * Copyright (C) 2011-2014 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <unistd.h>
#include <signal.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"
#include "count.h"

static struct ubus_context *ctx;
static struct ubus_subscriber test_event;
static struct blob_buf b;
static char *modem_port_name = "/dev/ttyUSB3";

#define CMD_MSG_LEN 1024

int wwan_if_enable_do(char *recv_msg)
{

  char *cmd_name_cfun_enable = "at+cfun=1";
  char *cmd_name_cpin_insert = "at+cpin=9754";
  int fd, ret;

  printf("Command to be sent to serial port: %s\n", cmd_name_cfun_enable );

  fd = open_modem(modem_port_name);
  if(fd < 0)
  {
    exit(1);
  }

  ret = send_cmd_to_modem(fd, cmd_name_cfun_enable );
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     exit(1);
  }
  
  ret = send_cmd_to_modem(fd, cmd_name_cfun_enable );
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     exit(1);
  }

  ret = recv_data_from_modem(fd, recv_msg);
   
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     exit(1);
  }
  
  close_modem(fd);

  exit(0);
}




enum {
	WWAN_IF_ENABLE_ID,
	WWAN_IF_ENABLE_ID_MSG,
	__HELLO_MAX
};

static const struct blobmsg_policy wwan_if_enable_policy[] = {
	[WWAN_IF_ENABLE_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[WWAN_IF_ENABLE_ID_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};

struct wwan_if_enable_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	int fd;
	int idx;
	char data[];
};

static void wwan_if_enable_reply(struct uloop_timeout *t)
{
	struct wwan_if_enable_request *req = container_of(t, struct wwan_if_enable_request, timeout);
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

static void wwan_if_enable_reply(struct uloop_timeout *t)
{
	struct wwan_if_enable_request *req = container_of(t, struct hello_request, timeout);
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

	req->timeout.cb = wwan_if_enable_reply;
	wwan_if_enable_reply(t);
}

static int test_hello(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct hello_request *hreq;
	struct blob_attr *tb[__HELLO_MAX];
	const char *format = "%s received a message: %s";
	const char *msgstr = "(unknown)";

	blobmsg_parse(hello_policy, ARRAY_SIZE(hello_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[HELLO_MSG])
		msgstr = blobmsg_data(tb[HELLO_MSG]);

	hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1);
	sprintf(hreq->data, format, obj->name, msgstr);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = wwan_if_enable_reply;
	uloop_timeout_set(&hreq->timeout, 1000);

	return 0;
}

enum {
	WATCH_ID,
	WATCH_COUNTER,
	__WATCH_MAX
};

static const struct blobmsg_policy watch_policy[__WATCH_MAX] = {
	[WATCH_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[WATCH_COUNTER] = { .name = "counter", .type = BLOBMSG_TYPE_INT32 },
};

static void
test_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s,
                   uint32_t id)
{
	fprintf(stderr, "Object %08x went away\n", id);
}

static int
test_notify(struct ubus_context *ctx, struct ubus_object *obj,
	    struct ubus_request_data *req, const char *method,
	    struct blob_attr *msg)
{
#if 0
	char *str;

	str = blobmsg_format_json(msg, true);
	fprintf(stderr, "Received notification '%s': %s\n", method, str);
	free(str);
#endif

	return 0;
}

static int test_watch(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__WATCH_MAX];
	int ret;

	blobmsg_parse(watch_policy, __WATCH_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[WATCH_ID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	test_event.remove_cb = test_handle_remove;
	test_event.cb = test_notify;
	ret = ubus_subscribe(ctx, &test_event, blobmsg_get_u32(tb[WATCH_ID]));
	fprintf(stderr, "Watching object %08x: %s\n", blobmsg_get_u32(tb[WATCH_ID]), ubus_strerror(ret));
	return ret;
}

enum {
    COUNT_TO,
    COUNT_STRING,
    __COUNT_MAX
};

static const struct blobmsg_policy count_policy[__COUNT_MAX] = {
    [COUNT_TO] = { .name = "to", .type = BLOBMSG_TYPE_INT32 },
    [COUNT_STRING] = { .name = "string", .type = BLOBMSG_TYPE_STRING },
};

static int test_count(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__COUNT_MAX];
	char *s1, *s2;
	uint32_t num;

	blobmsg_parse(count_policy, __COUNT_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[COUNT_TO] || !tb[COUNT_STRING])
		return UBUS_STATUS_INVALID_ARGUMENT;

	num = blobmsg_get_u32(tb[COUNT_TO]);
	s1 = blobmsg_get_string(tb[COUNT_STRING]);
	s2 = count_to_number(num);
	if (!s1 || !s2) {
		free(s2);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "rc", strcmp(s1, s2));
	ubus_send_reply(ctx, req, b.head);
	free(s2);

	return 0;
}

static const struct ubus_method wwan_methods[] = {
	UBUS_METHOD("enable", wwan_if_enable, wwan_if_enable_policy),
	UBUS_METHOD("disable", wwan_if_disable, wwan_if_disable_policy),
	UBUS_METHOD("connect", wwan_if_connect, wwan_if_connect_policy),
	UBUS_METHOD("disconnect", wwan_if_disconnect, wwan_if_disconnect_policy),
};

static struct ubus_object_type wwan_object_type =
	UBUS_OBJECT_TYPE("wwan", wwan_methods);

static struct ubus_object wwan_object = {
	.name = "wwan",
	.type = &wwan_object_type,
	.methods = wwan_methods,
	.n_methods = ARRAY_SIZE(wwan_methods),
};

static void server_main(void)
{
	int ret;

	ret = ubus_add_object(ctx, &wwan_object);
	if (ret)
		fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));

	ret = ubus_register_subscriber(ctx, &test_event);
	if (ret)
		fprintf(stderr, "Failed to add watch handler: %s\n", ubus_strerror(ret));

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
	signal(SIGPIPE, SIG_IGN);

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	server_main();

	ubus_free(ctx);
	uloop_done();

	return 0;
}

