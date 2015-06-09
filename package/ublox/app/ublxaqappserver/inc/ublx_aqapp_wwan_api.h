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
 * @file ublx_aqapp_wwan_api.h
 *
 * @brief ublx aqapp api handler for ubus wwan object communication.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/

#ifndef __UBLX_AQAPP_WWAN_API_H__
#define __UBLX_AQAPP_WWAN_API_H__

#include <libubox/blobmsg_json.h>
#include "lib320u.h"
#include "libubus.h"
#include "ublx_aqapp_common.h"

extern struct ubus_context *ctx;

void ublx_add_object_wwan(void);

static int wwan_if_enable_do(char *recv_msg);

static void wwan_if_enable_fd_reply(struct uloop_timeout *t);

static void wwan_if_enable_reply(struct uloop_timeout *t);

static int wwan_if_enable(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);

static int wwan_if_disable_do(char *recv_msg);

static void wwan_if_disable_reply(struct uloop_timeout *t);

static int wwan_if_disable(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);

static int wwan_if_connect_do(char *recv_msg);

static void wwan_if_connect_reply(struct uloop_timeout *t);

static int wwan_if_connect(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);

static int wwan_if_disconnect_do(char *recv_msg);

static void wwan_if_disconnect_reply(struct uloop_timeout *t);

static int wwan_if_disconnect(struct ubus_context *ctx, struct ubus_object *obj,
          struct ubus_request_data *req, const char *method,
          struct blob_attr *msg);

#endif



