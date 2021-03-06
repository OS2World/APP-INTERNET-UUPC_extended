/*--------------------------------------------------------------------*/
/*       p n t e r r . c                                              */
/*                                                                    */
/*       Report error message from NT error library                   */
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
 *    $Id: pnterr.c 1.1 1993/09/25 03:02:37 ahd Exp $
 *
 *    Revision history:
 *    $Log: pnterr.c $
 * Revision 1.1  1993/09/25  03:02:37  ahd
 * Initial revision
 *
 * Revision 1.1  1993/09/24  21:43:27  dmwatt
 * Initial revision
 *
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <errno.h>

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "pnterr.h"

currentfile();

/*--------------------------------------------------------------------*/
/*    p N T e r r                                                     */
/*                                                                    */
/*    Perform a perror() with logging                                 */
/*--------------------------------------------------------------------*/

void pNTErr(const size_t lineno,
             const char *fname,
             const char *prefix,
             DWORD rc)
{
   char buf[BUFSIZ];
   static boolean recursion  = FALSE;
   int l;
   boolean redirect = ((logfile != stdout) && !isatty(fileno(stdout)));

   DWORD xrc;
   xrc = FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, rc, LANG_USER_DEFAULT, buf, BUFSIZ, NULL);

   if ( xrc == 0 )
   {

      if ( ! recursion )
      {
         recursion = TRUE;
         printNTerror( "FormatMessage", xrc );
         recursion = FALSE;
      } /* recursion */

      sprintf(buf, "NT API error %u in %s at line %d, cannot find message",
                   (unsigned int) rc,
                   fname,
                   lineno );

   } /* if ( xrc == 0 ) */

/*--------------------------------------------------------------------*/
/*    Drop extra new from error message if we have room in our        */
/*    small buffer                                                    */
/*--------------------------------------------------------------------*/

   l = strlen( buf );

   if (( buf[l-1] == '\n') & (l < sizeof buf ))
      buf[l-1] = '\0';          /* Drop extra newline from string      */

/*--------------------------------------------------------------------*/
/*           Display the message with option file location            */
/*--------------------------------------------------------------------*/

   printmsg(2,"NT API error %d in %s at line %d ...",
            (int) rc, fname, lineno );

   printmsg(0,"%s: %s", prefix, buf);

   if ( redirect )
      fprintf(stdout,"%s: %s\n", prefix, buf);

} /* pNTErr */
