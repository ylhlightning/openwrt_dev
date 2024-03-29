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
 * @file ublx_at_common.h
 *
 * @brief ublx at coomand for ubus communication.
 *
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     03/07/2015
 *
 ***********************************************************/

#ifndef __UBLX_AT_COMMON__
#define __UBLX_AT_COMMON__

#define MODEM_DEFAULT_PORT_NAME "/dev/ttyACM0"
#define MSG_OK "OK"
#define MSG_ERROR "ERROR"

#define CMD_MSG_LEN 1024
#define CMD_LEN     128


int modem_fd;

#endif

