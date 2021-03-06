/*--------------------------------------------------------------------*/
/*    n b s t i m e . c                                               */
/*                                                                    */
/*    Set local system clock from National Bureau of Standards        */
/*    Standard Time service                                           */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*    Copyright (c) 1990-1993 by Kendra Electronic Wonderworks.       */
/*                                                                    */
/*    All rights reserved except those explicitly granted by the      */
/*    UUPC/extended license agreement.                                */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*                          RCS Information                           */
/*--------------------------------------------------------------------*/

/*
 *    $Id: nbstime.c 1.10 1993/09/26 03:32:27 dmwatt Exp $
 *
 *    Revision history:
 *    $Log: nbstime.c $
 * Revision 1.10  1993/09/26  03:32:27  dmwatt
 * Use Standard Windows NT error message module
 *
 * Revision 1.9  1993/09/20  04:46:34  ahd
 * OS/2 2.x support (BC++ 1.0 support)
 * TCP/IP support from Dave Watt
 * 't' protocol support
 *
 * Revision 1.8  1993/07/22  23:22:27  ahd
 * First pass at changes for Robert Denny's Windows 3.1 support
 *
 * Revision 1.7  1993/06/13  14:02:32  dmwatt
 * Additional Windows/NT fixes
 *
 * Revision 1.6  1993/05/30  00:04:53  ahd
 * Multiple communications drivers support
 *
 * Revision 1.5  1993/04/15  04:15:43  ahd
 * Twiddle OS/2 support and reduce number of #ifdef segments
 *
 * Revision 1.4  1993/04/13  02:27:31  dmwatt
 * Windows/NT updates
 *
 * Revision 1.3  1993/04/11  00:34:11  ahd
 * Global edits for year, TEXT, etc.
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#ifdef WIN32

#include <windows.h>
#define NONDOS

#elif defined(FAMILYAPI) || defined(__OS2__)

#define NONDOS
#define INCL_BASE

#include <os2.h>

#elif !defined(__TURBOC__)

#include <dos.h>

#endif /* WIN32 */


/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "arpadate.h"
#include "dcp.h"
#include "dcpsys.h"
#include "hostable.h"
#include "nbstime.h"
#include "script.h"
#include "security.h"
#include "commlib.h"

#if !defined(__TURBOC__) || defined(__OS2__)
   currentfile();
#endif

#if defined(WIN32)
#include "pnterr.h"
#endif

/*--------------------------------------------------------------------*/
/*    n b s t i m e                                                   */
/*                                                                    */
/*    Set system clock from using time from NBS of the format:        */
/*                                                                    */
/*                  MJD  YR MO DA  H  M  S ST S UT1 msADV         OTM */
/*  nbs format-->  47511 88-12-16 06:03:44 00 0 -.1 045.0 UTC(NIST) * */
/*  @ 1200 baud    47511 88-12-16 06:03:45 00 0 -.1 045.0 UTC(NIST) * */
/*--------------------------------------------------------------------*/

