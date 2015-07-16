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

static struct blob_buf b;
static struct blob_buf b_local;

static char client_reply_msg[1024];
static char client_cimi_num[CIMI_NUM_LEN];

static char cmd_name_cfun_enable[CMD_LEN]         = "at+cfun=1";
static char cmd_name_cfun_disable[CMD_LEN]        = "at+cfun=0";
static char cmd_name_cpin_query[CMD_LEN]          = "at+cpin?";
static char cmd_name_cgdcont_query[CMD_LEN]       = "at+cgdcont?";
static char cmd_name_context_active[CMD_LEN]      = "at!scact=1,1";
static char cmd_name_context_deactive[CMD_LEN]    = "at!scact=0,1";
static char cmd_name_get_public_addr[CMD_LEN]     = "at!scpaddr=1";
static char cmd_name_set_sms[CMD_LEN]             = "at+cmgf=1";
static char cmd_name_get_cimi[CMD_LEN]            = "at+cimi";
static char cmd_name_list_operator[CMD_LEN]       = "at+cops=?";
static char cmd_name_registered_operator[CMD_LEN] = "at+cops?";
static char cmd_name_check_registered[CMD_LEN]    = "at+creg?";


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
  __UBLX_SEND_SMS_MAX,
};

static const struct blobmsg_policy ublx_af_send_sms_policy[__UBLX_SEND_SMS_MAX] = {
  [UBLX_SEND_SMS_NUM] = { .name = "number", .type = BLOBMSG_TYPE_STRING },
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

char *get_cimi_num(char *str)
{
  int len = strlen(str);
  int i,j= 0;
  int ret_str_len = CIMI_NUM_LEN;
  char *ret_str = (char *)malloc(ret_str_len*sizeof(char)); 
//  printf("start to process string:%s\n", str);
  
  for(i = 0; i < len; i++)
  {
    if(*str >= 48 && *str <= 57)
    {
      ret_str[j] = *str;
      j++;
    }
    str ++;
  }
  ret_str[j] = '\0';
  return ret_str;
}


/***************************************************************/
/*Ubus object ublxaf method registration function called by server application*/


void ublx_add_object_af(void)
{
  int ret = ubus_add_object(ctx, &ublxaf_object);
  if (ret){
    printf("Failed to add object %s: %s\n", ublxaf_object.name, ubus_strerror(ret));
  }
  else
  {
    printf("Register object %s in ubusd.\n", ublxaf_object.name);
  }
}


static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, 0);

  memset(client_reply_msg, '\0', strlen(client_reply_msg));
  strncpy(client_reply_msg, str, strlen(str));

  printf("%s\n", client_reply_msg);

  free(str);
}


static int client_ubus_process(char *ubus_object, char *ubus_method, char *argv)
{
  static struct ubus_request req;
  uint32_t id;
  int ret, ret_ubus_invoke;
  const char *ubus_socket = NULL;
  struct ubus_context *ctx_local;

  ctx_local = ubus_connect(ubus_socket);
  if (!ctx_local) {
    printf("Failed to connect to ubus\n");
    return FALSE;
  }

  if (ubus_lookup_id(ctx_local, ubus_object, &id)) {
    printf("Failed to look up test object\n");
    return FALSE;
  }

  blob_buf_init(&b_local, 0);

  if(argv != NULL)
  {
    blobmsg_add_string(&b_local, "cmd", argv);
  }

  ret_ubus_invoke = ubus_invoke(ctx_local, id, ubus_method, b_local.head, receive_call_result_data, 0, UBLX_AF_INVOKE_TIMEOUT);

  if(ret_ubus_invoke == 0 || ret_ubus_invoke == 7)
  {
    ret = TRUE;
  }
  else
  {
    ret = FALSE;
  }

  ubus_free(ctx_local);
  return ret;
}

