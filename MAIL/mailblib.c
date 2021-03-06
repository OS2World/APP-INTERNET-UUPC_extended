/*--------------------------------------------------------------------*/
/*       m a i l b l i b . c                                          */
/*                                                                    */
/*       Support routines for UUPC/extended mail user agent           */
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
 *    $Id: mailblib.c 1.4 1993/09/20 04:41:54 ahd Exp $
 *
 *    Revision history:
 *    $Log: mailblib.c $
 * Revision 1.4  1993/09/20  04:41:54  ahd
 * OS/2 2.x support
 *
 * Revision 1.3  1993/07/31  16:26:01  ahd
 * Changes in support of Robert Denny's Windows support
 *
 */

/*--------------------------------------------------------------------*/
/*                        System include files                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(_Windows)
#include <windows.h>
#endif

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "address.h"
#include "mail.h"
#include "maillib.h"
#include "mailblib.h"
#include "mailsend.h"
#include "hlib.h"
#include "alias.h"
#include "expath.h"

/*--------------------------------------------------------------------*/
/*                      Variables global to file                      */
/*--------------------------------------------------------------------*/

static int *item_list = NULL;
static size_t next_item;

currentfile();

/*--------------------------------------------------------------------*/
/*                    Internal function prototypes                    */
/*--------------------------------------------------------------------*/

static boolean SearchUser( char *token , char **input, const int bits);

static boolean SearchSubject( char *token,
                              char **input,
                              char *trailing,
                              const int bits);

/*--------------------------------------------------------------------*/
/*                     Externally known functions                     */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*    S h o w A l i a s                                               */
/*                                                                    */
/*    Print the expansion of an alias                                 */
/*--------------------------------------------------------------------*/

void ShowAlias( const char *alias)
{
   char *fullname = AliasByNick( alias );
   static int level = 0;
   int column = level * 2;

   if ( alias == NULL )
   {
      printf("Missing operand\n");
      return;
   }

/*--------------------------------------------------------------------*/
/*                        Indent nested calls                         */
/*--------------------------------------------------------------------*/

   while(column-- > 0 )
      putchar(' ');

/*--------------------------------------------------------------------*/
/*       Show the alias, breaking it down recursively if need be      */
/*--------------------------------------------------------------------*/

   if (fullname == NULL)
   {
      char user[MAXADDR];
      char path[MAXADDR];
      char node[MAXADDR];

      printf("No alias defined for \"%s\"\n",alias);

      column = level * 2 + 2;
      while(column-- > 0 )
         putchar(' ');

      user_at_node(alias, path, node, user);
                              /* Reduce address to basics */
      printf("(%s@%s via %s)\n", user, node, path);
   }
   else {

      printf("%s is aliased to %s\n", alias, fullname);

      if (*fullname == '"')
      {

         if ( debuglevel > 1)
         {
            char user[MAXADDR];
            char path[MAXADDR];
            char node[MAXADDR];

            ExtractAddress(user,fullname, FALSE);
            user_at_node(user,path,node,user);
                                    /* Reduce address to basics */
            column = level * 2 + 2;
            while(column-- > 0 )
               putchar(' ');

            printf("(%s@%s via %s)\n", user, node, path);
         }
      }
      else {
         char buf[LSIZE];

         strcpy( buf, fullname );
         fullname = strtok( buf , WHITESPACE "," );

         while (fullname != NULL )
         {
            char *save = strtok( NULL , "");
            level++;             /* Bump up a level for recursive calls */
            ShowAlias(fullname);
            level--;             /* Restore indent level                */
            fullname = strtok( save , " ," );
         } /* while */
      } /* else */
   } /* else */

} /* ShowAlias */

/*--------------------------------------------------------------------*/
/*    S a v e I t e m                                                 */
/*                                                                    */
/*    Save an item in another mailbox                                 */
/*--------------------------------------------------------------------*/

