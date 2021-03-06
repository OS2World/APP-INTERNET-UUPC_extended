/*--------------------------------------------------------------------*/
/*          u u x . c                                                 */
/*                                                                    */
/*          Queue remote commands for UUCP under UUPC/extended        */
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
 *    $Id: uux.c 1.8 1993/10/03 20:43:08 ahd Exp $
 *
 *    Revision history:
 *    $Log: uux.c $
 * Revision 1.8  1993/10/03  20:43:08  ahd
 * Normalize comments to C++ double slash
 *
 * Revision 1.7  1993/10/01  01:17:44  ahd
 * Additional correct from Richard Gumpertz
 *
 * Revision 1.6  1993/09/28  01:38:19  ahd
 * Corrections from Robert H. Gumpertz (rhg@cps.com)
 *
 * Revision 1.5  1993/09/20  04:48:25  ahd
 * TCP/IP support from Dave Watt
 * 't' protocol support
 * OS/2 2.x support (BC++ 1.0 for OS/2)
 *
 */

/*
      Program:    uux.c              27 August 1991
      Author:     Mitch Mitchell
      Email:      mitch@harlie.lonestar.org

      Much of this code is shamelessly taken from extant code in
      UUPC/Extended.

      Usage:      uux [ options ] command-string

               Where [ options ] are:

     -aname    Use name as the user identification replacing the initiator
               user-id.  (Notification will be returned to the user.)

     -b        Return whatever standard input was provided to the uux command
               if the exit status is non-zero.

     -c        Do not copy local file to the spool directory for transfer to
               the remote machine (default).

     -C        Force the copy of local files to the spool directory for
               transfer.

     -e        Remote system should use sh to execute commands.

     -E        Remote system should use exec to execute commands.

     -ggrade   Grade is a single letter/number; lower ASCII sequence
               characters will cause the job to be transmitted earlier during
               a particular conversation.

     -j        Output the jobid ASCII string on the standard output which is
               the job identification.  This job identification can be used by
               uustat to obtain the status or terminate a job.

     -n        Do not notify the user if the command fails.

     -p        The standard input to uux is made the standard input to the
               command-string.

     -r        Do not start the file transfer, just queue the job.
               (Currently uux does not attempt to start the transfer
                regardless of the presense of this option).

     -sfile    Report status of the transfer in file.

     -xdebug_level
               Produce debugging output on the standard output.  The
               debug_level is a number between 0 and ??; higher numbers give
               more detailed information.

     -z        Send success notification to the user.


      The command-string is made up of one or more arguments that
      look like a normal command line, except that the command and
      filenames may be prefixed by system-name!.  A null
      system-name is interpreted as the local system.

*/

/*--------------------------------------------------------------------*/
/*         System include files                                       */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _Windows
#include <windows.h>
#endif

/*--------------------------------------------------------------------*/
/*         Local include files                                        */
/*--------------------------------------------------------------------*/

#include  "lib.h"
#include  "hlib.h"
#include  "getopt.h"
#include  "getseq.h"
#include  "expath.h"
#include  "import.h"
#include  "pushpop.h"
#include  "security.h"
#include  "hostable.h"
#include  "timestmp.h"

#ifdef _Windows
#include "winutil.h"
#include "logger.h"
#endif

/*--------------------------------------------------------------------*/
/*        Define current file name for panic() and printerr()         */
/*--------------------------------------------------------------------*/

currentfile();

/*--------------------------------------------------------------------*/
/*                          Global variables                          */
/*--------------------------------------------------------------------*/

typedef enum {
          FLG_USE_USERID,
          FLG_OUTPUT_JOBID,
          FLG_READ_STDIN,
          FLG_QUEUE_ONLY,
          FLG_NOTIFY_SUCCESS,
          FLG_NONOTIFY_FAIL,
          FLG_COPY_SPOOL,
          FLG_RETURN_STDIN,
          FLG_STATUS_FILE,
          FLG_USE_EXEC,
          FLG_MAXIMUM
       } UuxFlags;

typedef enum {
      DATA_FILE   = 0,        // Normal data file passed argument
      INPUT_FILE  = 1,        // Redirected stdin file
      OUTPUT_FILE = 2         // Redirected stdout file
      } FileType;

static boolean flags[FLG_MAXIMUM] = {
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        FALSE
                                    };
static char* st_out = NULL;
static char* user_id = NULL;
static char  grade = 'Z';          // Default grade of service

static char  job_id[15];

