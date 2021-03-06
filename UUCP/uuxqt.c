/*
      Program:    uuxqt.c              23 September 1991
      Author:     Mitch Mitchell
      Email:      mitch@harlie.lonestar.org

      This is a re-write of the (much cleaner) UUXQT.C originally
      distributed with UUPC/Extended.  The modifications are
      intended primarily to lay a foundation for support for the
      more advanced features of UUX.

      Usage:      uuxqt -xDEBUG -sSYSTEM
*/

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
 *    $Id: uuxqt.c 1.18 1993/10/03 22:10:24 ahd Exp $
 *
 *    Revision history:
 *    $Log: uuxqt.c $
 * Revision 1.18  1993/10/03  22:10:24  ahd
 * Use signed for length of parameters
 *
 * Revision 1.17  1993/10/03  20:43:08  ahd
 * Normalize comments to C++ double slash
 *
 * Revision 1.16  1993/09/29  23:29:56  ahd
 * Add xqtrootdir for UUXQT
 *
 * Revision 1.15  1993/09/20  04:48:25  ahd
 * TCP/IP support from Dave Watt
 * 't' protocol support
 * OS/2 2.x support (BC++ 1.0 for OS/2)
 *
 * Revision 1.14  1993/08/03  03:35:58  ahd
 * Correct path pointer to initialized variable
 *
 * Revision 1.13  1993/08/03  03:11:49  ahd
 * Initialize buffer for shell() in non-Windows environment
 *
 * Revision 1.12  1993/07/31  16:27:49  ahd
 * Changes in support of Robert Denny's Windows support
 *
 * Revision 1.11  1993/07/24  03:40:55  ahd
 * Agressively trap carriage returns at ends of lines (from X.* files
 * being edited by elves with DOS editors!)
 *
 * Revision 1.10  1993/07/20  21:45:37  ahd
 * Don't delete file after -2 abort from UUXQT
 *
 * Revision 1.9  1993/06/26  16:01:48  ahd
 * Normalize white space used to parse strings
 *
 * Revision 1.8  1993/06/13  14:06:00  ahd
 * Correct off-by-one error in RMAIL arg parse loop which crashed UUXQT
 * on long system names
 *
 * Revision 1.7  1993/04/11  00:35:46  ahd
 * Global edits for year, TEXT, etc.
 *
 * Revision 1.6  1993/04/05  04:35:40  ahd
 * Use timestamp/file size information returned by directory search
 *
 * Revision 1.5  1992/11/25  12:59:17  ahd
 * Change NUL to /dev/nul to prevent ImportPath() mangling.
 *
 * Revision 1.4  1992/11/23  03:56:06  ahd
 * Selected fixes for use of generlized uux commands
 *
 * Revision 1.3  1992/11/19  03:03:33  ahd
 * drop rcsid
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#ifdef _Windows
#include <windows.h>
#endif

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "arpadate.h"
#include "dater.h"
#include "expath.h"
#include "getopt.h"
#include "getseq.h"
#include "hlib.h"
#include "hostable.h"
#include "import.h"
#include "lock.h"
#include "logger.h"
#include "pushpop.h"
#include "readnext.h"
#include "security.h"
#include "timestmp.h"
#include "usertabl.h"
#include "execute.h"

#ifdef _Windows
#include "winutil.h"
#endif

currentfile();

/*--------------------------------------------------------------------*/
/*                      Execution flag defines                        */
/*--------------------------------------------------------------------*/

typedef enum {
        X_OUTPUT,     /* 'O' return output to "outnode"       */
        X_FAILED,     /* 'Z' send status if command failed    */
        X_SUCCESS,    /* 'n' send status if command succeeded */
        X_INPUT,      /* 'B' return command input on error    */
        X_USEEXEC,    /* 'E' process command using exec(2)    */
        X_STATFIL,    /* 'M' return status to file on remote  */

        S_CORRUPT,
        S_EMPTY,
        S_NOREAD,
        S_NOWRITE,
        S_STDIN,

        E_NORMAL,
        E_NOACC,
        E_SIGNAL,
        E_STATUS,
        E_NOEXE,
        E_FAILED,

        UU_LAST

        } UU_FLAGS;

/*--------------------------------------------------------------------*/
/*                          Global Variables                          */
/*--------------------------------------------------------------------*/

static char *spool_fmt = SPOOLFMT;
static char *dataf_fmt = DATAFFMT;
static char *send_cmd  = "S %s %s %s -%s %s 0666 %s\n";

/*--------------------------------------------------------------------*/
/*                        Internal prototypes                         */
/*--------------------------------------------------------------------*/

static void usage( void );

static boolean copylocal(const char *from, const char *to);

static boolean do_uuxqt( const char *sysname );

static void process( const char *fname,
                     const char *remote,
                     const char *executeDirectory);

char **create_environment(const char *logname,
                          const char *requestor);

static void delete_environment( char **envp);

static boolean AppendData( const char *input, FILE* dataout);