boolean SaveItem( const int item,
               const boolean delete,
               const copyopt headers,
               char *fname,
               const ACTION verb)
{
   char filename[FILENAME_MAX];
   char *s = "?";
   FILE *stream;

   if (fname == NULL)
      fname = "~/mbox";

/*--------------------------------------------------------------------*/
/*                        Build the file name                         */
/*--------------------------------------------------------------------*/

   switch (*fname)
   {
      case '=':         /* relative to home directory? */
            printf("Syntax is obsolete ... use \"~/%s\"", fname + 1 );
            mkfilename(filename, E_homedir, ++fname);
            break;

      case '+':         /* Relative to mail directory?   */
            mkmailbox(filename, ++fname);
            break;

      default:
      case '~':         /* Relative to home directory?            */
            strcpy( filename , fname );
            if (expand_path( filename , NULL, E_homedir, E_mailext ) == NULL)
               return FALSE;
            break;
   }  /* end switch */

/*--------------------------------------------------------------------*/
/*               Announce our action based on the verb                */
/*--------------------------------------------------------------------*/

   switch( verb )
   {
      case M_COPY:
         s = "Copying";
         break;

      case M_SAVE:
         s = "Saving";
         break;

      case M_WRITE:
         s = "Writing";
         break;
   } /* switch */

   printf("%s item %d to %s\n", s , item + 1, filename );

/*--------------------------------------------------------------------*/
/*                     Open the mailox to save to                     */
/*--------------------------------------------------------------------*/

   if ((stream = FOPEN(filename, "a",TEXT_MODE)) == nil(FILE))
   {
      printf("Cannot append to %s\n", filename);
      return FALSE;
   } /* if */

   CopyMsg(item, stream, headers, FALSE);
   fclose(stream);

/*--------------------------------------------------------------------*/
/*                   Delete the message if required                   */
/*--------------------------------------------------------------------*/

   if (letters[item].status < M_DELETED)
      letters[item].status = delete ? M_DELETED : M_SAVED;

   return TRUE;
} /* SaveItem */


/*--------------------------------------------------------------------*/
/*    P o s i t i o n                                                 */
/*                                                                    */
/*    Makes reasonable choice of next letter to examine               */
/*--------------------------------------------------------------------*/

int Position(int absolute, int relative, int start)
{
   int current = start;

/*--------------------------------------------------------------------*/
/*                   Explicitly position the letter                   */
/*--------------------------------------------------------------------*/

   if ( absolute )
   {
      current = absolute ;
      if (( current <= letternum ) && (current > 0))
         return current - 1;
      else if ( current >= letternum )
         printf("Item %d does not exist, last item in mailbox is %d\n",
               current , letternum);
      else
         printf("Cannot backup beyond top of mailbox\n");

      return start;
   } /* if */

/*--------------------------------------------------------------------*/
/*           Position the pionter relative to current item            */
/*--------------------------------------------------------------------*/

   if ( relative )
   {
      int move ;
      move = (relative > 0) ? 1 : -1;
      if ( (current + move) == letternum )
      {
         printf("At end of mailbox\n");
         return current;
      }

      while ( relative )
      {
         current += move;
         if ( current >= letternum )
         {
            printf("Item %d does not exist, last item in mailbox is %d\n",
                  current+relative, letternum);
            return start;
         }
         else if ( current < 0 )
         {
            printf("Cannot backup beyond top of mailbox\n");
            return start;
         }
         else if (letters[current].status != M_DELETED)
            relative -= move;
      } /* while */

      return current;
   } /* if */

/*--------------------------------------------------------------------*/
/*                   Implicitly position the letter                   */
/*--------------------------------------------------------------------*/

   while (current < letternum)
   {
      if (letters[current].status != M_DELETED)
         return current;
      else
         ++current;
   } /* while */

   current = start;

   while (current-- > 0)
      if (letters[current].status != M_DELETED)
         return current;

   printf("At end of mailbox\n");
   return start;
} /* Position */

/*--------------------------------------------------------------------*/
/*    D e l i v e r M a i l                                           */
/*                                                                    */
/*    Compose interactive outgoing mail                               */
/*--------------------------------------------------------------------*/

boolean DeliverMail( char *addresses , int item)
{
   char *Largv[MAXADDRS];
   int   Largc;

   Largc = getargs(addresses , Largv );
   return Collect_Mail(stdin, Largc , Largv, item , FALSE);
} /* DeliverMail */

/*--------------------------------------------------------------------*/
/*    R e p l y                                                       */
/*                                                                    */
/*    Reply to incoming mail                                          */
/*--------------------------------------------------------------------*/