boolean nbstime( void )
{
   char buf[BUFSIZ];
   time_t today;
   struct tm  tx;
   int cycles = 15;
   int dst= 0;
   time_t delta;
   char sync = '?';

#ifdef WIN32

   SYSTEMTIME DateTime;
   TOKEN_PRIVILEGES tkp;
   HANDLE hToken;
   USHORT rc;
   DWORD dwError;

#elif defined(FAMILYAPI) || defined(__OS2__)

   DATETIME DateTime;
   struct tm *tp;
   USHORT rc;

#elif !defined(NONDOS) && !defined(__TURBOC__)

   unsigned short rc;
   struct dosdate_t ddate;
   struct dostime_t dtime;
   struct tm *tp;

#endif

   memset( &tx , '\0', sizeof tx);        /* Clear pointers          */
   if (!expectstr("MJD", 5, NULL )) /* Margaret Jane Derbyshire? :-) */
   {
      printmsg(0,"nbstime: Did not find MJD literal in data from remote");
      return FALSE;
   }

   rmsg(buf, 2, 2, sizeof buf);
                  /* Read one line to get us setup for input   */

/*--------------------------------------------------------------------*/
/*                  Begin main loop to get the time                   */
/*--------------------------------------------------------------------*/

   while ((rmsg(buf, 2, 2, sizeof buf) != TIMEOUT) && cycles--)
   {
      sync = buf[ strlen( buf ) - 1 ];

      if (sync == '#')
         break;
      else if (sync != '*')
         *buf = '\0';

   } /* while */

   if ( (cycles && (sync == '*')) || (*buf == '\0'))
   {
      printmsg(0,"nbstime: Did not get good buffer: \"%s\"", buf );
                  return FALSE;
   }

/*--------------------------------------------------------------------*/
/*                   Determine the time we received                   */
/*--------------------------------------------------------------------*/

   sscanf(buf,"%*s %d-%d-%d %d:%d:%d %d ",
         &tx.tm_year, &tx.tm_mon, &tx.tm_mday ,
         &tx.tm_hour, &tx.tm_min, &tx.tm_sec, &dst);
   tx.tm_mon--;               /* Tm record counts months from zero   */

/*--------------------------------------------------------------------*/
/*     mktime()'s "renormalizing" the tm struct is screwing up NT    */
/*--------------------------------------------------------------------*/
#if !defined(WIN32)
   today = mktime(&tx);       /* Current UTC (GMT) time in seconds   */

   if ( debuglevel > 2 )
   {
      printmsg(3,"%2d/%2d/%2d %2d:%2d:%2d %2d %c translates to %ld or %s",
         tx.tm_year, tx.tm_mon + 1 , tx.tm_mday ,
         tx.tm_hour, tx.tm_min, tx.tm_sec, dst, sync ,
         today, ctime( &today ));
   }

/*--------------------------------------------------------------------*/
/*    Perform a sanity check; the time must be 20 years past 1970     */
/*--------------------------------------------------------------------*/

   if ( today < 630720000L )
   {
      printmsg(0,"nbstime: Time warp error (%s), clock not set",
            ctime( &today ));
      return FALSE;
   }
#endif

/*--------------------------------------------------------------------*/
/*                        Set the system clock                        */
/*--------------------------------------------------------------------*/

#if defined(FAMILYAPI) || defined(__OS2__)

   rc = DosGetDateTime( &DateTime );
   if ( rc != 0 )
   {
      printmsg(0,"Return code from DosGetDateTime %d", rc);
      panic();
   }

   printmsg(3,"Date time: %2d/%2d/%2d %2d:%2d:%2d tz %d, weekday %d",
      (int) DateTime.year, (int) DateTime.month, (int) DateTime.day ,
      (int) DateTime.hours, (int) DateTime.minutes,(int) DateTime.seconds ,
      (int) DateTime.timezone, (int) DateTime.weekday );

   today -= timezone;
   tp = localtime(&today);    /* Get local time as a record          */

   DateTime.year    = (USHORT) tp->tm_year + 1900;
   DateTime.month   = (UCHAR) (tp->tm_mon + 1);
   DateTime.day     = (UCHAR) tp->tm_mday;
   DateTime.hours   = (UCHAR) tp->tm_hour;
   DateTime.minutes = (UCHAR) tp->tm_min;
   DateTime.seconds = (UCHAR) tp->tm_sec;

   printmsg(3,"Date time: %2d/%2d/%2d %2d:%2d:%2d tz %d, weekday %d",
      (int) DateTime.year, (int) DateTime.month, (int) DateTime.day ,
      (int) DateTime.hours, (int) DateTime.minutes,(int) DateTime.seconds ,
      (int) DateTime.timezone, (int) DateTime.weekday );

   rc = DosSetDateTime( &DateTime );
   if ( rc != 0 )
   {
      printmsg(0,"Return code from DosGetDateTime %d", rc);
      panic();
   }

#elif defined( WIN32 )

   GetSystemTime( &DateTime );

   printmsg(3,"Date time: %2d/%2d/%2d %2d:%2d:%2d, weekday %d",
      (int) DateTime.wYear, (int) DateTime.wMonth, (int) DateTime.wDay ,
      (int) DateTime.wHour, (int) DateTime.wMinute,(int) DateTime.wSecond ,
      (int) DateTime.wDayOfWeek );

   DateTime.wYear    = (WORD) tx.tm_year + 1900;
   DateTime.wMonth   = (WORD) tx.tm_mon + 1;
   DateTime.wDay     = (WORD) tx.tm_mday;
   DateTime.wHour    = (WORD) tx.tm_hour;
   DateTime.wMinute  = (WORD) tx.tm_min;
   DateTime.wSecond  = (WORD) tx.tm_sec;

   printmsg(3,"Date time: %2d/%2d/%2d %2d:%2d:%2d, weekday %d",
      (int) DateTime.wYear, (int) DateTime.wMonth, (int) DateTime.wDay ,
      (int) DateTime.wHour, (int) DateTime.wMinute, (int) DateTime.wSecond ,
      (int) DateTime.wDayOfWeek );

   if (!OpenProcessToken(GetCurrentProcess(),
      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
      &hToken))
   {
      dwError = GetLastError();
      printmsg(0, "nbstime: OpenProcessToken failed");
      printNTerror("OpenProcessToken", dwError);
      return FALSE;
   }

   LookupPrivilegeValue(NULL, "SeSystemtimePrivilege",
      &tkp.Privileges[0].Luid);
   tkp.PrivilegeCount = 1;
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
      (PTOKEN_PRIVILEGES)NULL, 0))
   {
      dwError = GetLastError();
      printmsg(0, "nbstime: first AdjustTokenPrivilege failed");
      printNTerror("AdjustTokenPrivileges", dwError);
      return FALSE;
   }

   rc = SetSystemTime( &DateTime );
   if ( !rc )
   {
      dwError = GetLastError();
      printmsg(0, "nbstime: SetSystemTime failed");
      printNTerror("SetSystemTime", dwError);
   }

   tkp.Privileges[0].Attributes = 0;

   if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
      (PTOKEN_PRIVILEGES)NULL, 0))
   {
      dwError = GetLastError();
      printmsg(0, "nbstime: AdjustTokenPrivileges disable failed");
      printNTerror("AdjustTokenPrivileges", dwError);
      return FALSE;
   }