static char* spool_fmt = SPOOLFMT;
static char* dataf_fmt = DATAFFMT;

static char* send_cmd  = "S %s %s %s - %s 0666\n";

/*--------------------------------------------------------------------*/
/*                        Internal prototypes                         */
/*--------------------------------------------------------------------*/

void main(int  argc, char  **argv);
static void usage( void );
static char *SwapSlash(char *p);
static boolean cp(char *from, char *to);
static boolean split_path(char *path,
                          char *system,
                          char *file,
                          boolean expand,
                          char *default_sys);
static boolean CopyData( const char *input, const char *output);

static boolean do_uuxqt(char *job_name, char *src_syst, char *src_file, char *dest_syst, char *dest_file);

static boolean do_copy(char *src_syst, char *src_file, char *dest_syst, char *dest_file);

static boolean do_remote(int optind, int argc, char **argv);
static void preamble(FILE* stream);
static char subseq( void );

/*--------------------------------------------------------------------*/
/*    u s a g e                                                       */
/*                                                                    */
/*    Report flags used by program                                    */
/*--------------------------------------------------------------------*/

static void usage()
{
      fprintf(stderr, "Usage: uux\t[-c|-C] [-e|-E] [-b] [-gGRADE] "
                      "[-p] [-j] [-n] [-r] [-sFILE]\\\n"
                      "\t\t[-aNAME] [-z] [-] [-xDEBUG_LEVEL] "
                      "command-string\n");
}


/*--------------------------------------------------------------------*/
/*    s w a p s l a s h                                               */
/*                                                                    */
/*    Change backslash in a directory path to forward slash           */
/*--------------------------------------------------------------------*/

static char *SwapSlash(char *p)
{
     char *q = p;

     while (*q) {
        if (*q ==  '\\')
           *q = '/';
        q++;
     }
     return p;
};

/*--------------------------------------------------------------------*/
/*    c p                                                             */
/*                                                                    */
/*    Copy Local Files                                                */
/*--------------------------------------------------------------------*/

static boolean cp(char *from, char *to)
{
      int  fd_from, fd_to;
      int  nr;
      int  nw = -1;
      char buf[BUFSIZ];            // faster if we alloc a big buffer

      /* This would be even faster if we determined that both files
         were on the same device, dos >= 3.0, and used the dos move
         function */

      if ((fd_from = open(from, O_RDONLY | O_BINARY)) == -1)
         return FALSE;        // failed

      /* what if the to is a directory? */
      /* possible with local source & dest uucp */

      if ((fd_to = open(to, O_CREAT | O_BINARY | O_WRONLY, S_IWRITE | S_IREAD)) == -1) {
         close(fd_from);
         return FALSE;        // failed
         /* NOTE - this assumes all the required directories exist!  */
      }

      while  ((nr = read(fd_from, buf, sizeof buf)) > 0 &&
         (nw = write(fd_to, buf, nr)) == nr)
         ;

      close(fd_to);
      close(fd_from);

      if (nr != 0 || nw == -1)
         return FALSE;        // failed in copy
      return TRUE;
}


/*--------------------------------------------------------------------*/
/*    C o p y D a t a                                                 */
/*                                                                    */
/*    Copy data into its final resting spot                           */
/*--------------------------------------------------------------------*/

static boolean CopyData( const char *input, const char *output)
{
   FILE    *datain;
   FILE    *dataout;
   char     buf[BUFSIZ];
   boolean  status = TRUE;
   size_t   len;

   if ( (dataout = FOPEN(output, "w", BINARY_MODE)) == NULL )
   {
      printerr(output);
      printmsg(0,"uux: Cannot open spool file \"%s\" for output",
               output);
      return FALSE;
   }

/*--------------------------------------------------------------------*/
/*                      Verify the input opened                       */
/*--------------------------------------------------------------------*/

   if (input == NULL)
   {
      datain = stdin;
      setmode(fileno(datain), O_BINARY);   // Don't die on control-Z, etc
   }
   else
      datain = FOPEN(input, "r", BINARY_MODE);

   if (datain == NULL) {
      printerr(input);
      printmsg(0,"Unable to open input file \"%s\"",
               (input == NULL ? "stdin" : input));
      fclose(dataout);
      return FALSE;
   } /* datain */

/*--------------------------------------------------------------------*/
/*                       Loop to copy the data                        */
/*--------------------------------------------------------------------*/

   while ( (len = fread( buf, 1, BUFSIZ, datain)) != 0)
   {
      if ( fwrite( buf, 1, len, dataout ) != len)     // I/O error?
      {
         printerr("dataout");
         printmsg(0,"I/O error on \"%s\"", output);
         fclose(dataout);
         return FALSE;
      } /* if */
   } /* while */

/*--------------------------------------------------------------------*/
/*                      Close up shop and return                      */
/*--------------------------------------------------------------------*/

   if (ferror(datain))        // Clean end of file on input?
   {
      printerr(input);
      clearerr(datain);
      status = FALSE;
   }

   if (input != NULL)
       fclose(datain);

   fclose(dataout);
   return status;

} /* CopyData */