boolean Reply( const int current )
{
   char *Largv[MAXADDRS];
   char subject[LSIZE];
   char addr[LSIZE];
   char line[LSIZE];
   char *column;
   int   Largc = 0;

   subject[0] = '\0';

/*--------------------------------------------------------------------*/
/*               Determine if we can reply to the mail                */
/*--------------------------------------------------------------------*/

   if (!RetrieveLine(letters[current].replyto, addr, LSIZE))
   {
      printf("Cannot determine return address\n");
      return FALSE;
   }

/*--------------------------------------------------------------------*/
/*                  Get the incoming subject, if any                  */
/*--------------------------------------------------------------------*/

   if (RetrieveLine(letters[current].subject, line, LSIZE))
   {
      register char  *sp = line;

      while (!isspace(*sp))     /* Skip "Subject:"      */
         sp++;
      while (isspace(*sp))      /* Skip leading whitespace */
         sp++;
      Largv[Largc++] = "-s";

      if (!equalni(sp,"Re:",3))
         strcat(subject,"Re: ");
      strcat(subject,sp);
      Largv[Largc++] = subject;
   }

/*--------------------------------------------------------------------*/
/*                   Get the extract return address                   */
/*--------------------------------------------------------------------*/

   column = addr;
   while (!isspace(*column) && strlen(column))
      column++;

   BuildAddress(line,column);      /* Build standard "To:" addr  */
   printf("Replying to %s\n", line);

/*--------------------------------------------------------------------*/
/*                    Format the outgoing address                     */
/*--------------------------------------------------------------------*/

   Largv[Largc++] = line;

   if (letters[current].status < M_ANSWERED)
      letters[current].status = M_ANSWERED;

   return Collect_Mail(stdin, Largc, Largv, current, TRUE);

} /* Reply */


/*--------------------------------------------------------------------*/
/*    F o r w a r d I t e m                                           */
/*                                                                    */
/*    Forward (resend) mail to another address                        */
/*--------------------------------------------------------------------*/

boolean ForwardItem( const int item , const char *string )
{
   FILE *stream;
   char  *Largv[MAXADDRS];
   char buf[LSIZE];
   char tmailbag[FILENAME_MAX];
   int   Largc;
   boolean success;

/*--------------------------------------------------------------------*/
/*              copy current message to a temporary file              */
/*--------------------------------------------------------------------*/

   mktempname(tmailbag, "TMP");
   stream = FOPEN(tmailbag, "w",TEXT_MODE);
   if (stream == NULL )
   {
      printerr(tmailbag);
      return FALSE;
   } /* if */

   CopyMsg(item, stream, noreceived,FALSE);

   fclose(stream);

/*--------------------------------------------------------------------*/
/*               mail the content of the temporary file               */
/*--------------------------------------------------------------------*/

   stream = FOPEN(tmailbag, "r",TEXT_MODE);
   if (stream == NULL )
   {
      printerr(tmailbag);
      panic();
   }

   strcpy( buf , string );
   Largc = getargs( buf , Largv );
   success = Send_Mail(stream, Largc , Largv, NULL, TRUE);

/*--------------------------------------------------------------------*/
/*                   Clean up and return to caller                    */
/*--------------------------------------------------------------------*/

   if (letters[item].status < (int) M_FORWARDED)
      letters[item].status = (int) M_FORWARDED;
   remove(tmailbag);

   return success;
} /* ForwardItem */


/*--------------------------------------------------------------------*/
/*    s u b s h e l l                                                 */
/*                                                                    */
/*    Invoke inferior command processor                               */
/*--------------------------------------------------------------------*/