static boolean do_copy( char *localfile,
                       const char *rmtsystem,
                       const char *remotefile,
                       const char *requestor,
                       const boolean success );

static void ReportResults(const int   status,
                          const char *input,
                                char *output,
                          const char *command,
                          const char *job_id,
                          const time_t jtime,
                          const char *requestor,
                          const char *outnode,
                          const char *outname,
                          const boolean xflag[],
                          const char *statfil,
                          const char *machine,
                          const char *user);

static int shell(char *command,
                 const char *inname,
                 const char *outname,
                 const char *remotename,
                 boolean xflag[]);

static boolean MailStatus(char *tempfile,
                          char *address,
                          char *subject);

/*--------------------------------------------------------------------*/
/*    m a i n                                                         */
/*                                                                    */
/*    Main program                                                    */
/*--------------------------------------------------------------------*/

void main( int argc, char **argv)
{
   int c;
   extern char *optarg;
   extern int   optind;
   char *sysname = "all";

/*--------------------------------------------------------------------*/
/*     Report our version number and date/time compiled               */
/*--------------------------------------------------------------------*/

   debuglevel = 1;
   banner( argv );

#if defined(__CORE__)
   copywrong = strdup(copyright);
   checkref(copywrong);
#endif

/*--------------------------------------------------------------------*/
/*        Process our arguments                                       */
/*--------------------------------------------------------------------*/

   while ((c = getopt(argc, argv, "s:x:")) !=  EOF)
      switch(c) {

      case 's':
         sysname = optarg;
         break;

      case 'x':
         debuglevel = atoi( optarg );
         break;

      case '?':
         usage();
         exit(1);
         break;

      default:
         printmsg(0, "uuxqt - invalid option -%c", c);
         usage();
         exit(2);
         break;
   }

   if (optind != argc) {
      fputs("Extra parameter(s) at end.\n", stderr);
      usage();
      exit(2);
   }

/*--------------------------------------------------------------------*/
/*                             Initialize                             */
/*--------------------------------------------------------------------*/

   tzset();                      // Set up time zone information

   if (!configure( B_UUXQT ))
      exit(1);   /* system configuration failed */

/*--------------------------------------------------------------------*/
/*                  Switch to the spooling directory                  */
/*--------------------------------------------------------------------*/

   PushDir( E_spooldir );
   atexit( PopDir );

/*--------------------------------------------------------------------*/
/*                     Initialize logging file                        */
/*--------------------------------------------------------------------*/

   openlog( NULL );

   checkuser( E_mailbox  );   // Force User Table to initialize
   checkreal( E_mailserv );   // Force Host Table to initialize

   if (!LoadSecurity())
   {
      printmsg(0,"Unable to initialize security, see previous message");
      exit(2);
   } /* if (!LoadSecurity()) */

#if defined(_Windows)
   atexit( CloseEasyWin );               // Auto-close EasyWin on exit
#endif

/*--------------------------------------------------------------------*/
/*                Set up search path for our programs                 */
/*--------------------------------------------------------------------*/

   if ( E_uuxqtpath != NULL )
   {
      char buf[BUFSIZ];
      char *p;
      sprintf(buf,"PATH=%s", E_uuxqtpath);
      p = newstr(buf);

      if (putenv( p ))
      {
         printmsg(0,"Unable to set path \"%s\"", p);
         panic();
      } /* if (putenv( p )) */

   } /* if ( E_uuxqtpath != NULL ) */

/*--------------------------------------------------------------------*/
/*              Disable OS/2 undelete support if desired              */
/*--------------------------------------------------------------------*/

   if ( !bflag[ F_UNDELETE ] )
      putenv( "DELDIR=");

/*--------------------------------------------------------------------*/
/*    Actually invoke the processing routine for the eXecute files    */
/*--------------------------------------------------------------------*/

   do_uuxqt( sysname );
   if( equal( sysname , "all" ) )
       do_uuxqt( E_nodename );

   exit(0);

} /* main */

/*--------------------------------------------------------------------*/
/*    d o _ u u x q t                                                 */
/*                                                                    */
/*    Processing incoming eXecute (X.*) files for a remote system     */
/*--------------------------------------------------------------------*/