/*--------------------------------------------------------------------*/
/*    s p l i t _ p a t h                                             */
/*--------------------------------------------------------------------*/

static boolean split_path(char *path,
                          char *sysname,
                          char *file,
                          boolean expand,
                          char *default_sys )
{
      char *p_left;
      char *p_right;
      char *p = path;

      *sysname = *file = '\0';    // init to nothing

/*--------------------------------------------------------------------*/
/*                if path is wildcarded then error                    */
/*--------------------------------------------------------------------*/

   if (strcspn(path, "*?[") < strlen(path))
   {
      printmsg(0,"uux - Wildcards not allowed in operand: %s",p );
      return FALSE;
   }

/*--------------------------------------------------------------------*/
/*                        Find the first bangs                        */
/*--------------------------------------------------------------------*/

   p_left = strchr(p, '!');         // look for the first bang

/*--------------------------------------------------------------------*/
/*   If no bangs, then the file is on the remote system.  We hope.    */
/*--------------------------------------------------------------------*/

   if ( p_left == NULL )
   {
      strcpy( file, p);       // Entire string is file name

      strcpy( sysname, default_sys );   // Use default system name

      if ( equal(sysname, E_nodename ) &&
           expand &&
           (expand_path(file, NULL, E_homedir, NULL) == NULL))
         return FALSE;     /* expand_path will delivery any needed
                                 nasty-gram to user                  */

      return TRUE;

   } /* if ( p_left == NULL ) */

/*--------------------------------------------------------------------*/
/*                         Find the last bang                         */
/*--------------------------------------------------------------------*/

   p_right = strrchr(p, '!');       // look for the last bang

/*--------------------------------------------------------------------*/
/*    If the first bang is the first character, it's a local file     */
/*--------------------------------------------------------------------*/

      if (p_left == p)                 // First character in path?
      {                                // Yes --> not a remote path

         if ( p_left != p_right )      // More bangs?
         {
            printmsg(0,"uux - Invalid syntax for local file: %s", p );
            return FALSE;              // Yes --> I don't grok this
         }

         strcpy(file, p+1);            // Just return filename

         if ( expand && (expand_path(file, NULL, E_homedir, NULL) == NULL))
            return FALSE;     /* expand_path will delivery any needed
                                 nasty-gram to user                  */
         strcpy(sysname, E_nodename);
         return TRUE;
      } /* p_left == p */

/*--------------------------------------------------------------------*/
/*             It's not a local file, continue processing             */
/*--------------------------------------------------------------------*/

      strcpy(file, p_right + 1);      // and thats our filename

      strncpy(sysname, p, p_left - p); // and we have a system thats not us
      sysname[p_left - p] = '\0';

/*--------------------------------------------------------------------*/
/*              Now see if there is an intermediate path              */
/*--------------------------------------------------------------------*/

      if (p_left != p_right)
      {

          printmsg(0, "uux - Intermediate system %.*s not supported",
                     p_right - (p_left + 1), p_left + 1);
          return FALSE;

      } /* if (p_left != p_right) */

      return TRUE;                     // and we're done

} /* split_path */

/*--------------------------------------------------------------------*/
/*    d o _ u u x q t                                                 */
/*                                                                    */
/*    Generate a UUXQT command file for local system                  */
/*--------------------------------------------------------------------*/