void subshell( char *command )
{
#if defined(_Windows)
   char buf[128];
   //
   // Here we simply use the Windows DOSPRMPT.PIF and fire off
   // an ASYNCHRONOUS DOS box. Under 286 mode, this will be
   // synchronous. But who in the hell cares!
   //
   sprintf(buf, "dosprmpt.pif %s", command);
   WinExec(buf, SW_SHOWMAXIMIZED);
#else
   if ( command == NULL )
   {
      static char *new_prompt = NULL;

      char *exit_prompt =  "PROMPT=Enter EXIT to return to MAIL$_";
      char *old_prompt;

      if ( new_prompt == NULL )
      {
         old_prompt = getenv( "PROMPT" );
         if ( old_prompt == NULL )
            old_prompt = "$p$g";

         new_prompt = malloc( strlen( old_prompt ) + strlen( exit_prompt ) + 1);
         checkref( new_prompt );

         strcpy( new_prompt , exit_prompt );
         strcat( new_prompt, old_prompt );

         if (putenv( new_prompt ) )
         {
            printmsg(0,"Prompt update failed ...");
            printerr("putenv");
         } /* if (putenv( new_prompt ) ) */

      } /* if ( new_prompt == NULL ) */

      system( getenv( "COMSPEC" ) );
   } /* if */
   else
      system ( command );
#endif
} /* subshell */

/*--------------------------------------------------------------------*/
/*    S e l e c t I t e m s                                           */
/*                                                                    */
/*    Select mail items to be processed by the current command        */
/*--------------------------------------------------------------------*/

boolean SelectItems( char **input, int current , int bits)
{
   char *next_token = *input;
   char *token = NULL;
   char trailing[LSIZE];      /* for saving trailing part of line    */
   int item;
   boolean hit = FALSE;

/*--------------------------------------------------------------------*/
/*                 Reset all mail items to unselected                 */
/*--------------------------------------------------------------------*/

   next_item = 0;

/*--------------------------------------------------------------------*/
/*            If no operands, return the current mail item            */
/*--------------------------------------------------------------------*/

   if ( *input == NULL )
   {
      SetItem( current+1 );
      return SetTrailing( input , bits );
   }

/*--------------------------------------------------------------------*/
/*             Select all items if the user requested so              */
/*--------------------------------------------------------------------*/

   strcpy( trailing , next_token );
   token = strtok( next_token , WHITESPACE );
   if (equal(token,"*"))      /* Select all items?                   */
   {
      *input = strtok( NULL , "" );

      for ( item = 1; item <= letternum; item++)
         SetItem( item );

      return SetTrailing( input , bits );
   } /* if */

/*--------------------------------------------------------------------*/
/*   If the first token begins with a slash (/), scan for items       */
/*   with the subject.                                                */
/*--------------------------------------------------------------------*/

   if ( *token == '/' )
      return SearchSubject( token, input, trailing, bits);

/*--------------------------------------------------------------------*/
/*          Scan the line until we hit a non-numeric operand          */
/*--------------------------------------------------------------------*/

   while ( token != NULL)
   {
      boolean success = TRUE;
      next_token = strtok( NULL , "");
                              /* Remember next of line for next pass */

      if (Numeric( token ))
         hit = success = SetItem( atoi(token) );
      else if (equal( token, "$"))
         hit = success = SetItem( letternum );
      else if (equal( token, "."))
         hit = success = SetItem( current + 1 );
      else if (strpbrk(token,"@!") != NULL ) /* User id?             */
         break;                  /* Yes --> Exit loop gracefully     */
      else if (isdigit(*token) || (*token == '$') || (*token == '.'))
                                 /* Is it start-end combination?     */
      {                          /* Yes --> Handle it                */
         char *start, *end ;
         int  istart, iend;
         start = strtok( token , "-");
         end   = strtok( NULL , "");

         if (equal(start,"$"))
            istart = letternum;
         else if (equal(start,"."))
            istart = current + 1 ;
         else if (!Numeric( start ))
         {
            printf("%s: Operand is not numeric\n", start );
            return FALSE;
         } /* if */
         else
            istart = atoi( start );

         if ( (end == NULL) )
         {
            printf("Missing end of item range\n" );
            return FALSE;
         } /* if */

         if (equal(end,"$"))
            iend = letternum;
         else if (equal(end,"."))
            iend = current + 1 ;
         else if (!Numeric( end ))
         {
            printf("%s: Operand is not numeric\n", end );
            return FALSE;
         } /* if */
         else
            iend = atoi( end );

         if ( iend < istart)
         {
            printf("Ending item (%d) is less than starting item (%d)\n",
                   iend , istart );
            return FALSE;
         } /* if */

         for ( item = istart; (item <= iend) && success; item++ )
            hit = success = SetItem ( item );

      } /* else */
      else
         break ;

      if ( !success )
         return FALSE;

      if ( next_token != NULL )
      {
         strcpy( trailing , next_token );
                              /* Save current line so we can back up */
         token = strtok( next_token, WHITESPACE );
      }
      else
         token = NULL;
   } /* while */

/*--------------------------------------------------------------------*/
/*   Determine if we have a user id to search for.  This is harder    */
/*   than the above search for subject lines, because the user id     */
/*   doesn't have to have a special delimiter; thus, we do our        */
/*   best to discard other types of items and assume a user id        */
/*   only if we don't know what it is.                                */
/*--------------------------------------------------------------------*/

   if ( ! hit )
   {
      if ( (bits & (FILE_OP | USER_OP)) == 0x0000)
      {
         *input = next_token;
         return SearchUser( token, input, bits);
      }
      else if ((bits & USER_OP) == 0x0000)
      {
         if ((strpbrk(token,"@%!") != NULL) || (next_token != NULL))
         {
            *input = next_token;
            return SearchUser( token, input, bits);
         }
      }
   } /* if (! hit) */

/*--------------------------------------------------------------------*/
/*      Handle trailing operands when user selected items by number   */
/*--------------------------------------------------------------------*/

   if ( token != NULL )
   {
      if (!hit)               /* Any numeric operands?               */
         SetItem( current+1 );   /* No --> Set current as default    */
      strcpy( *input, trailing );
   }
   else
      *input = NULL ;

   return SetTrailing( input , bits );

} /* SelectItems */


