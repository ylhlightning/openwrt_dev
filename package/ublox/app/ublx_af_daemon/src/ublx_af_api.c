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
 * @file ublx_af_api.c
 *
 * @brief ublx af command api object.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     03/07/2015
 *
 ***********************************************************/

#include "ublx_af_api.h"

struct ublx_af_request {
  struct ubus_request_data req;
  struct uloop_timeout timeout;
  int fd;
  int idx;
  char data[CMD_MSG_LEN];
};

static struct blob_buf b;

char cmd_name_cfun_enable[CMD_LEN]      = "at+cfun=1";
char cmd_name_cfun_disable[CMD_LEN]     = "at+cfun=0";
char cmd_name_cpin_query[CMD_LEN]       = "at+cpin=?";
char cmd_name_cpin_insert[CMD_LEN]      = "at+cpin=";
char cmd_name_cgdcont_query[CMD_LEN]   = "at+cgdcont?";
char cmd_name_context_active[CMD_LEN]   = "at!scact=1,1";
char cmd_name_context_deactive[CMD_LEN] = "at!scact=0,1";
char cmd_name_get_public_addr[CMD_LEN]  = "at!scpaddr=1";
char cmd_name_set_sms[CMD_LEN]          = "at+cmgf=1";
char cmd_name_send_sms[CMD_LEN]          = "at+cmgs=";



/***************************************************************/
/*Ubus object policy configuration*/
enum {
  UBLX_UNLOCK_SIM,
  __UBLX_UNLOCK_SIM_MAX,
};

static const struct blobmsg_policy ublx_af_unlock_sim_policy[__UBLX_UNLOCK_SIM_MAX] = {
  [UBLX_UNLOCK_SIM] = { .name = "pin", .type = BLOBMSG_TYPE_STRING },
};

enum {
  UBLX_SEND_SMS_NUM,
  UBLX_SEND_SMS_MSG,
  __UBLX_SEND_SMS_MAX,
};

