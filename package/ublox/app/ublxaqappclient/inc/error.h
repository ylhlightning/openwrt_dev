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
 * @file error.h
 * 
 * @brief ublx aqapp client application server error handler.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#ifndef __ERROR_H__
#define __ERROR_H__

/**
* This function defines the error handling strategy.
* In this sample code, it simply prints the given error string and 
* terminates the program.
*/
void error(const char* reason);

#endif