static boolean do_uuxqt( const char *sysname )
{
   struct HostTable *hostp;
   static char uu_machine[] = UU_MACHINE "=";
   char hostenv[sizeof uu_machine + 25 + 2];
   char executeDirectory[FILENAME_MAX];
   char *pattern;

/*--------------------------------------------------------------------*/
/*                 Determine if we have a valid host                  */
/*--------------------------------------------------------------------*/

   if( !equal( sysname , "all" ) ) {
      if (equal( sysname , E_nodename ))
          hostp = checkname( sysname );
      else
          hostp = checkreal( sysname );

      if (hostp  ==  BADHOST) {
         printmsg(0, "Unknown host \"%s\".", sysname );
         exit(1);
      }

   } else
        hostp = nexthost( TRUE );

/*--------------------------------------------------------------------*/
/*                Define mask for execution directory                 */
/*--------------------------------------------------------------------*/

   if (( E_xqtRootDir == NULL ) || equali( E_xqtRootDir, E_spooldir ))
      sprintf( executeDirectory, "%s/%%s/XQT", E_spooldir );
                                 // Nice parallel construction
   else
      sprintf( executeDirectory, "%s/XQT/%%s", E_xqtRootDir);
                                 // Fewer directories than if we
                                 // use the spool version

   pattern = newstr( executeDirectory );  // Save pattern for posterity

/*--------------------------------------------------------------------*/
/*             Outer loop for processing different hosts              */
/*--------------------------------------------------------------------*/

   while  (hostp != BADHOST)
   {
      char fname[FILENAME_MAX];
      boolean locked = FALSE;

/*--------------------------------------------------------------------*/
/*                Initialize security for this remote                 */
/*--------------------------------------------------------------------*/

      if ( !equal(sysname, E_nodename) &&
           (securep = GetSecurity( hostp )) == NULL )
         printmsg(0,"No security defined for \"%s\","
                  " cannot process X.* files",
                  hostp->hostname );
      else {

/*--------------------------------------------------------------------*/
/*              Set up environment for the machine name               */
/*--------------------------------------------------------------------*/

         sprintf(hostenv,"%s%.25s", uu_machine, hostp->hostname);

         if (putenv( hostenv ))
         {
            printmsg(0,"Unable to set environment \"%s\"",hostenv);
            panic();
         }

         sprintf(executeDirectory , pattern, hostp->hostname );
         printmsg(5,"Execute directory is %s", executeDirectory );

/*--------------------------------------------------------------------*/
/*           Inner loop for processing files from one host            */
/*--------------------------------------------------------------------*/

         while (readnext(fname, hostp->hostname, "X", NULL, NULL, NULL) )
         {
            if ( locked || LockSystem( hostp->hostname , B_UUXQT ))
            {
               process( fname , hostp->hostname, executeDirectory );
               locked = TRUE;
            }
            else
               break;               // We didn't get the lock

         } /* while */

         if ( locked )
            UnlockSystem();

      } /* else if */

/*--------------------------------------------------------------------*/
/*                        Restore environment                         */
/*--------------------------------------------------------------------*/

      putenv( uu_machine );   // Reset to empty string

/*--------------------------------------------------------------------*/
/*    If processing all hosts, step to the next host in the queue     */
/*--------------------------------------------------------------------*/

      if( equal(sysname,"all") )
         hostp = nexthost( FALSE );
      else
         hostp = BADHOST;

   } /*while nexthost*/

   return FALSE;

} /* do_uuxqt */

/*--------------------------------------------------------------------*/
/*    p r o c e s s                                                   */
/*                                                                    */
/*    Process a single execute file                                   */
/*--------------------------------------------------------------------*/

