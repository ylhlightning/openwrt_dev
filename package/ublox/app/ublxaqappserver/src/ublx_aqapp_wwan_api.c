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

enum {
  WWAN_IF_ENABLE_ID,
  WWAN_IF_ENABLE_MSG,
  __WWAN_IF_ENABLE_MAX
};

static const struct blobmsg_policy wwan_if_enable_policy[] = {
  [WWAN_IF_ENABLE_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
  [WWAN_IF_ENABLE_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
};

static const struct ubus_method wwan_methods[] = {
  UBUS_METHOD("enable", wwan_if_enable, wwan_if_enable_policy),
};

static struct ubus_object_type wwan_object_type =
  UBUS_OBJECT_TYPE("wwan", wwan_methods);

struct ubus_object wwan_object = {
  .name = "wwan",
  .type = &wwan_object_type,
  .methods = wwan_methods,
  .n_methods = ARRAY_SIZE(wwan_methods),
};

struct wwan_if_enable_request {
  struct ubus_request_data req;
  struct uloop_timeout timeout;
  int fd;
  int idx;
  char data[];
};

static struct blob_buf b;

static int wwan_if_enable_do(char *recv_msg)
{
  char cmd_name_cfun_enable[] = "at+cfun=1";
  char cmd_name_cpin_query[] = "at+cpin=?";
  char cmd_name_cpin_insert[] = "at+cpin=9754";
  int cmd_cfun_result, cmd_cpin_result;
  char msg[CMD_MSG_MAX_LEN];
  int fd, ret;

  printf("Command to be sent to serial port: %s\n", cmd_name_cfun_enable);

  fd = open_modem(modem_port_name);
  if(fd < 0)
  {
    printf("Failed to open the modem\n");
    return FALSE;
  }

  ret = send_cmd_to_modem(fd, cmd_name_cfun_enable);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(fd, &cmd_cfun_result, msg);   
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }
  
  ret = send_cmd_to_modem(fd, cmd_name_cpin_query);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(fd, &cmd_cpin_result, msg);   
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_cpin_result == FALSE)
  {
    ret = send_cmd_to_modem(fd, cmd_name_cpin_insert);
    if(ret < 0)
    {
      printf("Failed to send command to modem\n");
      return FALSE;
    }
      
    ret = recv_data_from_modem(fd, &cmd_cpin_result, msg);   
    if(ret < 0)
    {
      printf("Failed to receive message from modem\n");
      return FALSE;
    }
  }
  
  close_modem(fd);

  if((cmd_cfun_result == TRUE) && (cmd_cpin_result == TRUE))
  {
    printf("WWAN interface has already enabled.\n");
    strncpy(recv_msg, msg_ok, strlen(msg_ok));
    return TRUE;
  }
  else
  {
    printf("WWAN interface initialization failed.\n");
    strncpy(recv_msg, msg_err, strlen(msg_err));
    return FALSE;
  }
  
}

static void wwan_if_enable_fd_reply(struct uloop_timeout *t)
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
  struct wwan_if_enable_request *req = container_of(t, struct wwan_if_enable_request, timeout);
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
  struct wwan_if_enable_request *hreq;
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
    printf("wwan_if_enable_do failed.\n");
  }
  
  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_enable_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}

void ublx_add_object_wwan(void)
{
  int ret = ubus_add_object(ctx, &wwan_object);
  if (ret){
    fprintf(stderr, "Failed to add object %s: %s\n", wwan_object.name, ubus_strerror(ret));
  }
}


