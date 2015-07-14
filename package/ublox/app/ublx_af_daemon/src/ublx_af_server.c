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
 * @file ublx_at_server.c
 * 
 * @brief ublx at server for ubus communication.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/
 

#include "ublx_af_server.h"

struct ubus_context *ctx;

static void server_main(void)
{
  int ret;

  ublx_add_object_af();

  printf("AF process run in loop and wait for ubus communcation.....\n\n\n");

  uloop_run();
}

int main(int argc, char **argv)
{
  const char *ubus_socket = NULL;
  int ch;

  while ((ch = getopt(argc, argv, "cs:")) != -1) {
    switch (ch) {
    case 's':
      ubus_socket = optarg;
      break;
    default:
      break;
    }
  }

  argc -= optind;
  argv += optind;

  uloop_init();
  signal(SIGPIPE, SIG_IGN);

  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    fprintf(stderr, "Failed to connect to ubus\n");
    return -1;
  }

  printf("Start to run ublx process AF.\n");

  ubus_add_uloop(ctx);

  server_main();

  ubus_free(ctx);
  uloop_done();

  return 0;
}