static int client_ubus_process_with_message(char *ubus_object, char *ubus_method, char *argv, char *sms_msg)
{
  static struct ubus_request req;
  uint32_t id;
  int ret, ret_ubus_invoke;
  const char *ubus_socket = NULL;
  struct ubus_context *ctx_local;

  if(!sms_msg)
  {
    printf("sms message could not be NULL.\n");
    return FALSE;
  }

  ctx_local = ubus_connect(ubus_socket);
  if (!ctx_local) {
    printf("Failed to connect to ubus\n");
    return FALSE;
  }

  if (ubus_lookup_id(ctx_local, ubus_object, &id)) {
    printf("Failed to look up test object\n");
    return FALSE;
  }

  blob_buf_init(&b_local, 0);

  if(argv != NULL)
  {
    blobmsg_add_string(&b_local, "cmd", argv);
  }

  if(sms_msg != NULL)
  {
    blobmsg_add_string(&b_local, "message", sms_msg);
  }

  ret_ubus_invoke = ubus_invoke(ctx_local, id, ubus_method, b_local.head, receive_call_result_data, 0, UBLX_AF_INVOKE_TIMEOUT);

  if(ret_ubus_invoke == 0 || ret_ubus_invoke == 7)
  {
    ret = TRUE;
  }
  else
  {
    ret = FALSE;
  }

  ubus_free(ctx_local);
  return ret;
}



/***************************************************************/
/*Ubus object method handler function*/

/* unlock sim card */

