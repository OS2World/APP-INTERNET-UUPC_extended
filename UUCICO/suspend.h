#ifndef SUSPEND_H
#define SUSPEND_H

/*--------------------------------------------------------------------*/
/*    s u s p e n d . h                                               */
/*                                                                    */
/*    suspend/resume uupoll/uucico daemon                             */
/*                                                                    */
/*    Author: Kai Uwe Rommel                                          */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*       Copyright (c) 1993 by Kai Uwe Rommel                         */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*       Changes Copyright (c) 1989-1993 by Kendra Electronic         */
/*       Wonderworks.                                                 */
/*                                                                    */
/*       All rights reserved except those explicitly granted by       */
/*       the UUPC/extended license agreement.                         */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*                          RCS Information                           */
/*--------------------------------------------------------------------*/

/*
 *    $Id: suspend.h 1.2 1993/09/29 04:56:11 ahd Exp $
 *
 *    Revision history:
 *    $Log: suspend.h $
 * Revision 1.2  1993/09/29  04:56:11  ahd
 * Suspend port by port name, not modem file name
 *
 */

#ifndef NO_SUSPEND_FUNCTIONS
extern boolean suspend_processing;

void suspend_init(const char *port );

int suspend_other(const boolean suspend,
                  const char *port );

CONN_STATE suspend_wait(void);
#endif

#define SUSPEND_PIPE "\\PIPE\\UUCICO\\ZZ"

#endif
