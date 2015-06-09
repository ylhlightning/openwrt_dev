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
 * @file ublx_aqapp_wwan_api.c
 *
 * @brief ublx aqapp api for wwan object.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#include "ublx_aqapp_wwan_api.h"

struct wwan_if_request {
  struct ubus_request_data req;
  struct uloop_timeout timeout;
  int fd;
  int idx;
  char data[];
};

static struct blob_buf b;

char cmd_name_cfun_enable[CMD_LEN]  = "at+cfun=1";
char cmd_name_cfun_disable[CMD_LEN] = "at+cfun=0";
char cmd_name_cpin_query[CMD_LEN]   = "at+cpin=?";
char cmd_name_cpin_insert[CMD_LEN]  = "at+cpin=9754";


/***************************************************************/
/*Ubus object policy configuration*/

/* wwan enable */
enum {
  WWAN_IF_ENABLE_ID,
  WWAN_IF_ENABLE_MSG,
  __WWAN_IF_ENABLE_MAX
};

static const struct blobmsg_policy wwan_if_enable_policy[] = {
  [WWAN_IF_ENABLE_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
  [WWAN_IF_ENABLE_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};



/* wwan disable */
enum {
  WWAN_IF_DISABLE_ID,
  WWAN_IF_DISABLE_MSG,
  __WWAN_IF_DISABLE_MAX
};

static const struct blobmsg_policy wwan_if_disable_policy[] = {
  [WWAN_IF_DISABLE_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
  [WWAN_IF_DISABLE_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};


/***************************************************************/
/*Ubus object wwan method registration*/

static const struct ubus_method wwan_methods[] = {
  UBUS_METHOD("enable", wwan_if_enable, wwan_if_enable_policy),
  UBUS_METHOD("disable", wwan_if_disable, wwan_if_disable_policy),

};

static struct ubus_object_type wwan_object_type =
  UBUS_OBJECT_TYPE("wwan", wwan_methods);

struct ubus_object wwan_object = {
  .name = "wwan",
  .type = &wwan_object_type,
  .methods = wwan_methods,
  .n_methods = ARRAY_SIZE(wwan_methods),
};


/***************************************************************/
/*Ubus object wwan method registration function called by server application*/

void ublx_add_object_wwan(void)
{
  int ret = ubus_add_object(ctx, &wwan_object);
  if (ret){
    fprintf(stderr, "Failed to add object %s: %s\n", wwan_object.name, ubus_strerror(ret));
  }
}


/***************************************************************/
/*Ubus object method handler function*/

/* wwan enable */

static int wwan_if_enable_do(char *recv_msg)
{
  int cmd_cfun_result, cmd_cpin_result;
  char msg[CMD_MSG_MAX_LEN];
  int ret;

  printf("Command to be sent to serial port: %s\n", cmd_name_cfun_enable);

  ret = send_cmd_to_modem(modem_fd, cmd_name_cfun_enable);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_cfun_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  ret = send_cmd_to_modem(modem_fd, cmd_name_cpin_query);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_cpin_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_cpin_result == FALSE)
  {
    ret = send_cmd_to_modem(modem_fd, cmd_name_cpin_insert);
    if(ret < 0)
    {
      printf("Failed to send command to modem\n");
      return FALSE;
    }

    ret = recv_data_from_modem(modem_fd, &cmd_cpin_result, msg);
    if(ret < 0)
    {
      printf("Failed to receive message from modem\n");
      return FALSE;
    }
  }

  if((cmd_cfun_result == TRUE) && (cmd_cpin_result == TRUE))
  {
    printf("WWAN interface has already enabled.\n");
    strncpy(recv_msg, MSG_OK, strlen(MSG_OK));
    return TRUE;
  }
  else
  {
    printf("WWAN interface initialization failed.\n");
    strncpy(recv_msg, MSG_ERROR, strlen(MSG_ERROR));
    return FALSE;
  }
}

static void wwan_if_enable_fd_reply(struct uloop_timeout *t)
{
  struct wwan_if_request *req = container_of(t, struct wwan_if_request, timeout);
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
  struct wwan_if_request *req = container_of(t, struct wwan_if_request, timeout);
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

  req->timeout.cb = wwan_if_enable_fd_reply;
  wwan_if_enable_fd_reply(t);
}

static int wwan_if_enable(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  struct blob_attr *tb[__WWAN_IF_ENABLE_MAX];
  const char *format = "%s received a message: %s";
  char data[1024];
  char *msgstr = data;

  blobmsg_parse(wwan_if_enable_policy, ARRAY_SIZE(wwan_if_enable_policy), tb, blob_data(msg), blob_len(msg));

  if (tb[WWAN_IF_ENABLE_MSG])
    msgstr = blobmsg_data(tb[WWAN_IF_ENABLE_MSG]);

  hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1);

  if(wwan_if_enable_do(msgstr) == FALSE)
  {
    printf("wwan_if_enable failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_enable_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* wwan disable */

static int wwan_if_disable_do(char *recv_msg)
{
  int cmd_cfun_result;
  char msg[CMD_MSG_MAX_LEN];
  int ret;

  printf("Command to be sent to serial port: %s\n", cmd_name_cfun_disable);

  ret = send_cmd_to_modem(modem_fd, cmd_name_cfun_disable);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_cfun_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_cfun_result == TRUE)
  {
    printf("WWAN interface has already disabled.\n");
    strncpy(recv_msg, MSG_OK, strlen(MSG_OK));
    return TRUE;
  }
  else
  {
    printf("WWAN interface disable failed.\n");
    strncpy(recv_msg, MSG_ERROR, strlen(MSG_ERROR));
    return FALSE;
  }
}

static void wwan_if_disable_fd_reply(struct uloop_timeout *t)
{
  struct wwan_if_request *req = container_of(t, struct wwan_if_request, timeout);
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

static void wwan_if_disable_reply(struct uloop_timeout *t)
{
  struct wwan_if_request *req = container_of(t, struct wwan_if_request, timeout);
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

  req->timeout.cb = wwan_if_disable_fd_reply;
  wwan_if_disable_fd_reply(t);
}

static int wwan_if_disable(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  struct blob_attr *tb[__WWAN_IF_DISABLE_MAX];
  const char *format = "%s received a message: %s";
  char data[1024];
  char *msgstr = data;

  blobmsg_parse(wwan_if_disable_policy, ARRAY_SIZE(wwan_if_disable_policy), tb, blob_data(msg), blob_len(msg));

  if (tb[WWAN_IF_DISABLE_MSG])
    msgstr = blobmsg_data(tb[WWAN_IF_DISABLE_MSG]);

  hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1);

  if(wwan_if_disable_do(msgstr) == FALSE)
  {
    printf("wwan_if_disable failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_disable_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}

/***************************************************************/