static void process( const char *fname,
                     const char *remote,
                     const char *executeDirectory)
{
   char *command = NULL,
        *input = NULL,
        *output = NULL,
        *job_id = NULL,
        *token = NULL,
        line[BUFSIZ];
   char hostfile[FILENAME_MAX];
   boolean skip = FALSE;
   boolean reject = FALSE;
   FILE *fxqt;
   int   status;

   char *outnode = NULL;
   char *outname = NULL;
   char *user = NULL;
   char *requestor = NULL;
   char *statfil = NULL;
   char *machine = NULL;
   char **envp;

   boolean xflag[UU_LAST - 1] = { 0 };
   time_t jtime = time(NULL);

/*--------------------------------------------------------------------*/
/*                         Open the X.* file                          */
/*--------------------------------------------------------------------*/

   if ( (fxqt = FOPEN(fname, "r", BINARY_MODE)) == NULL)
   {
      printerr(fname);
      return;
   }
   else
      printmsg(2, "processing %s", fname);

/*--------------------------------------------------------------------*/
/*                  Begin loop to read the X.* file                   */
/*--------------------------------------------------------------------*/

   while (!skip & (fgets(line, BUFSIZ, fxqt) != NULL))
   {
      char *cp;

      if ( (cp = strchr(line, '\n')) != NULL )
         *cp = '\0';

      printmsg(5, "Input read: %s", line);

/*--------------------------------------------------------------------*/
/*            Process the input line according to its type            */
/*--------------------------------------------------------------------*/

      switch (line[0])
      {

      case '#':
         break;

/*--------------------------------------------------------------------*/
/*                  User which submitted the command                  */
/*--------------------------------------------------------------------*/

      case 'U':
         if ( (cp = strtok(line + 1,WHITESPACE)) == NULL )
         {
            printmsg(0,"No user on U line in file \"%s\"", fname );
            reject = TRUE;
            break;
         }
         else {
             user = strdup(cp);
             checkref(user);
         };
                                    // Get the system name
         if ( (cp = strtok(NULL,WHITESPACE)) == NULL)
         {                          // Did we get a string?
            printmsg(2,"No node on U line in file \"%s\"", fname );
            cp = (char *) remote;
         }
         else if (!equal(cp,remote))
         {
            printmsg(2,"Node on U line in file \"%s\" doesn't match remote",
                     fname );
            cp = (char * ) remote;
         };
         machine = newstr(cp);
         break;

/*--------------------------------------------------------------------*/
/*                       Input file for command                       */
/*--------------------------------------------------------------------*/

      case 'I':
         cp = strtok( line + 1, WHITESPACE );
         if ( cp == NULL )
         {
            printmsg(0,"No input file name on I line");
            reject = TRUE;
         }
         else {
            input = strdup( cp );
            checkref(input);
            if (!equal(remote, E_nodename) &&
                !(equaln(input,"D.",2) || ValidateFile( input, ALLOW_READ)))
            {
                reject = TRUE;
                xflag[S_NOREAD] = TRUE;
            }
         } /* else */
         break;

/*--------------------------------------------------------------------*/
/*                      Output file for command                       */
/*--------------------------------------------------------------------*/

      case 'O':
         if ( (cp = strtok(line + 1, WHITESPACE)) != NULL )
         {
             outname = strdup(cp);
             checkref(outname);
             xflag[X_OUTPUT] = TRUE;  // return output to "outnode"

             if ( (cp = strtok(NULL,WHITESPACE)) != NULL)
             {                // Did we get a string?
                   outnode = strdup(cp);
                   checkref(outnode);
                   checkreal(outnode);
             }
             else if (!equal(remote, E_nodename))
             {
                if (!(equaln(outname,"D.",2) || ValidateFile( outname, ALLOW_WRITE)))
                {
                    reject = TRUE;
                    xflag[S_NOWRITE] = TRUE;
                } /* if */
             } /* else if (!equal(remote, E_nodename)) */
         } /* if ( (cp = strtok(NULL,WHITESPACE)) != NULL ) */
         break;

/*--------------------------------------------------------------------*/
/*                         Command to execute                         */
/*--------------------------------------------------------------------*/

      case 'C':
         cp = strtok( line + 2, "\r\n" );
         if ( cp == NULL )
         {
            printmsg(0,"No command name on C line");
            reject = TRUE;
         }
         else {
            command = strdup( cp );
            checkref(command);
         }
         break;

/*--------------------------------------------------------------------*/
/*                      Job Id for status reporting                   */
/*--------------------------------------------------------------------*/

      case 'J':
         if ( (cp = strtok(line + 1, WHITESPACE)) == NULL )
         {
            printmsg(0,"No job id on J line in file \"%s\"", fname );
            reject = TRUE;
         }
         else {
            job_id = strdup( cp );
            checkref( job_id );
         } /* else */
         break;

/*--------------------------------------------------------------------*/
/*                 Check that a required file exists                  */
/*--------------------------------------------------------------------*/

      case 'F':
         token = strtok(line + 1, WHITESPACE);
         importpath(hostfile, token, remote);

         if ( access( hostfile, 0 ))   // Does the host file exist?
         {                             // No --> Skip the file
            printmsg(0,"Missing file %s (%s) for %s, command skipped",
                     token, hostfile, fname);
            skip = TRUE;
         }
         break;

/*--------------------------------------------------------------------*/
/*             Requestor name (overrides user name, above)            */
/*--------------------------------------------------------------------*/

      case 'R':
         if ( (cp = strtok(line + 1,WHITESPACE)) == NULL )
         {
            printmsg(0,"No requestor on R line in file \"%s\"", fname );
            reject = TRUE;
         }
         else {
            requestor = strdup(cp);
            checkref(requestor);
         }
         break;

/*--------------------------------------------------------------------*/
/*        Status file name to return info to on remote node           */
/*--------------------------------------------------------------------*/

      case 'M':
         if ( (cp = strtok(line + 1, WHITESPACE)) == NULL )
            printmsg(0,"No file name on M line in file \"%s\"", fname);
         else {
            statfil = strdup(cp);
            checkref(statfil);
            xflag[X_STATFIL] = TRUE;     // return status to remote file
         }
         break;

/*--------------------------------------------------------------------*/
/*                            Flag fields                             */
/*--------------------------------------------------------------------*/

      case 'Z': xflag[X_FAILED] = TRUE;   // send status if command failed
         break;

      case 'N': xflag[X_FAILED] = FALSE;  // send NO status if command failed
         break;

      case 'n': xflag[X_SUCCESS] = TRUE;  // send status if command succeeded
         break;

      case 'z': xflag[X_SUCCESS] = FALSE; // NO status if command succeeded
         break;

      case 'B': xflag[X_INPUT] = TRUE;    // return command input on error
         break;

      case 'e': xflag[X_USEEXEC] = FALSE; // process command using sh(1)
         break;

      case 'E': xflag[X_USEEXEC] = TRUE;  // process command using exec(2)
         break;

/*--------------------------------------------------------------------*/
/*                    Quietly ignore unknown fields                   */
/*--------------------------------------------------------------------*/

      default :
         break;

      } /* switch */
   } /* while (!skip & (fgets(line, BUFSIZ, fxqt) != NULL)) */

   if ( fxqt != NULL )
      fclose(fxqt);


   if ((command == NULL) && !skip)
   {
      printmsg(0,"No command supplied for X.* file %s, skipped", fname);
      reject = TRUE;
   }

/*--------------------------------------------------------------------*/
/*           We have the data for this command; process it            */
/*--------------------------------------------------------------------*/

   if ( ! (skip || reject ))
   {
      if ( user == NULL )
      {
         user = strdup("uucp");    // User if none given
         checkref(user);
      }

      if (requestor == NULL)
      {
         requestor = strdup(user);
         checkref(requestor);
      }

      if (input == NULL)
         input = strdup("/dev/nul");

      if (output == NULL)
         output = mktempname(NULL, "OUT");

      printmsg(equaln(command,RMAIL,5) ? 2 : 0,
               "uuxqt: executing \"%s\" for user \"%s\" at  \"%s\"",
                   command, user, machine);

/*--------------------------------------------------------------------*/
/*             Create the environment and run the command             */
/*--------------------------------------------------------------------*/

      envp = create_environment("uucp", requestor);
      status = shell(command, input, output, remote, xflag );
      delete_environment(envp);

      ReportResults( status, input, output, command, job_id,
                     jtime, requestor, outnode, outname, xflag,
                     statfil, machine, user);

/*--------------------------------------------------------------------*/
/*                  Clean up files after the command                  */
/*--------------------------------------------------------------------*/

      if ( status > -2 )
      {
         unlink(fname);       // Already a local file name

         if (equaln(input,"D.",2))
         {
             importpath(hostfile, input, remote);
             unlink(hostfile);
         }

         if (xflag[X_OUTPUT])
         {
             importpath(hostfile, output, remote);
             unlink(hostfile);
         }

      } /* if ( status > -2 ) */

   }
   else if (reject && !skip)
        unlink(fname);       // Already a local file name

/*--------------------------------------------------------------------*/
/*              Free various temporary character strings              */
/*--------------------------------------------------------------------*/

   if (command    != NULL) free(command);
   if (input      != NULL) free(input);
   if (job_id     != NULL) free(job_id);
   if (outnode    != NULL) free(outnode);
   if (output     != NULL) free(output);
   if (requestor  != NULL) free(requestor);
   if (statfil    != NULL) free(statfil);
   if (user       != NULL) free(user);

} /* process */

