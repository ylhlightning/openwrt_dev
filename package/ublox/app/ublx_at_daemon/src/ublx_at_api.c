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
 * @file ublx_at_api.c
 *
 * @brief ublx at command api object.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     03/07/2015
 *
 ***********************************************************/

#include "ublx_at_api.h"

struct ublx_at_request {
  struct ubus_request_data req;
  struct uloop_timeout timeout;
  int fd;
  int idx;
  char data[CMD_MSG_LEN];
};

static struct blob_buf b;

/***************************************************************/
/*Ubus object policy configuration*/

enum {
  UBLX_SEND_CMD_STR,
  __UBLX_SEND_CMD_MAX,
};

static const struct blobmsg_policy ublx_at_send_cmd_policy[__UBLX_SEND_CMD_MAX] = {
  [UBLX_SEND_CMD_STR] = { .name = "cmd", .type = BLOBMSG_TYPE_STRING },
};

enum {
  UBLX_SEND_SMS_NUM,
  UBLX_SEND_SMS_MSG,
  __UBLX_SEND_SMS_MAX,
};

static const struct blobmsg_policy ublx_at_send_sms_policy[__UBLX_SEND_SMS_MAX] = {
  [UBLX_SEND_SMS_NUM] = { .name = "cmd", .type = BLOBMSG_TYPE_STRING },
  [UBLX_SEND_SMS_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
};


/***************************************************************/
/*Ubus object at_send_cmd method registration*/

static const struct ubus_method ublx_at_methods[] = {
  UBUS_METHOD("at_send_cmd",  ublx_at_send_cmd, ublx_at_send_cmd_policy),
  UBUS_METHOD("at_send_sms",  ublx_at_send_sms, ublx_at_send_sms_policy),
};

static struct ubus_object_type ublx_at_object_type =
  UBUS_OBJECT_TYPE("ublxat", ublx_at_methods);

struct ubus_object ublxat_object = {
  .name = "ublxat",
  .type = &ublx_at_object_type,
  .methods = ublx_at_methods,
  .n_methods = ARRAY_SIZE(ublx_at_methods),
};


/***************************************************************/
/*Ubus object ublxat method registration function called by server application*/

void ublx_add_object_at(void)
{
  int ret = ubus_add_object(ctx, &ublxat_object);
  if (ret){
    printf("Failed to add object %s: %s\n", ublxat_object.name, ubus_strerror(ret));
  }
}

/***************************************************************/
/*Ubus object method handler function*/

static int ublx_at_send_cmd_do(char *recv_msg, char *cmd)
{
  int cmd_send_cmd_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Send cmd:";
  int ret;

  printf("Command to be sent to serial port: %s\n", cmd);

  append_quotation_mark(cmd, num_append);

  ret = send_cmd_to_modem(modem_fd, cmd);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_send_cmd_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_send_cmd_result == TRUE)
  {
    printf("send at command:%s successful.\n", cmd);
    strncat(client_msg, msg, strlen(msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("send at command failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_send_cmd_fd_reply(struct uloop_timeout *t)
{
  struct ublx_at_request *req = container_of(t, struct ublx_at_request, timeout);
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

static void ublx_send_cmd_reply(struct uloop_timeout *t)
{
  struct ublx_at_request *req = container_of(t, struct ublx_at_request, timeout);
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

  req->timeout.cb = ublx_send_cmd_fd_reply;
  ublx_send_cmd_fd_reply(t);
}

static int ublx_at_send_cmd(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_at_request *hreq;
  struct blob_attr *tb[__UBLX_SEND_SMS_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  blobmsg_parse(ublx_at_send_cmd_policy, __UBLX_SEND_CMD_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_CMD_STR])
    return UBUS_STATUS_INVALID_ARGUMENT;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_at_send_cmd_do(msgstr, blobmsg_data(tb[UBLX_SEND_CMD_STR])) == FALSE)
  {
    printf("ublx at send cmd failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_send_cmd_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}



/***************************************************************/
/*Ubus object method handler function*/

/* send via sms */
static void append_quotation_mark(char *num, char *str_append)
{
  char *ptr_num = num;
  char *ptr_append = str_append;

  *ptr_append = '"';
  ptr_append ++;

  while(*ptr_num != '\0')
  {
    *ptr_append ++ = *ptr_num ++;
  }

  *ptr_append = '"';

  ptr_append ++;
  *ptr_append = '\0';

  return str_append;
}


static int ublx_at_send_sms_do(char *recv_msg, char *cmd, char *sms_msg)
{
  int cmd_send_sms_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Send message via sms:";
  int ret;
  char tmp_msg[5] = "test";

  printf("Command to be sent to serial port: %s with message: %s\n", cmd, tmp_msg);

  append_quotation_mark(cmd, num_append);

  ret = send_sms_to_modem_with_cmd(modem_fd, cmd, tmp_msg);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_send_sms_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_send_sms_result == TRUE)
  {
    printf("send at command:%s via sms successful.\n", cmd);
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("send at command via sms failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_send_sms_fd_reply(struct uloop_timeout *t)
{
  struct ublx_at_request *req = container_of(t, struct ublx_at_request, timeout);
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

static void ublx_send_sms_reply(struct uloop_timeout *t)
{
  struct ublx_at_request *req = container_of(t, struct ublx_at_request, timeout);
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

  req->timeout.cb = ublx_send_sms_fd_reply;
  ublx_send_sms_fd_reply(t);
}

static int ublx_at_send_sms(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_at_request *hreq;
  struct blob_attr *tb[__UBLX_SEND_SMS_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  blobmsg_parse(ublx_at_send_sms_policy, __UBLX_SEND_SMS_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_SMS_NUM] || !tb[UBLX_SEND_SMS_MSG])
    return UBUS_STATUS_INVALID_ARGUMENT;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_at_send_sms_do(msgstr, blobmsg_data(tb[UBLX_SEND_SMS_NUM]), tb[UBLX_SEND_SMS_MSG]) == FALSE)
  {
    printf("ublx at send sms failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_send_sms_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}