#elif defined( __TURBOC__ )

/*--------------------------------------------------------------------*/
/*     Borland C++ doesn't set the time properly; do a conversion     */
/*--------------------------------------------------------------------*/

   today -= timezone;

/*--------------------------------------------------------------------*/
/*    If this timezone uses daylight savings and we are in the        */
/*    period to spring forward, do so.                                */
/*--------------------------------------------------------------------*/

   if (daylight && ( dst > 1 ) && ( dst < 52 ))
      today += 3600;          /* This is valid for the USA only      */
   stime( &today );

#else /* __TURBOC__ */


   tp = localtime(&today);    /* Get local time as a record          */

   ddate.day     = (unsigned char) tp->tm_mday;
   ddate.month   = (unsigned char) (tp->tm_mon + 1);
   ddate.year    = (unsigned int)  (tp->tm_year + 1900);
   ddate.dayofweek = (unsigned char) tp->tm_mday;       /* 0-6, 0=Sunday */

   dtime.hour    = (unsigned char) tp->tm_hour;
   dtime.minute  = (unsigned char) tp->tm_min;
   dtime.second  = (unsigned char) tp->tm_sec;
   dtime.hsecond = (unsigned char) 0;

   printmsg(3,"Date time: %2d/%2d/%2d %2d:%2d:%2d tz %d, weekday %d",
      (int) ddate.year, (int) ddate.month, (int) ddate.day ,
      (int) dtime.hour, (int) dtime.minute,(int) dtime.second ,
      (int) timezone, (int) ddate.dayofweek );

   if ( (rc = _dos_settime( &dtime )) != 0 )
   {
      printmsg(0,"Return code from _dos_settime %d", rc);
      panic();
   }

   if ( (rc = _dos_setdate( &ddate )) != 0 )
   {
      printmsg(0,"Return code from _dos_setdate %d", rc);
      panic();
   }

#endif

/*--------------------------------------------------------------------*/
/*             Print debugging information, if requested              */
/*--------------------------------------------------------------------*/

#if !defined(WIN32)
   delta = today - time( NULL );
   printmsg(2,"nbstime: \"%s\"", buf);
   printmsg(2,"nbstime: Time delta is %ld seconds, zone offset %ld, "
              "daylight savings %d",
                  delta, timezone, dst );
#endif

   if ( sync == '*' )
      printmsg(2,"Warning: Was unable to synchonize with NBS master");

/*--------------------------------------------------------------------*/
/*                Announce new time, return to caller                 */
/*--------------------------------------------------------------------*/

   printmsg(0,"nbstime: New system time is %s", arpadate());
   return TRUE;

} /* nbstime */