/*--------------------------------------------------------------------*/
/*    S e a r c h S u j e c t                                         */
/*                                                                    */
/*    Search for mail items to select by the subject                  */
/*--------------------------------------------------------------------*/

static boolean SearchSubject( char *token,
                              char **input,
                              char *trailing,
                              const int bits)
{
   char line[LSIZE];
   int item;
   char *next_token;
   boolean hit = FALSE;

   token = strtok(trailing,"/");    /* Get subject to search      */
   if ( token == NULL )
   {
      printf("Missing subject to search for\n");
      return FALSE;
   }
   token = strlwr(token);  /* Case insensitive search             */
   next_token = strtok(NULL,"");
                           /* Get rest of line                    */
   for ( item = 1; item <= letternum; item++)
   {
      if (letters[item-1].status == M_DELETED)
         continue;
      if (
           RetrieveLine(letters[item-1].subject, line, LSIZE ) &&
          strstr( strlwr(line), token ))
                           /* This item have subject?             */
      {
         SetItem( item );
         hit = TRUE;
      } /* if */
   } /* for */

   if (hit)                /* Did we find the string for user?    */
   {
      if ( next_token == NULL )
         *input = NULL;
      else
         strcpy( *input, next_token );
      return SetTrailing( input , bits ); /* Yes --> Success      */
   } /* if (hit) */
   else {
      printf("No mail items found with subject \"%s\"\n",token);
      return FALSE;
   }  /* else */
} /* SearchSubject */


/*--------------------------------------------------------------------*/
/*    S e a r c h U s e r                                             */
/*                                                                    */
/*    Search for a user id on mail items                              */
/*--------------------------------------------------------------------*/