static boolean do_uuxqt(char *job_name,
                        char *src_syst,
                        char *src_file,
                        char *dest_syst,
                        char *dest_file)
{
   long seqno = 0;
   char *seq  = NULL;
   FILE *stream;              // For writing out data

   char msfile[FILENAME_MAX]; // MS-DOS format name of files
   char msname[22];           // MS-DOS format w/o path name
   char ixfile[15];           /* eXecute file for local system,
                                 UNIX format name for local system    */

/*--------------------------------------------------------------------*/
/*          Create the UNIX format of the file names we need          */
/*--------------------------------------------------------------------*/

   seqno = getseq();
   seq = JobNumber( seqno );

   sprintf(ixfile, spool_fmt, 'X', E_nodename, grade , seq);

/*--------------------------------------------------------------------*/
/*                     create local X (xqt) file                      */
/*--------------------------------------------------------------------*/

   importpath( msname, ixfile, E_nodename);
   mkfilename( msfile, E_spooldir, msname);

   if ( (stream = FOPEN(msfile, "w", BINARY_MODE)) == NULL ) {
      printerr(msfile);
      printmsg(0, "uux: cannot open X file %s", msfile);
      return FALSE;
   } /* if */

   fprintf(stream, "# third party request, job id\n" );
   fprintf(stream, "J %s\n",               job_name );
   fprintf(stream, "F %s/%s/%s %s\n",      E_spooldir, src_syst, dest_file,
                                          src_file );
   fprintf(stream, "C uucp -C %s %s!%s\n", src_file, dest_syst, dest_file );
   fclose (stream);

   return TRUE;

} /* do_uuxqt */

/*--------------------------------------------------------------------*/
/*    d o _ c o p y                                                   */
/*                                                                    */
/*    At this point only one of the systems can be remote and only    */
/*    1 hop away.  All the rest have been filtered out                */
/*--------------------------------------------------------------------*/

static boolean do_copy(char *src_syst,
                       char *src_file,
                       char *dest_syst,
                       char *dest_file)
{
      char    tmfile[25];               // Unix style name for c file
      char    idfile[25];       // Unix style name for data file copy
      char    work[66];             // temp area for filename hacking
      char    icfilename[66];               // our hacked c file path
      char    idfilename[66];               // our hacked d file path

      struct  stat    statbuf;

      long    int     sequence;
      char    *remote_syst;    // Non-local system in copy
      char    *sequence_s;
      FILE        *cfile;

      sequence = getseq();
      sequence_s = JobNumber( sequence );

      remote_syst =  equal(src_syst, E_nodename) ? dest_syst : src_syst;

      sprintf(tmfile, spool_fmt, 'C', remote_syst, grade, sequence_s);
      importpath(work, tmfile, remote_syst);
      mkfilename(icfilename, E_spooldir, work);

      if (!equal(src_syst, E_nodename))
      {
         if (expand_path(dest_file, NULL, E_homedir, NULL) == NULL)
            return FALSE;

         SwapSlash(src_file);

         printmsg(1, "uux - from \"%s\" - control = %s", src_syst,
                  tmfile);
         if ((cfile = FOPEN(icfilename, "a",TEXT_MODE)) == NULL)  {
            printerr( icfilename );
            printmsg(0, "uux: cannot append to %s\n", icfilename);
            return FALSE;
         }

         fprintf(cfile, "R %s %s %s -c D.0 0666", src_file, dest_file,
                  E_mailbox);

         if (flags[FLG_USE_USERID])
             fprintf(cfile, " %s\n", user_id);
         else
             fprintf(cfile, " %s\n", E_mailbox);


         fclose(cfile);
         return TRUE;
      }
      else if (!equal(dest_syst, E_nodename))  {

         printmsg(1,"uux - spool %s - execute %s",
                  flags[FLG_COPY_SPOOL] ? "on" : "off",
                  flags[FLG_QUEUE_ONLY] ? "do" : "don't");
         printmsg(1,"     - dest m/c = %s  sequence = %ld  control = %s",
                  dest_syst, sequence, tmfile);

         if (expand_path(src_file, NULL, E_homedir, NULL) == NULL)
            return FALSE;

         SwapSlash(dest_file);

         if (stat(src_file, &statbuf) != 0)  {
            printerr( src_file );
            return FALSE;
         }

         if (statbuf.st_mode & S_IFDIR)  {
            printf("uux - directory name \"%s\" illegal\n",
                    src_file );
            return FALSE;
         }

         if (flags[FLG_COPY_SPOOL]) {
            sprintf(idfile , dataf_fmt, 'D', E_nodename, sequence_s,
                              subseq());
            importpath(work, idfile, remote_syst);
            mkfilename(idfilename, E_spooldir, work);

            /* Do we need a MKDIR here for the system? */

            if (!cp(src_file, idfilename))  {
               printmsg(0, "copy \"%s\" to \"%s\" failed",
                  src_file, idfilename);           // copy data
               return FALSE;
            }
         }
         else
            strcpy(idfile, "D.0");

         if ((cfile = FOPEN(icfilename, "a",TEXT_MODE)) == NULL)
         {
            printerr( icfilename );
            printf("uux: cannot append to %s\n", icfilename);
            return FALSE;
         } /* if ((cfile = FOPEN(icfilename, "a",TEXT_MODE)) == NULL) */

         fprintf(cfile, "S %s %s %s -%s %s 0666", src_file, dest_file,
                  E_mailbox, flags[FLG_COPY_SPOOL] ? "c" : " ", idfile);

         if (flags[FLG_USE_USERID])
             fprintf(cfile, " %s\n", user_id);
         else
             fprintf(cfile, " %s\n", E_mailbox);

         fclose(cfile);

         return TRUE;
      }
      else {
         if (expand_path(src_file, NULL, E_homedir, NULL) == NULL)
            return FALSE;

         if (expand_path(dest_file, NULL, E_homedir, NULL) == NULL)
            return FALSE;

         if (strcmp(src_file, dest_file) == 0)
         {
            printmsg(0, "%s %s - same file; can't copy\n",
                  src_file, dest_file);
            return FALSE;
         } /* if (strcmp(src_file, dest_file) == 0) */

         return(cp(src_file, dest_file));
      } /* else */
} /* do_copy */

