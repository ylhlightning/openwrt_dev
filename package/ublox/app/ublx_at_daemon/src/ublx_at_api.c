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
  UBLX_SEND_SMS_CMD,
  UBLX_SEND_SMS_MSG,
  __UBLX_SEND_SMS_MAX,
};

static const struct blobmsg_policy ublx_at_send_sms_policy[__UBLX_SEND_SMS_MAX] = {
  [UBLX_SEND_SMS_CMD] = { .name = "cmd", .type = BLOBMSG_TYPE_STRING },
  [UBLX_SEND_SMS_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
};


/***************************************************************/
/*Ubus object at_send_cmd method registration*/

static const struct ubus_method ublx_at_methods[] = {
  UBUS_METHOD("at_send_cmd",  ublx_at_send_cmd, ublx_at_send_cmd_policy),
  UBUS_METHOD("at_send_sms",  ublx_at_send_sms, ublx_at_send_sms_policy),
};

static struct ubus_object_type ublx_at_object_type =
  UBUS_OBJECT_TYPE(UBLX_AT_PATH, ublx_at_methods);

struct ubus_object ublxat_object = {
  .name = UBLX_AT_PATH,
  .type = &ublx_at_object_type,
  .methods = ublx_at_methods,
  .n_methods = ARRAY_SIZE(ublx_at_methods),
  .subscribe_cb = ublxat_uBussubscribeCb,
};


/***************************************************************/
/*Ubus object ublxat method registration function called by server application*/

int ublx_add_object_at(void)
{
  uint32_t id;
  int ret = 0;

  ret = ubus_add_object(ctx, &ublxat_object);
  if (ret){
    printf("Failed to add object %s: %s\n", ublxat_object.name, ubus_strerror(ret));
  }
  else
  {
    printf("Register object %s in ubusd.\n", ublxat_object.name);
    return ret;
  }

  return 0;
}

static void ublxat_uBussubscribeCb(struct ubus_context *ctx, struct ubus_object *obj)
{
  printf("ublxat subscribers ", obj->has_subscribers?"success!\n":"fail!\n");
}


/***************************************************************/
/*Ubus object method handler function*/

static void timestamp()
{
    struct timeval detail_time;
    struct tm *Tm;
    time_t ltime;

    ltime=time(NULL);

    Tm=localtime(&ltime);

    gettimeofday(&detail_time,NULL);

    printf("\nTimestamp: [%d:%d:%d.%d%d]\n\n",
            Tm->tm_hour,
            Tm->tm_min,
            Tm->tm_sec,
            detail_time.tv_usec /1000,  /* milliseconds */
            detail_time.tv_usec); /* microseconds */
}


static int ublx_at_send_cmd_do(char *recv_msg, char *cmd)
{
  int cmd_send_cmd_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Send cmd:";
  int ret;

  printf("\n\n\n***********Command to be sent to serial port: %s****************\n", cmd);

  timestamp();

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
//    printf("\n\n\nsend at command:%s successful.\n", cmd);
    strncat(client_msg, msg, strlen(msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("\n\n\nsend at command failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static int ublx_at_send_cmd(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct blob_attr *tb[__UBLX_SEND_SMS_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  blobmsg_parse(ublx_at_send_cmd_policy, __UBLX_SEND_CMD_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_CMD_STR])
  {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  if(ublx_at_send_cmd_do(msgstr, blobmsg_data(tb[UBLX_SEND_CMD_STR])) == FALSE)
  {
    printf("ublx at send cmd failed.\n");
  }

  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

  printf("\n\n*************************************************************************\n\n\n\n");

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
}

static int ublx_at_send_sms_do(char *recv_msg, char *cmd, char *sms_msg)
{
  int cmd_send_sms_result;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN];
  char *format = "Send message via sms: %s";
  int ret;

  printf("\n\n\n***Command to be sent to serial port: %s with message: %s*****\n", cmd, sms_msg);

  timestamp();

  append_quotation_mark(cmd, num_append);

  ret = send_sms_to_modem_with_cmd(modem_fd, cmd, sms_msg);
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

  snprintf(client_msg, strlen(format)+strlen(cmd)+1, format, cmd);

  if(cmd_send_sms_result == TRUE)
  {
//    printf("\n\n\n send at command:%s via sms successful.\n", cmd);
    strncat(client_msg, msg, strlen(msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("\n\n\n send at command via sms failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static int ublx_at_send_sms(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct blob_attr *tb[__UBLX_SEND_SMS_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  blobmsg_parse(ublx_at_send_sms_policy, __UBLX_SEND_SMS_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_SMS_CMD] || !tb[UBLX_SEND_SMS_MSG])
  {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  if(ublx_at_send_sms_do(msgstr, blobmsg_data(tb[UBLX_SEND_SMS_CMD]), blobmsg_data(tb[UBLX_SEND_SMS_MSG])) == FALSE)
  {
    printf("ublx at send sms failed.\n");
  }

  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

  printf("\n\n*************************************************************************\n\n\n\n");

  return 0;
}