/*--------------------------------------------------------------------*/
/*    s h e l l                                                       */
/*                                                                    */
/*    Simulate a Unix command                                         */
/*--------------------------------------------------------------------*/

static int shell(char *command,
                 const char *inname,
                 const char *outname,
                 const char *remotename,
                 boolean xflag[])
{
   int    result = 0;
   char   inlocal[FILENAME_MAX];
   char   outlocal[FILENAME_MAX];
   char   buf[255];

   char   *cmdname;
   char   *parameters;

   if (xflag[X_USEEXEC])
      printmsg(2, "exec(2) not supported, executing using spawn");

/*--------------------------------------------------------------------*/
/*         Determine the command name and parameters, if any          */
/*--------------------------------------------------------------------*/

   cmdname = strtok( command, WHITESPACE );
   parameters = strtok( NULL, "\r\n" );

   if ( parameters != NULL )
   {
      while (isspace( *parameters ) || iscntrl( *parameters ))
         parameters++;

      if ( !strlen( parameters ))
         parameters = NULL;

   } /* if ( parameters != NULL ) */

/*--------------------------------------------------------------------*/
/*    Verify we support the command, and get it's real name, if so    */
/*--------------------------------------------------------------------*/

   if ( (!equal(remotename, E_nodename)) && (!ValidateCommand( cmdname )) )
   {
      printmsg(0,"Command \"%s\" not allowed at this site", cmdname);
      xflag[E_NOEXE] = TRUE;
      return 99;
   }

/*--------------------------------------------------------------------*/
/*                     Open files for processing                      */
/*--------------------------------------------------------------------*/

   if (inname != NULL)
      importpath(inlocal, inname, remotename);

   if ( outname != NULL )
      importpath(outlocal, outname, remotename);

/*--------------------------------------------------------------------*/
/*               We support the command; execute it                   */
/*--------------------------------------------------------------------*/

   fflush(logfile);

/*--------------------------------------------------------------------*/
/*               RNEWS may be special, handle it if so                */
/*--------------------------------------------------------------------*/

   if (equal(cmdname,RNEWS) &&
       bflag[F_WINDOWS] &&
       ( inname != NULL ))       // rnews w/input?
   {
      strcpy( buf, "-f " );
      strcat( buf, inlocal );
      parameters = buf;          // We explicitly ignore all parameters
                                 // on the RNEWS command

      result = execute( RNEWS,
                        buf,
                        NULL,
                        outname == NULL ? NULL : outlocal,
                        TRUE,
                        FALSE );
   }

/*--------------------------------------------------------------------*/
/*        RMAIL is special, we need to break up the parameters        */
/*--------------------------------------------------------------------*/

   else if (equal(cmdname,RMAIL) && ( inname != NULL )) // rmail w/input?
   {
      parameters = strtok( parameters, WHITESPACE );

      while (( parameters != NULL ) && (result != -1 ))
      {

         boolean firstPass = TRUE;
         int left = 0;

#if defined(__OS2__) || defined(WIN32)
         int rlen =  254 ;
#elif defined(__TURBOC__)
         int rlen =  126 ;
#else
         int rlen = (_osmode == DOS_MODE) ? 126 :  254;
#endif

#ifdef _Windows
         if ( bflag[F_WINDOWS] )
         {
            strcpy( buf, "-f ");
            strcat( buf, inlocal);
            strcat( buf, " ");
         }
         else
            *buf = '\0';
#else
         *buf = '\0';
#endif
         rlen -= strlen( buf ) + strlen( RMAIL ) + 1;

/*--------------------------------------------------------------------*/
/*                   Copy addresses into the buffer                   */
/*--------------------------------------------------------------------*/

         left = rlen - strlen( parameters );

         while ((parameters != NULL) && (left > 0))
         {
            char *next = strtok( NULL, "");

            if ( *parameters == '-')   // Option flag for mail?
               printmsg(0,"Disallowed option %s ignored",parameters);
            else {                     // Not option, add to param list
               strcat( buf, " ");
               strcat( buf, parameters );
               rlen -= strlen( parameters ) + 1;
               firstPass = FALSE;
            }

/*--------------------------------------------------------------------*/
/*                       Step to next parameter                       */
/*--------------------------------------------------------------------*/

            if ( next == NULL )
               parameters = NULL;
            else
               parameters = strtok( next, WHITESPACE );

         } /* while ( parameters != NULL ) */

         if (firstPass)       // Did we process at least one addr?
         {                    // No --> Serious problem!
            printmsg(0,
                     "Address \"%s\" too long (%d chars)!  %d available, short fall would be %d",
                      parameters,
                      strlen(parameters),
                      rlen,
                      left );

            panic();
         } /* if (*buf = '\0') */

      } /* while */

/*--------------------------------------------------------------------*/
/*               Execute one command line of addresses                */
/*--------------------------------------------------------------------*/

      result = execute( RMAIL,
                        buf,
                        bflag[F_WINDOWS] ? NULL : inlocal,
                        outname == NULL ? NULL : outlocal,
                        TRUE,
                        FALSE );

      if ( result != 0 )    // Did command execution fail?
      {
         printmsg(0,"shell: command \"%s %s\" returned error code %d",
               cmdname, buf, result);
         panic();
      }

   } /* if (equal(cmdname,RMAIL) && ( inname != NULL )) */
   else
      result = execute( command,
                        parameters,
                        inlocal,
                        outlocal,
                        TRUE,
                        FALSE );

/*--------------------------------------------------------------------*/
/*                    Determine result of command                     */
/*--------------------------------------------------------------------*/

   if ( result == 0 )
      xflag[E_NORMAL] = TRUE;
   else if ( result > 0 )
      xflag[E_STATUS] = TRUE;

   fflush(logfile);

   return result;

} /* shell */