/*--------------------------------------------------------------------*/
/*    p r e a m b l e                                                 */
/*                                                                    */
/*    write the execute file preamble based on the global flags       */
/*--------------------------------------------------------------------*/

static void preamble(FILE* stream)
{

     fprintf(stream, "U %s %s\n", E_mailbox, E_nodename);

     if (flags[FLG_RETURN_STDIN]) {
         fprintf(stream, "# return input on abnormal exit\n");
         fprintf(stream, "B\n");
     }

     if (flags[FLG_NOTIFY_SUCCESS]) {
         fprintf(stream, "# return status on success\n");
         fprintf(stream, "n\n");
     }

     if (flags[FLG_NONOTIFY_FAIL]) {
         fprintf(stream, "# don't return status on failure\n");
         fprintf(stream, "N\n");
     } else {
         fprintf(stream, "# return status on failure\n");
         fprintf(stream, "Z\n");
     }

     if (flags[FLG_USE_EXEC]) {
         fprintf(stream, "# use exec to execute\n");
         fprintf(stream, "E\n");
     } else {
         fprintf(stream, "# use sh execute\n");
         fprintf(stream, "e\n");
     }

     if (flags[FLG_STATUS_FILE]) {
        fprintf(stream, "M %s\n", st_out );
     }

     if (flags[FLG_USE_USERID]) {
         fprintf(stream, "# return address for status or input return\n");
         fprintf(stream, "R %s\n", user_id );
     }

     fprintf(stream, "# job id for status reporting\n");
     fprintf(stream, "J %s\n", job_id );
     return;
} /* preamble */

/*--------------------------------------------------------------------*/
/*    d o _ r e m o t e                                               */
/*                                                                    */
/*   gather data files to ship to execution system and build X file   */
/*--------------------------------------------------------------------*/

