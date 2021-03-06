/*--------------------------------------------------------------------*/
/*      d o s 2 u n i x . c                                           */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*    Changes Copyright (c) 1989 by Andrew H. Derbyshire.             */
/*                                                                    */
/*    Changes Copyright (c) 1990-1993 by Kendra Electronic            */
/*    Wonderworks.                                                    */
/*                                                                    */
/*    All rights reserved except those explicitly granted by the      */
/*    UUPC/extended license agreement.                                */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*                          RCS Information                           */
/*--------------------------------------------------------------------*/

/*
 *    $Id: DOS2UNIX.C 1.4 1993/04/10 21:22:29 dmwatt Exp $
 *
 *    Revision history:
 *    $Log: DOS2UNIX.C $
 * Revision 1.4  1993/04/10  21:22:29  dmwatt
 * Windows/NT fixes
 *
 * Revision 1.3  1993/04/05  12:26:01  ahd
 * Correct headers to match gpkt
 *
 * Revision 1.2  1993/04/05  04:32:19  ahd
 * Add timestamp, size to information returned by directory searches
 *
 * Revision 1.1  1993/04/04  19:35:14  ahd
 * Initial revision
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <time.h>
#include <string.h>

/*--------------------------------------------------------------------*/
/*                         OS/2 include files                         */
/*--------------------------------------------------------------------*/

#ifdef FAMILY_API
#define INCL_BASE
#include <os2.h>
#endif

/*--------------------------------------------------------------------*/
/*                      Windows/NT include files                      */
/*--------------------------------------------------------------------*/

#ifdef WIN32
#include <windows.h>
#endif

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "dos2unix.h"

/*--------------------------------------------------------------------*/
/*       d o s 2 u n i x                                              */
/*                                                                    */
/*       Convert a DOS file time stamp to unix t_time                 */
/*--------------------------------------------------------------------*/

time_t dos2unix( const FDATE ddmmyy,
                 const FTIME ssmmhh )
{

   struct tm  time_record;

   time_record.tm_sec = ssmmhh.twosecs * 2;
   time_record.tm_min = ssmmhh.minutes;
   time_record.tm_hour= ssmmhh.hours;

   time_record.tm_mday = ddmmyy.day;
   time_record.tm_mon  = ddmmyy.month - 1;
   time_record.tm_year = 80 + ddmmyy.year;

   time_record.tm_isdst = -1;

   return mktime(&time_record);

} /* dos2unix */

#ifdef WIN32

/*--------------------------------------------------------------------*/
/*       n t 2 u n i x                                              */
/*                                                                    */
/*       Convert an NT file time stamp to unix t_time                 */
/*--------------------------------------------------------------------*/

time_t nt2unix( FILETIME *nsec )
{
   SYSTEMTIME sysTime;
   struct tm  time_record;

   FileTimeToSystemTime(nsec, &sysTime);

   time_record.tm_sec = sysTime.wSecond;
   time_record.tm_min = sysTime.wMinute;
   time_record.tm_hour= sysTime.wHour;

   time_record.tm_mday = sysTime.wDay;
   time_record.tm_mon  = sysTime.wMonth - 1;
   time_record.tm_year = sysTime.wYear - 1900;

   time_record.tm_isdst = -1;

   return mktime(&time_record);

} /* nt2unix */
#endif
