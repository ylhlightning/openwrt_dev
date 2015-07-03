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
 * @file ublx_at_api.h
 *
 * @brief ublx at api handler for ubus wwan object communication.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#ifndef __UBLX_AT_API_H__
#define __UBLX_AT_API_H__

#include <stdlib.h>
#include <libubox/blobmsg_json.h>
#include "lib320u.h"
#include "libubus.h"
#include "ublx_at_common.h"

extern struct ubus_context *ctx;

void ublx_add_object_at(void);

static int ublx_at_send_cmd_do(char *recv_msg, char *cmd);

static void ublx_send_cmd_fd_reply(struct uloop_timeout *t);

static void ublx_send_cmd_reply(struct uloop_timeout *t);

static int ublx_at_send_cmd(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);

static void append_quotation_mark(char *num, char *str_append);

static int ublx_at_send_sms_do(char *recv_msg, char *cmd, char *sms_msg);

static void ublx_send_sms_fd_reply(struct uloop_timeout *t);

static void ublx_send_sms_reply(struct uloop_timeout *t);

static int ublx_at_send_sms(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);




#endif