static boolean do_remote(int optind, int argc, char **argv)
{
   FILE    *stream;           // For writing out data
   char    *sequence_s;

   boolean s_remote;
   boolean d_remote;
   boolean i_remote = FALSE;
   boolean o_remote = FALSE;

   long    sequence;

   char    src_system[100];
   char    dest_system[100];
   char    src_file[FILENAME_MAX];
   char    dest_file[FILENAME_MAX];

   char    command[BUFSIZ];

   char    msfile[FILENAME_MAX];    // MS-DOS format name of files
   char    msname[22];              // MS-DOS format w/o path name

   char    tmfile[15];        // Call file, UNIX format name
   char    lxfile[15];        /* eXecute file for remote system,
                                 UNIX format name for local system */
   char    rxfile[15];        /* Remote system UNIX name of eXecute
                                 file                              */
   char    lifile[15];        // Data file, UNIX format name
   char    rifile[15];        /* Data file name on remote system,
                                 UNIX format                       */
   char* jobid_fmt = &spool_fmt[3];


/*--------------------------------------------------------------------*/
/*    Get the remote system and command to execute on that system     */
/*--------------------------------------------------------------------*/

   if (!split_path(argv[optind++], dest_system, command, FALSE, E_nodename))
   {
      printmsg(0, "uux - illegal syntax %s", argv[--optind]);
      return FALSE;
   }

   d_remote = equal(dest_system, E_nodename) ? FALSE : TRUE ;

/*--------------------------------------------------------------------*/
/*        OK - we have a destination system - do we know him?         */
/*--------------------------------------------------------------------*/

   if ((d_remote) && (checkreal(dest_system) == BADHOST))
   {
      printmsg(0, "uux - bad system: %s", dest_system);
      return FALSE;
   }

   printmsg(9,"xsys -> %s", dest_system);
   printmsg(9, "system \"%s\", rest \"%s\"", dest_system, command);

   sequence = getseq();
   sequence_s = JobNumber( sequence );

   sprintf(job_id, jobid_fmt, dest_system, grade, sequence_s);

/*--------------------------------------------------------------------*/
/*                     create remote X (xqt) file                     */
/*--------------------------------------------------------------------*/

      sprintf(rxfile, dataf_fmt, 'X', E_nodename, sequence_s, subseq());
      sprintf(lxfile, dataf_fmt, d_remote ? 'D' : 'X', E_nodename,
              sequence_s, subseq());

      importpath( msname, lxfile, dest_system);
      mkfilename( msfile, E_spooldir, msname);

      if ( (stream = FOPEN(msfile, "w", BINARY_MODE)) == NULL ) {
         printerr(msfile);
         printmsg(0, "uux: cannot open X file %s", msfile);
         return FALSE;
      } /* if */

      preamble(stream);

/*--------------------------------------------------------------------*/
/*           Process options for the remote command                   */
/*--------------------------------------------------------------------*/

      for (; optind < argc; optind++)
      {

         FileType f_remote = DATA_FILE;
         char *remote_file;

         switch (*argv[optind])
         {
             case '-':
             strcat(command," ");
             strcat(command,argv[optind]);
             printmsg(9, "prm -> %s", argv[optind]);
             continue;

             case '<':
             if (i_remote) {
                 printmsg(0, "uux - multiple input files specified");
                 return FALSE;
             }
             else
                 i_remote = TRUE;
             f_remote = INPUT_FILE;
             printmsg(9, "prm -> %c", *argv[optind]);
             if (!*++argv[optind])
                 if (++optind >= argc)
                 {
                 printmsg(0, "uux - no input file specified after <");
                 return FALSE;
                 }
             break;

             case '>':
             if (o_remote) {
                 printmsg(0, "uux - multiple output files specified");
                 return FALSE;
             } else
                 o_remote = TRUE;
             f_remote = OUTPUT_FILE;
             printmsg(9, "prm -> %c", *argv[optind]);
             if (!*++argv[optind])
                 if (++optind >= argc)
                 {
                 printmsg(0, "uux - no output file specified after >");
                 return FALSE;
                 }
             break;

             case '|':
             printmsg(9, "prm -> %c", *argv[optind]);
             if (!*++argv[optind])
                 if (++optind >= argc)
                 {
                 printmsg(0, "uux - no command specified after |");
                 return FALSE;
                 }
             if (strchr(argv[optind], '!'))
                 {
                 printmsg(0, "uux - no host name allowed after |");
                 return FALSE;
                 }
             strcat(command," | ");
             strcat(command, argv[optind]);
             continue;

             case '(':
             {
                 size_t len = strlen(argv[optind] + 1);

                 if (argv[optind][len] != ')')
                 {
                 printmsg(0, "uux - missing close parenthesis in %s",
                         argv[optind] + 1);
                 return FALSE;
                 }
                 argv[optind][len] = '\0';
             }

             printmsg(9, "prm -> %s", argv[optind]);
             strcat(command," ");
             strcat(command, argv[optind] + 1);
             continue;

             /* default: fall through */
         } /* switch (*argv[optind]) */


         printmsg(9, "prm -> %s", argv[optind]);

/*--------------------------------------------------------------------*/
/*        Hmmm.  Do we want the remote to have DOS style path?        */
/*--------------------------------------------------------------------*/

         if (!split_path(argv[optind], src_system, src_file, FALSE, dest_system))
         {
            printmsg(0, "uux - illegal syntax %s", argv[optind]);
            return FALSE;
         } /* if (!split_path()) */

         s_remote = equal(src_system, E_nodename) ? FALSE : TRUE ;

/*--------------------------------------------------------------------*/
/*                   Do we know the source system?                    */
/*--------------------------------------------------------------------*/

         if ((s_remote) && (checkreal(src_system) == BADHOST))
         {
            printmsg(0, "uux - bad system %s\n", src_system);
            return FALSE;
         } /* if ((s_remote) && (checkreal(src_system) == BADHOST)) */

         if (f_remote == OUTPUT_FILE)
         {
            fprintf(stream, "O %s %s\n", src_file,
                (equal(src_system, dest_system) ? " " : src_system) );
            continue;
         } /* if (f_remote == OUTPUT_FILE) */

         remote_file = src_file;
         if (!equal(src_system, dest_system))
         {
             remote_file += strlen(src_file);
             while (remote_file > src_file  // Keep trailing / and :
             && (*--remote_file == '/'
                 /* || *remote_file == '\\' */
                 || *remote_file == ':'))
            ;
             while (remote_file > src_file  // Stop at other / and :
                && remote_file[-1] != '/'
                /* && remote_file[-1] != '\\' */
                && remote_file[-1] != ':')
            --remote_file;
             /* remote_file is now src_file without any leading drive/path */
        } /* if (!equal(src_system, dest_system)) */

        if (f_remote == DATA_FILE)
        {

           strcat(command, " ");
           strcat(command, remote_file);

        } /* if (f_remote == DATA_FILE) */
        else if (f_remote == INPUT_FILE)
           fprintf(stream, "I %s\n", remote_file);

/*--------------------------------------------------------------------*/
/*    if both source & dest are not the same we must copy src_file    */
/*--------------------------------------------------------------------*/

         if ( !equal(src_system, dest_system) )
         {

            sprintf(dest_file, dataf_fmt, 'D', src_system, sequence_s,
                    subseq());

            fprintf(stream, "F %s %s\n", dest_file, remote_file );

/*--------------------------------------------------------------------*/
/*      if source is remote and dest is local copy source to local    */
/*      if source is local and dest is remote copy source to remote   */
/*--------------------------------------------------------------------*/

            if ((s_remote && !d_remote) || (!s_remote && d_remote))
            {
               if (!do_copy(src_system, src_file, dest_system, dest_file))
                   return FALSE;
            } /* if ((s_remote && !d_remote) || (!s_remote && d_remote)) */

/*--------------------------------------------------------------------*/
/*      if both source & dest are on remote nodes we need uuxqt       */
/*--------------------------------------------------------------------*/

            else if (s_remote && d_remote)
            {
               if (!do_uuxqt(job_id,
                             src_system,
                             remote_file,
                             dest_system,
                             dest_file))
                   return FALSE;

               if (!do_copy(src_system, src_file, E_nodename, dest_file))
                   return FALSE;

            } /* else if (s_remote && d_remote) */

            continue;
         } /* if ( !equal(src_system, dest_system) ) */

         printmsg(4, "system \"%s\", rest \"%s\"", src_system, src_file);

      } /* for (; optind < argc; optind++) */

/*--------------------------------------------------------------------*/
/*  Create the data file if any to send to the remote system          */
/*--------------------------------------------------------------------*/

      if (flags[FLG_READ_STDIN]) {
          if (i_remote) {
              printmsg(0, "uux - multiple input files specified");
              return FALSE;
          }

          sprintf(rifile, dataf_fmt, 'D', E_nodename, sequence_s,
                  subseq());
          sprintf(lifile, dataf_fmt, 'D', E_nodename, sequence_s,
                  subseq());

          importpath(msname, lifile, dest_system);
          mkfilename(msfile, E_spooldir, msname);

          if (!CopyData( NULL, msfile )) {
              remove( msfile );
              return FALSE;
          }

          fprintf(stream, "F %s\n", rifile);
          fprintf(stream, "I %s\n", rifile);
      }

/*--------------------------------------------------------------------*/
/*           here finish writing parameters to the X file             */
/*--------------------------------------------------------------------*/

      printmsg(4, "command \"%s\"", command);


      fprintf(stream, "C %s\n", command);
      fclose(stream);

/*--------------------------------------------------------------------*/
/*                     create local C (call) file                     */
/*--------------------------------------------------------------------*/

     if (d_remote) {
          sprintf(tmfile, spool_fmt, 'C', dest_system,  grade, sequence_s);
          importpath( msname, tmfile, dest_system);
          mkfilename( msfile, E_spooldir, msname);

          if ( (stream = FOPEN(msfile, "a",TEXT_MODE)) == NULL) {
             printerr( msname );
             printmsg(0, "uux: cannot write/append to C file %s", msfile);
             return FALSE;
          }

          if (flags[FLG_READ_STDIN])
              fprintf(stream, send_cmd, lifile, rifile, E_mailbox, lifile);

          fprintf(stream, send_cmd, lxfile, rxfile, E_mailbox, lxfile);

          fclose(stream);
     }
     return TRUE;
} /* do_remote */