/*--------------------------------------------------------------------*/
/*    u s a g e                                                       */
/*                                                                    */
/*    Report how to run this program                                  */
/*--------------------------------------------------------------------*/

static void usage( void )
{
   fputs("Usage:\tuuxqt\t[-xDEBUG] [-sSYSTEM]", stderr);
   exit(1);
} /* usage */

/*--------------------------------------------------------------------*/
/*    c o p y l o c a l                                               */
/*                                                                    */
/*    Copy Local Files                                                */
/*--------------------------------------------------------------------*/

static boolean copylocal(const char *from, const char *to)
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
} /* copylocal */

/*--------------------------------------------------------------------*/
/*    c r e a t e _ e n v i r o n m e n t                             */
/*                                                                    */
/*    Create the environment array for subprocesses                   */
/*--------------------------------------------------------------------*/

char **create_environment(const char *logname,
                          const char *requestor)
{
   char buffer[MAXADDR + 20];
   int subscript = 0;
   char **envp = (char **) malloc(sizeof(char *) * 3);

   checkref(envp);

/*--------------------------------------------------------------------*/
/*              "Current" user id processing the request              */
/*--------------------------------------------------------------------*/

   if ( logname != NULL )
   {
     sprintf(buffer,"%s=%s", LOGNAME, logname);
     envp[subscript] = strdup(buffer);
     checkref(envp[subscript++]);
   }

/*--------------------------------------------------------------------*/
/*               user id/nodename of original requestor               */
/*--------------------------------------------------------------------*/

   if ( requestor != NULL )
   {
      sprintf(buffer,"%s=%s",UU_USER, requestor);
      envp[subscript] =  strdup(buffer);
      checkref(envp[subscript++]);
   }

   envp[subscript] =  NULL;   // Terminate the list

/*--------------------------------------------------------------------*/
/*               Now put the data into our environment                */
/*--------------------------------------------------------------------*/

   while( subscript-- > 0)
   {
      if (putenv( envp[subscript] ))
      {
         printmsg(0,"Unable to set environment \"%s\"",envp[subscript]);
         panic();
      }
   } /* while */

   return envp;
} /* create_environment */

