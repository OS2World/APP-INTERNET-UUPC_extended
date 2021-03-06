#ifndef _DCPFPKT_H
#define _DCPFPKT_H

/*--------------------------------------------------------------------*/
/*       d c p f p k t . h                                            */
/*                                                                    */
/*       "f" protocol packet driver for dcp (UUPC/extended data       */
/*       communications)                                              */
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
  *      $Id: dcpfpkt.h 1.4 1993/09/20 04:53:57 ahd Exp $
  *
  *      $Log: dcpfpkt.h $
 * Revision 1.4  1993/09/20  04:53:57  ahd
 * TCP/IP support from Dave Watt
 * 't' protocol support
 * OS/2 2.x support (BC++ 1.0 for OS/2 support)
 *
 * Revision 1.3  1993/04/05  12:27:31  ahd
 * Correct protypes to match gpkt
 *
 * Revision 1.2  1992/11/15  20:08:29  ahd
 * Clean up modem file support for different procotols
 *
 *    08 Sep 90   -  Create via Microsoft C compiler /Zg          ahd
 *    21 Aug 91   -  Create from dcpgpkt.c                        ahd
 *
  */

short  fopenpk(const boolean master);

short  fclosepk(void);

short  fgetpkt(char  *data,short  *len);

short  fsendpkt(char  *data,short  len);

short  fwrmsg(char *str);

short  frdmsg(char *str);

short  feofpkt( void );

short  ffilepkt( void );

#endif