static boolean SearchUser( char *token , char **input, const int bits)
{
   char line[LSIZE];
   int item;
   boolean hit = FALSE;

   token = strlwr(token);  /* Case insensitive search          */

/*--------------------------------------------------------------------*/
/*    Our loop is as follows for each item in the mailbox:            */
/*                                                                    */
/*       If the letter is deleted, ignore it                          */
/*       If the From line can be retrieved from the item:             */
/*                                                                    */
/*          1) Read the line                                          */
/*          2) Scan up to the first whitespace                        */
/*             2a) If there is no whitespace, use entire line         */
/*             2b) If there is whitespace, step past it to next       */
/*                 non-whitespace character                           */
/*          3) Lowercase the line                                     */
/*          4) Scan for the the target address in the line:           */
/*             4a) If found select the item for processing            */
/*             4b) If not found, build a standard outgoing address    */
/*                 and search again.  If found, select the item       */
/*                                                                    */
/*       If the From line cannot be retrieved:                        */
/*          1) call ReturnAddress to format the return address        */
/*             (Because the line cannot be retrieved, it will         */
/*              "-- unknown --", the same string displayed by         */
/*              Headers.                                              */
/*          2) Scan for the the target address in the returned        */
/*             address                                                */
/*          3) If found, select the item for processing               */
/*--------------------------------------------------------------------*/

   for ( item = 1; item <= letternum; item++)
   {
      printmsg(2,"Examining item %d", item);
      if (letters[item-1].status == M_DELETED)
         continue;
      if (RetrieveLine(letters[item - 1].from, line, LSIZE))
      {
         char *addr  = strpbrk(line,WHITESPACE);
         if (addr == NULL)    /* Whitespace in input line?        */
            addr = line;      /* No --> Use entire line           */
         else
            while(isspace(*addr))   /* Yes --> Skip past first WS */
               addr++;
         printmsg(2,"SearchUser: Address %d is: %s",item-1,addr);
         if ( strstr( strlwr(addr), token ))    /* Find address?  */
            hit = SetItem( item );  /* Yes--> Select item for use */
         else {                     /* No--> Expand & search again*/
            char result[MAXADDR];
            BuildAddress( result, addr);
                           /* Get expanded address for user       */
            printmsg(2,"SearchUser: Formatted address %d is: %s",
                  item-1,result);
            if ( strstr( strlwr(result), token ))
                           /* This item have correct sender?      */
               hit = SetItem( item );  /* Yes --> Set it          */
            else
               printmsg(2,"SearchUser: Item %d not selected.",
                     item-1);
         } /* else */
      } /* if */
      else {
         ReturnAddress(line,&letters[item - 1]);
                           /* Get standard error text for letter  */
         printmsg(2,"SearchUser: Default address %d is: %s",
                  item-1,line);
         if ( strstr( strlwr(line), token ))
                           /* This item have correct sender?      */
            hit = SetItem( item );  /* Yes --> Set it             */
      } /* else */
   } /* for */

/*--------------------------------------------------------------------*/
/*        End of loop; determined success and return to caller        */
/*--------------------------------------------------------------------*/

   if (hit)             /* Did we find the string for user?    */
      return SetTrailing( input , bits ); /* Yes --> Success   */
   else {
      printf("No mail items found from \"%s\"\n",token);
      return FALSE;
   }  /* else */

}  /* SearchUser */


/*--------------------------------------------------------------------*/
/*    S e t T r a i l i n g                                           */
/*                                                                    */
/*    Determine success of command parse based on trailing operands   */
/*--------------------------------------------------------------------*/

boolean SetTrailing( char **input, int bits )
{

/*--------------------------------------------------------------------*/
/*                        Trim trailing spaces                        */
/*--------------------------------------------------------------------*/

   if (*input != NULL)
   {
      char *s = *input;
      while( isspace(*s))
         s++;
      if ( *s == '\0' )       /* Anything left in string?            */
         *input = NULL;       /* No --> Flag input as NULL           */
      else
         *input = s;          /* Yes --> Point input to next operand */
   }

/*--------------------------------------------------------------------*/
/*                     Trailing address operands?                     */
/*--------------------------------------------------------------------*/

   if (( bits & USER_OP ) || ( *input == NULL ))
      return TRUE;            /* Let Get_Operand check operands      */

/*--------------------------------------------------------------------*/
/*                        Trailing file name?                         */
/*--------------------------------------------------------------------*/

   if ( bits & FILE_OP )
   {
      char *token = strtok( *input , WHITESPACE );
      token = strtok( NULL , "" );

      if ( token == NULL )
         return TRUE;
      else {
         printf("%s: Only one file operand allowed on command\n",
            token);
         return FALSE;
      } /* else */
   } /* if */

/*--------------------------------------------------------------------*/
/*                   No operand allowed; reject it                    */
/*--------------------------------------------------------------------*/

   printf("%s: Unknown operand on command\n", *input);
   return FALSE;

} /* SetTrailing */

/*--------------------------------------------------------------------*/
/*    S e t I t e m                                                   */
/*                                                                    */
/*    Validate and select a single item                               */
/*--------------------------------------------------------------------*/

boolean SetItem( int item )
{
   if ( item_list == NULL )
   {
      item_list = calloc( letternum, sizeof *item_list);
      checkref( item_list );
   }

   if ((item > 0) && ( item <= letternum ))
   {
      item_list[ next_item++ ] = item - 1;
      return TRUE;
   }
   else {
      printf("Invalid item (%d) selected for processing\n",item);
      return FALSE;
   } /* else */
} /* SetItem */