/*--------------------------------------------------------------------*/
/*    d e l e t e  _ e n v i r o n m e n t                            */
/*                                                                    */
/*    Delete variables inserted by create_enviroment                  */
/*                                                                    */
/*    Our environment goes away when we are done executing we; just   */
/*    clean up the environment because we are freeing the storage     */
/*--------------------------------------------------------------------*/

static void delete_environment( char **envp )
{
   int subscript = 0;

   while ( envp[subscript] != NULL )
   {
      char *equal = strchr(envp[subscript]  , '=' );
      *++equal = '\0';        // Terminate the string
      if (putenv( envp[subscript] ))
      {
         printmsg(0,"Unable to reset environment \"%s\"",envp[subscript]);
         panic();
      }
      free( envp[subscript++] );
   }

   free( envp );
} /* delete_environment */


/*--------------------------------------------------------------------*/
/*    d o _ c o p y                                                   */
/*                                                                    */
/*    Send a file to remote node via uucp                             */
/*--------------------------------------------------------------------*/

static boolean do_copy(char *localfile,
                       const char *rmtsystem,
                       const char *remotefile,
                       const char *requestor,
                       const boolean success )
{
      if (rmtsystem == NULL) {
          copylocal(localfile, remotefile);
      } else {
          char    tmfile[FILENAME_MAX];  // Unix style name for c file
          char    idfile[FILENAME_MAX];  // Unix style name for data file copy
          char    work[FILENAME_MAX]; // temp area for filename hacking
          char    icfilename[FILENAME_MAX];  // our hacked c file path
          char    idfilename[FILENAME_MAX];  // our hacked d file path

          struct  stat    statbuf;

          long    int     sequence;
          static  char    subseq = 'A';
          char   *sequence_s;
          FILE   *cfile;


          sequence = getseq();
          sequence_s = JobNumber( sequence );

          sprintf(tmfile, spool_fmt, 'C', rmtsystem, 'Z', sequence_s);
          importpath(work, tmfile, rmtsystem);
          mkfilename(icfilename, E_spooldir, work);

          if (stat((char *) localfile, &statbuf) != 0)  {
              printerr( localfile );
              return FALSE;
          }

          sprintf(idfile , dataf_fmt, 'D', E_nodename, sequence_s,
                  (char) subseq++ );
          importpath(work, idfile, rmtsystem);
          mkfilename(idfilename, E_spooldir, work);

          if (!copylocal(localfile, idfilename))  {
             printmsg(0, "Copy \"%s\" to \"%s\" failed", localfile, idfilename);
             return FALSE;
          }

          if ((cfile = FOPEN(icfilename, "a",TEXT_MODE)) == NULL)  {
             printerr( icfilename );
             printf("cannot append to %s\n", icfilename);
             return FALSE;
          }

          fprintf(cfile, send_cmd, localfile, remotefile,
                   "uucp" , success ? "n" : " ", idfile,
                    success ? requestor : " ");

          fclose(cfile);
    };

    return TRUE;
} /* do_copy */

/*--------------------------------------------------------------------*/
/*    R e p o r t R e s u l t s                                       */
/*                                                                    */
/*    report results of command execution as specified by flags in    */
/*    X.* file.                                                       */
/*--------------------------------------------------------------------*/