static const struct blobmsg_policy ublx_af_send_sms_policy[__UBLX_SEND_SMS_MAX] = {
  [UBLX_SEND_SMS_NUM] = { .name = "number", .type = BLOBMSG_TYPE_STRING },
  [UBLX_SEND_SMS_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
};



/***************************************************************/
/*Ubus object at_send_cmd method registration*/

static const struct ubus_method ublx_af_methods[] = {
  UBUS_METHOD("unlock_sim",  ublx_unlock_sim, ublx_af_unlock_sim_policy),
  {.name = "net_list",      .handler = ublx_af_net_list},
  {.name = "net_home",      .handler = ublx_af_net_home},
  UBUS_METHOD("send_sms",    ublx_af_send_sms, ublx_af_send_sms_policy),
};

static struct ubus_object_type ublx_af_object_type =
  UBUS_OBJECT_TYPE("ublxaf", ublx_af_methods);

struct ubus_object ublxaf_object = {
  .name = "ublxaf",
  .type = &ublx_af_object_type,
  .methods = ublx_af_methods,
  .n_methods = ARRAY_SIZE(ublx_af_methods),
};


/***************************************************************/
/*Ubus object ublxaf method registration function called by server application*/

void ublx_add_object_af(void)
{
  int ret = ubus_add_object(ctx, &ublxaf_object);
  if (ret){
    printf("Failed to add object %s: %s\n", ublxaf_object.name, ubus_strerror(ret));
  }
}


/***************************************************************/
/*Ubus object method handler function*/

/* unlock sim card */

static int ublx_af_unlock_sim_do(char *recv_msg, char *pin)
{
  int cmd_send_sms_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Unlock sim card:";
  int ret;

  printf("Start to unlock sim card.\n");

  printf("1. Sending command : %s\n", cmd_name_cfun_enable);

  printf("2. Sending command : %s\n", strncat(cmd_name_cpin_insert, pin, strlen(pin)));

  printf("3. Sending command : %s\n", cmd_name_cpin_query);

  cmd_send_sms_result = TRUE;

  if(cmd_send_sms_result == TRUE)
  {
    printf("Unlock pin successful.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("Unlock pin failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_unlock_sim_fd_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

static void ublx_unlock_sim_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

  req->timeout.cb = ublx_unlock_sim_fd_reply;
  ublx_unlock_sim_fd_reply(t);
}

static int ublx_unlock_sim(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_af_request *hreq;
  struct blob_attr *tb[__UBLX_UNLOCK_SIM_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  blobmsg_parse(ublx_af_unlock_sim_policy, __UBLX_UNLOCK_SIM_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_UNLOCK_SIM])
    return UBUS_STATUS_INVALID_ARGUMENT;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_af_unlock_sim_do(msgstr, blobmsg_data(tb[UBLX_UNLOCK_SIM])) == FALSE)
  {
    printf("ublx unblock sim failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_unlock_sim_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* list network */
static int ublx_af_net_list_do(char *recv_msg)
{
  int ublx_af_net_list_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "List cellular network:";
  int ret;

  printf("Start to list cellular network.\n");

  printf("1. Sending command : %s\n", cmd_name_cgdcont_query);

  ublx_af_net_list_result = TRUE;

  if(ublx_af_net_list_result == TRUE)
  {
    printf("List cellular network successful.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("List cellular network failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_af_net_list_fd_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

static void ublx_af_net_list_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

  req->timeout.cb = ublx_af_net_list_fd_reply;
  ublx_af_net_list_fd_reply(t);
}

static int ublx_af_net_list(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_af_request *hreq;
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_af_net_list_do(msgstr) == FALSE)
  {
    printf("ublx_af_net_list failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_af_net_list_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* get home network address */
static int ublx_af_net_home_do(char *recv_msg)
{
  int ublx_af_net_home_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "List home network ip address:";
  int ret;

  printf("Start to get home network address.\n");

  printf("1. Sending command : %s\n", cmd_name_get_public_addr);

  ublx_af_net_home_result = TRUE;

  if(ublx_af_net_home_result == TRUE)
  {
    printf("List home network ip address successful.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("List home network ip address failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_af_net_home_fd_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

static void ublx_af_net_home_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

  req->timeout.cb = ublx_af_net_home_fd_reply;
  ublx_af_net_home_fd_reply(t);
}

static int ublx_af_net_home(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_af_request *hreq;
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_af_net_home_do(msgstr) == FALSE)
  {
    printf("ublx_af_net_list failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_af_net_home_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* send sms */

static int ublx_af_send_sms_do(char *recv_msg, char *num, char *sms_msg)
{
  int ublx_af_send_sms_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Unlock sim card:";
  int ret;

  printf("Start to unlock sim card.\n");

  printf("1. Sending command : %s\n", cmd_name_set_sms);

  printf("2. Sending command : %s\n", strncat(cmd_name_send_sms, num, strlen(num)));

  printf("3. Sending message : %s\n", sms_msg);

  ublx_af_send_sms_result = TRUE;

  if(ublx_af_send_sms_result == TRUE)
  {
    printf("Unlock pin successful.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("Unlock pin failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void ublx_send_sms_fd_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

static void ublx_af_send_sms_reply(struct uloop_timeout *t)
{
  struct ublx_af_request *req = container_of(t, struct ublx_af_request, timeout);
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

static int ublx_af_send_sms(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct ublx_af_request *hreq;
  struct blob_attr *tb[__UBLX_UNLOCK_SIM_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  blobmsg_parse(ublx_af_send_sms_policy, __UBLX_SEND_SMS_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_SMS_NUM] || !tb[UBLX_SEND_SMS_MSG])
    return UBUS_STATUS_INVALID_ARGUMENT;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(ublx_af_send_sms_do(msgstr, blobmsg_data(tb[UBLX_SEND_SMS_NUM]), blobmsg_data(tb[UBLX_SEND_SMS_MSG])) == FALSE)
  {
    printf("ublx send sms failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = ublx_af_send_sms_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}








