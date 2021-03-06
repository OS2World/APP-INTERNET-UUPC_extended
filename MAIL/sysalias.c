/*--------------------------------------------------------------------*/
/*    S y s A l i a s . C                                             */
/*                                                                    */
/*    System wide alias support for UUPC/extended                     */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
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
 *    $Id: SYSALIAS.C 1.3 1993/04/11 00:33:05 ahd Exp $
 *
 *    $Log: SYSALIAS.C $
 * Revision 1.3  1993/04/11  00:33:05  ahd
 * Global edits for year, TEXT, etc.
 *
 * Revision 1.2  1992/12/05  23:38:43  ahd
 * Skip blanks as well as unprintable characters
 *
 * Revision 1.1  1992/12/04  01:00:27  ahd
 * Initial revision
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "deliver.h"
#include "sysalias.h"
#include "hlib.h"

/*--------------------------------------------------------------------*/
/*                          Global variables                          */
/*--------------------------------------------------------------------*/

char *SysAliases = NULL;      /* Name of our system alias file       */
static ALIASTABLE *aliasTable = NULL;
static int aliases = 0;

currentfile();

/*--------------------------------------------------------------------*/
/*                             Prototypes                             */
/*--------------------------------------------------------------------*/

static void InitAlias( void );

/*--------------------------------------------------------------------*/
/*    c h e c k a l i a s                                             */
/*                                                                    */
/*    Check the system alias table for a user.  We perform a linear   */
/*    search on the unordered table we we don't expect to perform     */
/*    many searchs in one execution of the program, and this saves    */
/*    sorting it.                                                     */
/*--------------------------------------------------------------------*/

ALIASTABLE *checkalias( const char *user )
{
   int subscript = 0;

   if ( SysAliases == NULL )
      InitAlias();

   for ( subscript = 0; subscript < aliases; subscript ++ )
   {
      if ( equali(aliasTable[subscript].alias , user ))
         return &aliasTable[subscript];
   } /* for */

/*--------------------------------------------------------------------*/
/*                        No hit, return NULL                         */
/*--------------------------------------------------------------------*/

   return NULL;
} /* checkalias */

/*--------------------------------------------------------------------*/
/*       I n i t A l i a s                                            */
/*                                                                    */
/*       Initialize our system alias table                            */
/*--------------------------------------------------------------------*/

static void InitAlias( void )
{
   char buf[BUFSIZ];
   int subscript  = -1;
   int maxaliases = 64;
   boolean inAlias = FALSE;
   FILE *stream;
   long here;

/*--------------------------------------------------------------------*/
/*            Build the file name and try to open the file            */
/*--------------------------------------------------------------------*/

   mkfilename( buf, E_confdir, "aliases" );
   SysAliases = newstr( buf );

   stream = FOPEN( SysAliases , "r",TEXT_MODE );
   if ( stream == NULL )
   {
      if (debuglevel > 1)
         printerr( SysAliases );
      return;
   } /* if */

/*--------------------------------------------------------------------*/
/*                 The file is open, allocate a table                 */
/*--------------------------------------------------------------------*/

   aliasTable = malloc( sizeof *aliasTable * maxaliases );

   here = ftell( stream );       /* Remember location in file        */

/*--------------------------------------------------------------------*/
/*                    Begin loop to process names                     */
/*--------------------------------------------------------------------*/

   while (fgets( buf , BUFSIZ , stream ) != NULL )
   {
      char *s = buf;

/*--------------------------------------------------------------------*/
/*    Ignore comments, lines that begin with whitespace, and empty    */
/*    lines                                                           */
/*--------------------------------------------------------------------*/

      while( *s && ! isgraph( *s ))
         s++;

      if (*s == '#')
         continue;

      if ( ! *s )                /* Empty line?                      */
      {                          /* Yes --> 'tis end of alias        */
         if ( inAlias )
         {
            inAlias = FALSE;
            if (aliasTable[subscript].start == -1 )
               printmsg(0,"%s: Invalid alias %s, no data defined!",
                  SysAliases, aliasTable[subscript].alias );
            else
               aliasTable[subscript].end = here;
         }
      } /* if ( ! *s ) */
      else if (inAlias)
      {
         if (aliasTable[subscript].start == -1 )
            aliasTable[subscript].start = here + (s - buf);
      }
      else {                           /* Start of a new alias */
         char *colon = strchr( s, ':' );

         if (s != buf )
            printmsg(0,"%s: Error aliases must begin in column 1",
                  SysAliases );

         if (colon == NULL )
         {
            printmsg(0,"%s: No colon after alias %s",
                  SysAliases, s);
            continue;
         }

         if ( subscript+2 == maxaliases )
         {
            maxaliases *= 2;
            aliasTable = realloc( aliasTable,
                                  maxaliases * sizeof *aliasTable );
            checkref( aliasTable );
         }

         *colon = '\0';          /* Terminate the name               */
         aliasTable[++subscript].alias = newstr(strtok(s,WHITESPACE));
                                 /* Save name of alias               */
         s = strtok( colon+1, WHITESPACE);
                                 /* Find next token                  */

         if ((s == NULL) || (*s == '#'))  /* Any alias on same?      */
             aliasTable[subscript].start = -1;
                                 /* No--> Look for it later          */
         else
             aliasTable[subscript].start = here + ( s - buf );

          aliasTable[subscript].end  = -1;
          inAlias = TRUE;
      } /* if */

      here = ftell( stream );       /* Remember start of next line   */

   } /* while */

/*--------------------------------------------------------------------*/
/*                   Clean up and return to caller                    */
/*--------------------------------------------------------------------*/

   if ( inAlias )
      aliasTable[subscript].end = LONG_MAX;

   aliases = subscript + 1;
   if ( aliases == 0 )
      free( aliasTable );
   else
      aliasTable = realloc( aliasTable, aliases * sizeof *aliasTable );
   fclose( stream );

} /* InitAlias */
