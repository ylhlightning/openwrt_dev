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
  char data[CMD_MSG_LEN];
};

static struct blob_buf b;

static int is_network_configured = 0;

static char ublx_wwan_public_ip_addr_msg[CMD_MSG_LEN];

char cmd_name_cfun_enable[CMD_LEN]      = "at+cfun=1";
char cmd_name_cfun_disable[CMD_LEN]     = "at+cfun=0";
char cmd_name_cpin_query[CMD_LEN]       = "at+cpin=?";
char cmd_name_cpin_insert[CMD_LEN]      = "at+cpin=9754";
char cmd_name_cgdcont_enable[CMD_LEN]   = "at+cgdcont=1,\"IP\",\"ibox.tim.it\"";
char cmd_name_context_active[CMD_LEN]   = "at!scact=1,1";
char cmd_name_context_deactive[CMD_LEN] = "at!scact=0,1";
char cmd_name_get_public_addr[CMD_LEN]  = "at!scpaddr=1";
char cmd_name_set_sms[CMD_LEN]          = "at+cmgf=1";



/***************************************************************/
/*Ubus object policy configuration*/

enum {
  WWAN_IF_SENDADDR_NUM,
  __WWAN_IF_SENDADDR_MAX,
};

static const struct blobmsg_policy wwan_if_sendaddr_policy[__WWAN_IF_SENDADDR_MAX] = {
  [WWAN_IF_SENDADDR_NUM] = { .name = "number", .type = BLOBMSG_TYPE_STRING },
};

/***************************************************************/
/*Ubus object wwan method registration*/