/*--------------------------------------------------------------------*/
/*    m a i n                                                         */
/*                                                                    */
/*    main program                                                    */
/*--------------------------------------------------------------------*/

void main(int  argc, char  **argv)
{
   int         c;
   extern char *optarg;
   extern int   optind;

/*--------------------------------------------------------------------*/
/*     Report our version number and date/time compiled               */
/*--------------------------------------------------------------------*/

   debuglevel = 0;
   banner( argv );

#if defined(__CORE__)
   copywrong = strdup(copyright);
   checkref(copywrong);
#endif

   if (!configure( B_UUCP ))
      exit(1);   /* system configuration failed */

   user_id = E_mailbox;

/*--------------------------------------------------------------------*/
/*        Process our arguments                                       */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*
 *
 *   -aname    Use name as the user identification replacing the initiator
 *   -b        Return whatever standard input was provided to the uux command
 *   -c        Do not copy local file to the spool directory for transfer to
 *   -C        Force the copy of local files to the spool directory for
 *   -E        run job using exec
 *   -e        run job using sh
 *   -ggrade   Grade is a single letter number; lower ASCII sequence
 *   -j        Output the jobid ASCII string on the standard output which is
 *   -n        Do not notify the user if the command fails.
 *   -p        Same as -:  The standard input to uux is made the standard
 *   -r        Do not start the file transfer, just queue the job.
 *   -sfile    Report status of the transfer in file.
 *   -xdebug_level
 *   -z        Send success notification to the user.
 *
/*--------------------------------------------------------------------*/

   while ((c = getopt(argc, argv, "-a:bcCEejg:nprs:x:z")) !=  EOF)
      switch(c) {
      case '-':
         flags[FLG_READ_STDIN] = TRUE;
         break;
      case 'a':
         flags[FLG_USE_USERID] = TRUE;
         user_id = optarg;
         break;
      case 'b':
         flags[FLG_RETURN_STDIN] = TRUE;
         break;
      case 'c':               // don't spool
         flags[FLG_COPY_SPOOL] = FALSE;
         break;
      case 'C':               // force spool
         flags[FLG_COPY_SPOOL] = TRUE;
         break;
      case 'E':               // use exec to execute
         flags[FLG_USE_EXEC] = TRUE;
         break;
      case 'e':               // use sh to execute
         flags[FLG_USE_EXEC] = FALSE;
         break;
      case 'j':               // output job id to stdout
         flags[FLG_OUTPUT_JOBID] = TRUE;
         break;
      case 'n':               // do not notify user if command fails
         flags[FLG_NONOTIFY_FAIL] = TRUE;
         break;
      case 'p':
         flags[FLG_READ_STDIN] = TRUE;
         break;
      case 'r':               // queue job only
         flags[FLG_QUEUE_ONLY] = TRUE;
         break;
      case 'z':
         flags[FLG_NOTIFY_SUCCESS] = TRUE;
         break;
      case 'g':               // set grade of transfer
         grade = *optarg;
         break;
      case 's':               // report status of transfer to file
         flags[FLG_STATUS_FILE] = TRUE;
         st_out = optarg;
         break;
      case 'x':
         debuglevel = atoi( optarg );
         break;
      case '?':
         usage();
         exit(1);
         break;
      default:
         printmsg(0, "uux - bad argument from getopt \"%c\"", c);
         exit(1);
         break;
   }

   if (argc - optind < 1)     // Verify we have at least a command
   {
      printmsg(0,"uux - no command to execute!");
      usage();
      exit(1);
   }

#if defined(_Windows)
   openlog( NULL );
   atexit( CloseEasyWin );               // Auto-close EasyWin on exit
#endif

   if (!do_remote(optind, argc, argv))
   {
      printmsg(0, "uux command failed");
      exit(1);
   };

   if (flags[FLG_OUTPUT_JOBID])
       printf("%s\n", job_id);

   exit(0);
} /* main */

/*--------------------------------------------------------------------*/
/*    s u b s e q                                                    */
/*                                                                    */
/*    Generate a valid sub-sequence number                            */
/*--------------------------------------------------------------------*/

static char subseq( void )
{
   static char next = '0' - 1;

   switch( next )
   {
      case '9':
         next = 'A';
         break;

      case 'Z':
         next = 'a';
         break;

      default:
         next += 1;
   } /* switch */

   return next;

} /* subseq */
