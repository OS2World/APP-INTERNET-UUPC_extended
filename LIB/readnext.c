/*--------------------------------------------------------------------*/
/*    r e a d n e x t . c                                             */
/*                                                                    */
/*    Reads a spooling directory with optional pattern matching       */
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
 *    $Id: readnext.c 1.5 1993/08/26 05:00:25 ahd Exp $
 *
 *    $Log: readnext.c $
 *     Revision 1.5  1993/08/26  05:00:25  ahd
 *     Debugging code for odd failures on J. McBride's network
 *
 *     Revision 1.4  1993/04/05  04:32:19  ahd
 *     Add timestamp, size to information returned by directory searches
 *
 *     Revision 1.3  1992/11/22  20:58:55  ahd
 *     Use strpool to allocate const strings
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "readnext.h"
#include "uundir.h"
#include "hostable.h"
#include "security.h"

currentfile();

/*--------------------------------------------------------------------*/
/*    r e a d n e x t                                                 */
/*                                                                    */
/*    Read a directory into a linked list                             */
/*--------------------------------------------------------------------*/

char     *readnext(char *xname,
                   const char *remote,
                   const char *subdir,
                   char *pattern,
                   time_t *modified,
                   long   *size )
{
   static DIR *dirp = NULL;
   static char *SaveRemote = NULL;
   static char remotedir[FILENAME_MAX];

   struct direct *dp;

/*--------------------------------------------------------------------*/
/*          Determine if we must restart the directory scan           */
/*--------------------------------------------------------------------*/

   if ( (remote == NULL) || ( SaveRemote == NULL ) ||
        !equal(remote, SaveRemote ) )
   {
      if ( SaveRemote != NULL )   /* Clean up old directory? */
      {                           /* Yes --> Do so           */
         closedir(dirp);
         dirp = NULL;
         SaveRemote = NULL;
      } /* if */

      if ( remote == NULL )      /* Clean up only, no new search? */
         return NULL;            /* Yes --> Return to caller      */

      if ( pattern == NULL )
         pattern = "*.*";

      sprintf(remotedir,"%s/%.8s/%s",E_spooldir,remote, subdir);
      if ((dirp = opendirx(remotedir,pattern)) == nil(DIR))
      {
         printmsg(5, "readnext: couldn't opendir() %s", remotedir);
         dirp = NULL;
         return NULL;
      } /* if */

      SaveRemote = newstr( remote );
                              /* Flag we have an active search    */
   } /* if */

/*--------------------------------------------------------------------*/
/*              Look for the next file in the directory               */
/*--------------------------------------------------------------------*/

   if ((dp = readdir(dirp)) != nil(struct direct))
   {
      sprintf(xname, "%s/%s", remotedir, dp->d_name);
      printmsg(5, "readnext: matched \"%s\"",xname);

      if ( modified != NULL )
         *modified = dp->d_modified;

      if ( size != NULL )
         *size = dp->d_size;

      return xname;
   }

/*--------------------------------------------------------------------*/
/*     No hit; clean up after ourselves and return to the caller      */
/*--------------------------------------------------------------------*/

   printmsg(5, "readnext: \"%s\" not matched", remotedir);
   closedir(dirp);
   SaveRemote = NULL;
   dirp = NULL;
   return NULL;

} /*readnext*/