static const struct ubus_method wwan_methods[] = {
  {.name = "enable",       .handler = wwan_if_enable},
  {.name = "disable",      .handler = wwan_if_disable},
  {.name = "connect",      .handler = wwan_if_connect},
  {.name = "disconnect",   .handler = wwan_if_disconnect},
  {.name = "getaddr",      .handler = wwan_if_getaddr},
  UBUS_METHOD("sendaddr",  wwan_if_sendaddr, wwan_if_sendaddr_policy),
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

static int get_addr_from_string(char *string, char *addr_string)
{
  char quatation_mark = '"';
  int num_quatation_mark = 0;
  char *addr_str_ptr=addr_string;

  char *tmp_ptr = string;

  while(*tmp_ptr != '\0')
  {
    if(*tmp_ptr == quatation_mark)
      num_quatation_mark ++;

    if((num_quatation_mark == 1) && (*tmp_ptr != quatation_mark))
    {
      *addr_str_ptr = *tmp_ptr;
      addr_str_ptr ++;
    }
    tmp_ptr ++;
  }

  if(num_quatation_mark != 2)
  {
     printf("Error format string\n");
     return -1;
  }

  *addr_str_ptr = '\0';
  return 0;
}



/***************************************************************/
/*Ubus object method handler function*/

/* wwan enable */

static int wwan_if_enable_do(char *recv_msg)
{
  int cmd_cfun_result, cmd_cpin_result;
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "WWAN interface enable: ";
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

  printf("Command to be sent to serial port: %s\n", cmd_name_cpin_insert);

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

  if((cmd_cfun_result == TRUE))
  {
    printf("WWAN interface has already enabled.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface initialization failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
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
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

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
  char client_msg[CMD_MSG_MAX_LEN] = "WWAN interface disable: ";

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
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface disable failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
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
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

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

/*Ubus object method handler function*/

/* wwan connect */

static void wwan_network_configuration()
{
  system("/sbin/ifconfig wwan0 up");

  sleep(0.1);

  system("/sbin/udhcpc -i wwan0");

  sleep(0.1);

  system("echo \"nameserver 8.8.8.8\" > /etc/resolv.conf");

  is_network_configured = 1;

}

static int wwan_if_connect_do(char *recv_msg)
{
  int cmd_cgdcont_result, cmd_active_result;
  char msg[CMD_MSG_MAX_LEN];
  int ret;
  char client_msg[CMD_MSG_MAX_LEN] = "WWAN interface connect: ";

  printf("Command to be sent to serial port: %s\n", cmd_name_cgdcont_enable);

  ret = send_cmd_to_modem(modem_fd, cmd_name_cgdcont_enable);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_cgdcont_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  printf("Command to be sent to serial port: %s\n", cmd_name_context_active);

  ret = send_cmd_to_modem(modem_fd, cmd_name_context_active);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_active_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if((cmd_cgdcont_result == TRUE) && (cmd_active_result == TRUE))
  {
    printf("WWAN interface has actived a connection context.\n");

    if(is_network_configured == 0)
      wwan_network_configuration();

    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface has actived a connection context.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void wwan_if_connect_fd_reply(struct uloop_timeout *t)
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

static void wwan_if_connect_reply(struct uloop_timeout *t)
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

  req->timeout.cb = wwan_if_connect_fd_reply;
  wwan_if_connect_fd_reply(t);
}

static int wwan_if_connect(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(wwan_if_connect_do(msgstr) == FALSE)
  {
    printf("wwan_if_connect failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_connect_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}



/***************************************************************/
/*Ubus object method handler function*/

/* wwan disconnect */

static int wwan_if_disconnect_do(char *recv_msg)
{
  int cmd_disconnect_result;
  char msg[CMD_MSG_MAX_LEN];
  int ret;
  char client_msg[CMD_MSG_MAX_LEN] = "WWAN interface disconnect: ";

  printf("Command to be sent to serial port: %s\n", cmd_name_context_deactive);

  ret = send_cmd_to_modem(modem_fd, cmd_name_context_deactive);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_disconnect_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  is_network_configured = 0;

  if(cmd_disconnect_result == TRUE)
  {
    printf("WWAN interface has already disconnected.\n");
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface disconnect failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void wwan_if_disconnect_fd_reply(struct uloop_timeout *t)
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

static void wwan_if_disconnect_reply(struct uloop_timeout *t)
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

  req->timeout.cb = wwan_if_disconnect_fd_reply;
  wwan_if_disconnect_fd_reply(t);
}

static int wwan_if_disconnect(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(wwan_if_disconnect_do(msgstr) == FALSE)
  {
    printf("wwan_if_disconnect failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);
  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_disconnect_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}

/***************************************************************/
/*Ubus object method handler function*/

/* wwan get pubblic address */

static int wwan_if_getaddr_do(char *recv_msg)
{
  int cmd_getaddr_result;
  char msg[CMD_MSG_MAX_LEN];
  char ip_msg[20];
  char client_msg[CMD_MSG_MAX_LEN] = "WWAN public ip address:";
  int ret;

  memset(ip_msg, 0, strlen(ip_msg));

  strcpy(ip_msg, "");

  printf("Command to be sent to serial port: %s\n", cmd_name_get_public_addr);

  ret = send_cmd_to_modem(modem_fd, cmd_name_get_public_addr);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_getaddr_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_getaddr_result == TRUE)
  {

    get_addr_from_string(msg, ip_msg);

    printf("WWAN interface get public address: %s.\n", ip_msg);

    strncpy(ublx_wwan_public_ip_addr_msg, ip_msg, strlen(ip_msg));
    strncat(client_msg, ip_msg, strlen(ip_msg));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface get public failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void wwan_if_getaddr_fd_reply(struct uloop_timeout *t)
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

static void wwan_if_getaddr_reply(struct uloop_timeout *t)
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

  req->timeout.cb = wwan_if_getaddr_fd_reply;
  wwan_if_getaddr_fd_reply(t);
}

static int wwan_if_getaddr(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(wwan_if_getaddr_do(msgstr) == FALSE)
  {
    printf("wwan_if_getaddr failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_getaddr_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}



/***************************************************************/
/*Ubus object method handler function*/

/* wwan send pubblic address via sms */

static int wwan_if_sendaddr_do(char *recv_msg, char *num)
{
  int cmd_smsconf_result, cmd_sendaddr_result;
  char msg[CMD_MSG_MAX_LEN];
  char client_msg[CMD_MSG_MAX_LEN] = "Send WWAN public ip address via sms:";
  int ret;

  printf("Command to be sent to serial port: %s\n", cmd_name_set_sms);

  ret = send_cmd_to_modem(modem_fd, cmd_name_set_sms);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_smsconf_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  printf("Command to be sent to serial port: AT+CMGS\n");

  ret = send_sms_to_modem(modem_fd, num, ublx_wwan_public_ip_addr_msg);
  if(ret < 0)
  {
     printf("Failed to send command to modem\n");
     return FALSE;
  }

  ret = recv_data_from_modem(modem_fd, &cmd_sendaddr_result, msg);
  if(ret < 0)
  {
     printf("Failed to receive message from modem\n");
     return FALSE;
  }

  if(cmd_smsconf_result == TRUE && cmd_sendaddr_result == TRUE)
  {
    printf("WWAN interface send pubblic address to %s via sms successful.\n", num);
    strncat(client_msg, MSG_OK, strlen(MSG_OK));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return TRUE;
  }
  else
  {
    printf("WWAN interface get public failed.\n");
    strncat(client_msg, MSG_ERROR, strlen(MSG_ERROR));
    strncpy(recv_msg, client_msg, strlen(client_msg));
    return FALSE;
  }
}

static void wwan_if_sendaddr_fd_reply(struct uloop_timeout *t)
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

static void wwan_if_sendaddr_reply(struct uloop_timeout *t)
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

  req->timeout.cb = wwan_if_sendaddr_fd_reply;
  wwan_if_sendaddr_fd_reply(t);
}

static int wwan_if_sendaddr(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg)
{
  struct wwan_if_request *hreq;
  struct blob_attr *tb[__WWAN_IF_SENDADDR_MAX];
  const char *format = "%s received a message: %s";
  char data[CMD_MSG_LEN];
  char *msgstr = data;
  int hreq_size;

  blobmsg_parse(wwan_if_sendaddr_policy, __WWAN_IF_SENDADDR_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[WWAN_IF_SENDADDR_NUM])
    return UBUS_STATUS_INVALID_ARGUMENT;

  hreq_size = sizeof(*hreq) + strlen(format) + strlen(obj->name) + strlen(msgstr) + 1;
  hreq = calloc(1, hreq_size);
  memset(hreq, 0, hreq_size);
  memset(msgstr, 0, CMD_MSG_LEN);

  if(wwan_if_sendaddr_do(msgstr, tb[WWAN_IF_SENDADDR_NUM]) == FALSE)
  {
    printf("wwan_if_sendaddr failed.\n");
  }

  sprintf(hreq->data, format, obj->name, msgstr);

  ubus_defer_request(ctx, req, &hreq->req);
  hreq->timeout.cb = wwan_if_sendaddr_reply;
  uloop_timeout_set(&hreq->timeout, 1000);

  return 0;
}