static void ReportResults(const int status,
                          const char *input,
                                char *output,
                          const char *command,
                          const char *job_id,
                          const time_t jtime,
                          const char *requestor,
                          const char *outnode,
                          const char *outname,
                          const boolean xflag[],
                          const char *statfil,
                          const char *machine,
                          const char *user)
{
     char address[MAXADDR];
     char subject[80];
     FILE *mailtmp = NULL;
     char *tempmail;


     if (!(xflag[X_FAILED] | xflag[X_SUCCESS] |
           xflag[X_INPUT]  | xflag[X_STATFIL]))
     {  /* default actions */
         unlink(output);
         return;
     }

     tempmail = mktempname(NULL, "TMP");

     if ((mailtmp = FOPEN(tempmail, "w+", BINARY_MODE)) == NULL) {
         printerr(tempmail);
         return;
     }

     sprintf(subject, "\"[uucp job %s (%s)]\"", job_id, dater(jtime, NULL) );

     fprintf(mailtmp,"remote execution\n");
     fprintf(mailtmp,"%s\n", command);

#ifdef BETA_TEST
     strcpy(address,"postmaster");
#else
     if (equal(machine, E_nodename))
        strcpy(address, requestor);
     else
        sprintf(address,"%s!%s", machine, requestor);
#endif

     if (xflag[E_NORMAL])
     {                        // command succeded, process appropriate flags

       fprintf(mailtmp,"exited normally\n");

       if (xflag[X_OUTPUT])
           do_copy(output, outnode, outname, requestor, xflag[X_SUCCESS]);
       else
           unlink(output);

       fclose(mailtmp);

       if (xflag[X_SUCCESS]) {
          if (xflag[X_STATFIL]) {
              do_copy(tempmail, outnode, statfil, requestor, xflag[X_SUCCESS]);
          } else {
              MailStatus(tempmail, address, subject);
          }
       };

   } else {            /* command failed, process appropriate flags   */
     if (xflag[E_NOACC])
         fprintf(mailtmp,"file access denied to %s!%s", machine, user);
     else if (xflag[E_NOEXE])
        fprintf(mailtmp,"execution permission denied to %s!%s\n",
                machine, requestor);
     else if (xflag[E_SIGNAL])
        fprintf(mailtmp,"terminated by signal\n");
     else if (xflag[E_STATUS])
        fprintf(mailtmp,"exited with status %d\n", status);
     else /* xflag->e & E_FAILED */
        fprintf(mailtmp,"failed completely\n");


     if (xflag[E_STATUS]) {
       if ((xflag[X_FAILED]) && !(xflag[X_INPUT])) {
           fprintf(mailtmp,"===== error output not available =====\n");
       } else if ((xflag[X_FAILED]) && (xflag[X_INPUT])) {
           fprintf(mailtmp,"===== stdin was ");

           if (xflag[S_CORRUPT])
               fprintf(mailtmp,"unreadable =====\n");
           else if (xflag[S_EMPTY])
               fprintf(mailtmp,"empty =====\n");
           else if (xflag[S_NOREAD])
               fprintf(mailtmp,"denied read permission =====\n");
           else {
               fprintf(mailtmp,"=====\n");
               AppendData( input, mailtmp);
           };
           unlink(input);

           fprintf(mailtmp,"===== stderr is unavailable =====\n");
       }
     }

     fclose(mailtmp);

     if (xflag[X_STATFIL]) {
         do_copy(tempmail, outnode, statfil, requestor, xflag[X_SUCCESS]);
     } else {
         MailStatus(tempmail, address, subject);
     }

   }

   if (xflag[X_OUTPUT])
       unlink(output);

   unlink(tempmail);
   return;
} /* ReportResults */

/*--------------------------------------------------------------------*/
/* A p p e n d D a t a                                                */
/*                                                                    */
/* Append data to output file                                         */
/*--------------------------------------------------------------------*/

static boolean AppendData( const char *input, FILE* dataout)
{
   FILE    *datain;
   char     buf[BUFSIZ];
   boolean  status = TRUE;

/*--------------------------------------------------------------------*/
/*                      Verify the input opened                       */
/*--------------------------------------------------------------------*/

   if (input == NULL)
      return FALSE;
   else
      datain = FOPEN(input, "r",TEXT_MODE);

   if (datain == NULL) {
      printerr(input);
      printmsg(0,"Unable to open input file \"%s\"", input);
      return FALSE;
   } /* datain */

/*--------------------------------------------------------------------*/
/*                       Loop to copy the data                        */
/*--------------------------------------------------------------------*/

   while (fgets(buf, BUFSIZ, datain) != 0)
   {
      if (fputs(buf, dataout) == EOF)     // I/O error?
      {
         printmsg(0,"AppendData: I/O error on output file");
         printerr("dataout");
         fclose(datain);
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

   fclose(datain);
   return status;

} /* AppendData */

/*--------------------------------------------------------------------*/
/*    M a i l S t a t u s                                             */
/*                                                                    */
/*    Send text in a mailbag file to address(es) specified by line.   */
/*--------------------------------------------------------------------*/

static boolean MailStatus(char *tempfile,
                          char *address,
                          char *subject)
{
   boolean status;
   char **envp;
   char buf[BUFSIZ];

/*--------------------------------------------------------------------*/
/*                            Invoke RMAIL                            */
/*--------------------------------------------------------------------*/

   envp = create_environment( "uucp", NULL );

   strcpy(buf, "-w -f " );
   strcat(buf, tempfile );
   if ( subject != NULL )
   {
      strcat(buf, " -s " );
      strcat(buf, subject );
   }
   strcat( buf, " " );
   strcat( buf, address );

   status = execute( RMAIL, buf, NULL, NULL, TRUE, FALSE );

   delete_environment( envp );

/*--------------------------------------------------------------------*/
/*                       Report errors, if any                        */
/*--------------------------------------------------------------------*/

   if ( status < 0 )
   {
      printerr( RMAIL );
      printmsg(0,"Unable to execute rmail; status not delivered.");
   }
   else if ( status > 0 )
      printmsg(0, "Rmail returned error;\
 status delivery may be incomplete.");

/*--------------------------------------------------------------------*/
/*                          Return to caller                          */
/*--------------------------------------------------------------------*/

   return (status == 0 );

} /*MailStatus*/