static int ublx_af_unlock_sim_do(char *recv_msg, char *pin)
{
  int cmd_unlock_sim_result = FALSE;
  int cmd_radio_active = TRUE;
  int cmd_get_cimi_result = TRUE;
  int step = 1;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Unlock sim card:";
  int ret;
  char *ubus_object = "ublxat";
  char *ubus_method = "at_send_cmd";
  char cmd_name_cpin_insert[CMD_LEN] = "at+cpin=";

  if(!recv_msg || !pin)
  {
    printf("Receive a Message NULL.\n");
    return FALSE;
  }

  printf("\n\n\n**********************Start to unlock sim card.*********************\n");

  printf("%d. Sending command : %s\n", step, cmd_name_cfun_enable);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_cfun_enable) == TRUE)
  {
    cmd_radio_active = TRUE;
  }
  else
  {
    cmd_radio_active = FALSE;
  }

  step++;

  printf("%d. Sending command : %s\n", step, cmd_name_cpin_query);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_cpin_query) == TRUE)
  {
      cmd_unlock_sim_result = TRUE;
  }

  step ++;

  if(!strstr(client_reply_msg, "READY"))
  {
    printf("%d. Sending command : %s\n", step, strncat(cmd_name_cpin_insert, pin, strlen(pin)));
    client_ubus_process(ubus_object, ubus_method, cmd_name_cpin_insert);
    step ++;
  }

  printf("%d. Sending command : %s\n", step, cmd_name_get_cimi);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_get_cimi) == TRUE)
  {
    cmd_get_cimi_result = TRUE;
  }
  else
  {
    cmd_get_cimi_result = FALSE;
  }

  char *cimi_num = get_cimi_num(client_reply_msg);

  memset(client_cimi_num, '\0', CIMI_NUM_LEN);

  if(cimi_num){
    strncpy(client_cimi_num, cimi_num, CIMI_NUM_LEN);
  }
  else
  {
    printf("Error cimi message format.\n");
    free(cimi_num);
    cimi_num = NULL;
  }

  if(cmd_radio_active == TRUE && cmd_unlock_sim_result == TRUE && cmd_get_cimi_result == TRUE)
  {
    printf("unlock sim card successful.\n");
    strncat(client_msg, client_reply_msg, strlen(client_reply_msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    free(cimi_num);
    cimi_num = NULL;
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

static int ublx_unlock_sim(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct blob_attr *tb[__UBLX_UNLOCK_SIM_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  blobmsg_parse(ublx_af_unlock_sim_policy, __UBLX_UNLOCK_SIM_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_UNLOCK_SIM])
  {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  if(ublx_af_unlock_sim_do(msgstr, blobmsg_data(tb[UBLX_UNLOCK_SIM])) == FALSE)
  {
    printf("ublx unblock sim failed.\n");
  }

//  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", client_reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

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
  char *ubus_object = "ublxat";
  char *ubus_method = "at_send_cmd";

  if(!recv_msg)
  {
    printf("Receive a Message NULL.\n");
    return FALSE;
  }

  printf("\n\n\n**********************Start to list cellular network.*********************\n");

  printf("1. Sending command : %s\n", cmd_name_list_operator);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_list_operator) == TRUE)
  {
    ublx_af_net_list_result = TRUE;
  }

  if(ublx_af_net_list_result == TRUE)
  {
    printf("List cellular network successful.\n");
    strncat(client_msg, client_reply_msg, strlen(client_reply_msg));
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

static int ublx_af_net_list(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  if(ublx_af_net_list_do(msgstr) == FALSE)
  {
    printf("ublx_af_net_list failed.\n");
  }

//  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", client_reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* get home network registration */
static int ublx_af_net_home_do(char *recv_msg)
{
  int ublx_af_net_home_result = FALSE;
  int ublx_af_net_reg_result = FALSE;
  char num_append[20];
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Check home network registration:";
  int ret;
  char *ubus_object = "ublxat";
  char *ubus_method = "at_send_cmd";

  if(!recv_msg)
  {
    printf("Receive a Message NULL.\n");
    return FALSE;
  }

  printf("\n\n\n**********************Start to get home network condition.*********************\n");

  printf("1. Sending command : %s\n", cmd_name_registered_operator);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_registered_operator) == TRUE)
  {
     ublx_af_net_home_result = TRUE;
  }

  printf("2. Sending command : %s\n", cmd_name_check_registered);

  if(client_ubus_process(ubus_object, ubus_method, cmd_name_check_registered) == TRUE)
  {
     ublx_af_net_reg_result = TRUE;
  }

  if(ublx_af_net_home_result == TRUE && ublx_af_net_reg_result == TRUE)
  {
    printf("List home network ip address successful.\n");
    strncat(client_msg, client_reply_msg, strlen(client_reply_msg));
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

static int ublx_af_net_home(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  if(ublx_af_net_home_do(msgstr) == FALSE)
  {
    printf("ublx_af_net_list failed.\n");
  }

//  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", client_reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

  return 0;
}


/***************************************************************/
/*Ubus object method handler function*/

/* send sms */

static int ublx_af_send_sms_do(char *recv_msg, char *num)
{
  int ublx_af_send_sms_result;
  char num_append[20];
  char client_msg[CMD_MSG_MAX_LEN] = "Send message via sms:";
  int ret;
  char *ubus_object = "ublxat";
  char *ubus_method_cmd = "at_send_cmd";
  char *ubus_method_sms = "at_send_sms";
  char cmd_name_send_sms[CMD_LEN] = "at+cmgs=";
  char cmd_name[CMD_LEN];

  if(!recv_msg || !num)
  {
    printf("Receive a Message NULL.\n");
    return FALSE;
  }

  printf("\n\n\n**********************Start to send sms message to number:%s.*********************\n", num); 

  printf("1. Sending command : %s\n", cmd_name_set_sms);

  if(client_ubus_process(ubus_object, ubus_method_cmd, cmd_name_set_sms) == TRUE)
  {
    ublx_af_send_sms_result = TRUE;
  }
  else
  {
    goto send_sms_error;
  }

  printf("2. Sending command : %s with message: %s\n", cmd_name, client_cimi_num);

  if(client_ubus_process_with_message(ubus_object, ubus_method_sms, cmd_name, client_cimi_num) == TRUE)
  {
    ublx_af_send_sms_result = TRUE;
  }
  else
  {
    goto send_sms_error;
  }

  if(ublx_af_send_sms_result == TRUE)
  {
    printf("Send sms successful.\n");
    strncat(client_msg, client_reply_msg, strlen(client_reply_msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }

send_sms_error:
  printf("Send sms failed.\n");
  strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
  strncpy(recv_msg, client_msg, strlen(client_msg));
  return FALSE;
}


static int ublx_af_send_sms(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct blob_attr *tb[__UBLX_UNLOCK_SIM_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;

  int reply_msg_len = CMD_MSG_LEN + strlen(format) + strlen(obj->name);

  char reply_msg[reply_msg_len];

  struct ubus_request_data *hreq = (struct ubus_request_data *)malloc(sizeof(struct ubus_request_data));

  memset(hreq, 0, sizeof(struct ubus_request_data));

  memset(reply_msg, '\0', reply_msg_len);

  memset(msgstr, '\0', CMD_MSG_LEN);

  blobmsg_parse(ublx_af_send_sms_policy, __UBLX_SEND_SMS_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[UBLX_SEND_SMS_NUM])
  {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  if(ublx_af_send_sms_do(msgstr, blobmsg_data(tb[UBLX_SEND_SMS_NUM])) == FALSE)
  {
    printf("ublx send sms failed.\n");
  }

//  sprintf(reply_msg, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, hreq);

  blob_buf_init(&b, 0);

  blobmsg_add_string(&b, "message", client_reply_msg);

  ubus_send_reply(ctx, hreq, b.head);

  ubus_complete_deferred_request(ctx, hreq, 0);

  free(hreq);
  hreq = NULL;

  return 0;
}