/*--------------------------------------------------------------------*/
/*    G e t _ O p e r a n d                                           */
/*                                                                    */
/*    Get next operand to process                                     */
/*--------------------------------------------------------------------*/

boolean Get_Operand( int *item,
                     char **token,
                     int bits,
                     boolean first_pass )
{

/*--------------------------------------------------------------------*/
/*                         Handle no operand                          */
/*--------------------------------------------------------------------*/

   if (bits & NO_OPERANDS)
   {
      if ( *token == NULL )
         return first_pass;
      else {
         printf("Operands not allowed on this command!\n");
         return FALSE;
      } /* else */
   }

/*--------------------------------------------------------------------*/
/*        User operands are like string operands, but required        */
/*--------------------------------------------------------------------*/

   if ( (bits & USER_OP) && (*token == NULL))
   {
      printf("Missing addressees for command\n");
      return FALSE;
   }
/*--------------------------------------------------------------------*/
/*                       Handle letter operand                        */
/*--------------------------------------------------------------------*/

   if ( bits & LETTER_OP )
   {
      static size_t subscript;
      subscript = first_pass ? 0 : subscript + 1;

      if (subscript < next_item)
      {
         *item = item_list[subscript];
         return TRUE;
      } /* else */
      else {
         free( item_list );
         item_list = NULL;
         return FALSE;
      } /* else */
   } /* if*/

/*--------------------------------------------------------------------*/
/*                   Handle string operands                           */
/*--------------------------------------------------------------------*/

   if ( bits & (STRING_OP | USER_OP))
   {
      char *buf = *token;
      if (first_pass &&
          (buf != NULL) &&
          ( buf[ strlen(buf) - 1 ] == '\n'))
         buf[ strlen(buf) - 1 ] = '\0';
      return first_pass;
   }

/*--------------------------------------------------------------------*/
/*                     Handle tokenized operands                      */
/*--------------------------------------------------------------------*/

   if ( bits & TOKEN_OP )
   {
      static char *rest = NULL ;
      if (first_pass)
         rest = *token;

      if ((rest == NULL) || (*rest == (char) NULL))
      {
         *token = NULL;
         return first_pass;
      } /* if */

      *token = strtok( rest , WHITESPACE );
      if ( *token == (char) NULL)
      {
         rest = NULL;
         return first_pass;
      }
      else {
         rest = strtok( NULL , "" );
         return TRUE;
      } /* else */
   } /* if */

/*--------------------------------------------------------------------*/
/*                      Handle integer operands                       */
/*--------------------------------------------------------------------*/

   if ( bits & KEWSHORT_OP)
   {
      char *p;
      if ( (*token == NULL) || ! first_pass )
      {
         *item = 1;
         return first_pass;
      }
      p = strtok( *token, WHITESPACE );

      if (!Numeric( p ))
      {
         printf("%s: Operand is not numeric\n", p );
         return FALSE;
      } /* if */

      *item = atoi( p );
      p = strtok( NULL, WHITESPACE );
      if (p != NULL )
      {
         printf("%s: extra operand not allowed on command\n", p);
         return FALSE;
      }
      return TRUE;
   } /* if */

/*--------------------------------------------------------------------*/
/*                   We cannot handle this command                    */
/*--------------------------------------------------------------------*/

   printf("Unknown processing option = %x, cannot process command\n",
         bits);
   return FALSE;

} /* Get_Operand */

/*--------------------------------------------------------------------*/
/*    P u s h I t e m L i s t                                         */
/*                                                                    */
/*    Save item parsing list                                          */
/*--------------------------------------------------------------------*/

int PushItemList( int **save_list )
{
   *save_list = item_list;
   item_list = NULL;
   return next_item;
} /* PushItemList */

/*--------------------------------------------------------------------*/
/*    P o p I t e m L i s t                                           */
/*                                                                    */
/*    Restore parsing information saved by PushItemList               */
/*--------------------------------------------------------------------*/

void PopItemList( int *save_list, int save_item )
{
   item_list = save_list;
   next_item = save_item;
} /* PopItemList */
