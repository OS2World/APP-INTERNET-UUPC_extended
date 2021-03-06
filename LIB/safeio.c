/*--------------------------------------------------------------------*/
/*    s a f e i o . c                                                 */
/*                                                                    */
/*    Console I/O functions for use during interrupt processing       */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*    Changes Copyright (c) 1989-1993 by Kendra Electronic            */
/*    Wonderworks.                                                    */
/*                                                                    */
/*    All rights reserved except those explicitly granted by the      */
/*    UUPC/extended license agreement.                                */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*                          RCS Information                           */
/*--------------------------------------------------------------------*/

/*
 *    $Id: safeio.c 1.5 1993/10/03 00:03:45 ahd Exp $
 *
 *    Revision history:
 *    $Log: safeio.c $
 *     Revision 1.5  1993/10/03  00:03:45  ahd
 *     Only use currentfile() under Windows NT
 *
 *     Revision 1.4  1993/09/20  04:39:51  ahd
 *     OS/2 2.x support
 *
 *     Revision 1.3  1993/07/20  21:42:43  dmwatt
 *     Don't rely on standard I/O under Windows/NT
 *
 */

/*--------------------------------------------------------------------*/
/*    Since C I/O functions are not safe inside signal routines,      */
/*    the code uses conditionals to use system-level DOS and OS/2     */
/*    services.  Another option is to set global flags and do any     */
/*    I/O operations outside the signal handler.                      */
/*--------------------------------------------------------------------*/

#define __MSC                 /* Make Borland C++ 2.0 act like MS C  */

#include <stdio.h>

#if defined( WIN32 )
    #include <windows.h>
    #include <string.h>
#elif defined( FAMILYAPI ) || defined(__OS2__)
    #define INCL_NOCOMMON
    #define INCL_NOPM
    #define INCL_VIO
    #define INCL_KBD
    #include <os2.h>
    #include <string.h>
#else /* FAMILYAPI */
    #include <conio.h>
    #include <dos.h>
    #include <bios.h>
#endif

/*--------------------------------------------------------------------*/
/*                    UUPC/extended include files                     */
/*--------------------------------------------------------------------*/

#include "lib.h"
#include "safeio.h"

/*--------------------------------------------------------------------*/
/*                          Global variables                          */
/*--------------------------------------------------------------------*/

#if defined( WIN32 )
currentfile();
#endif

/*--------------------------------------------------------------------*/
/*    s a f e i n                                                     */
/*                                                                    */
/*    Inputs a character using system level calls.  From MicroSoft    */
/*    Programmer's Workbench QuickHelp samples                        */
/*--------------------------------------------------------------------*/

#if defined(WIN32)

static HANDLE hConsoleIn = INVALID_HANDLE_VALUE;

/*--------------------------------------------------------------------*/
/*       I n i t C o n s o l e I n p u t H a n d l e                  */
/*                                                                    */
/*       Initialize Window NT console handle allow reading            */
/*       from console when stdin is redirected.                       */
/*--------------------------------------------------------------------*/

void InitConsoleInputHandle(void)
{
   hConsoleIn = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE, 0, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

   if (hConsoleIn == INVALID_HANDLE_VALUE) {
      printmsg(0, "InitConsoleInputHandle:  could not open console handles!");
      panic();
   }
}
#endif


int safein( void )
{
#ifdef _Windows

/*--------------------------------------------------------------------*/
/*                       Windows get character                        */
/*--------------------------------------------------------------------*/

   return getchar( );

#elif defined( WIN32 )

/*--------------------------------------------------------------------*/
/*                      Windows NT get character                      */
/*--------------------------------------------------------------------*/

   CHAR ch;
   DWORD dwBytesRead;

   if (hConsoleIn == INVALID_HANDLE_VALUE)
      InitConsoleInputHandle();

   ReadFile(hConsoleIn, &ch, 1, &dwBytesRead, NULL);

   return ch;

#elif defined( FAMILYAPI ) || defined( __OS2__ )

/*--------------------------------------------------------------------*/
/*                         OS/2 Get character                         */
/*--------------------------------------------------------------------*/

    KBDKEYINFO kki;

    KbdCharIn( &kki, IO_WAIT, 0 );
    return kki.chChar;

#else /* FAMILYAPI */

/*--------------------------------------------------------------------*/
/*                         DOS get character                          */
/*--------------------------------------------------------------------*/

    int c = (_bios_keybrd( _KEYBRD_READ ) & 0xff );
    union REGS inregs, outregs;

    inregs.h.ah = 0x0e;
    inregs.h.al = (char) c;
    int86( 0x10, &inregs, &outregs );
    return c;

#endif

} /* safein */

/*--------------------------------------------------------------------*/
/*    s a f e p e e k                                                 */
/*                                                                    */
/*    Determine if a character is waiting at the keyboard for us.     */
/*    Written by ahd based on safein (above).                         */
/*--------------------------------------------------------------------*/

boolean safepeek( void )
{

#ifdef _Windows

   return 0;

#elif defined(WIN32)

   INPUT_RECORD Buffer;
   DWORD nEventsRead;

   if (hConsoleIn == INVALID_HANDLE_VALUE)
      InitConsoleInputHandle();

   PeekConsoleInput(hConsoleIn, &Buffer, 1, &nEventsRead);

   if (nEventsRead != 0 && Buffer.EventType == KEY_EVENT)
      return TRUE;
   return FALSE;

#elif defined( FAMILYAPI ) || defined(__OS2__)

    KBDKEYINFO kki;

    KbdPeek( &kki, 0 );

#ifdef __OS2__
    return (kki.fbStatus & KBDTRF_FINAL_CHAR_IN);
#else
    return (kki.fbStatus & FINAL_CHAR_IN);
#endif

#else /* FAMILYAPI */

/*--------------------------------------------------------------------*/
/*                         DOS Keyboard peek                          */
/*--------------------------------------------------------------------*/

    return (_bios_keybrd( _KEYBRD_READY ) & 0xff );

#endif /* _Windows */

} /* safepeek */

/*--------------------------------------------------------------------*/
/*    s a f e f l u s h                                               */
/*                                                                    */
/*    Flush the keyboard look ahead buffer.                           */
/*    Written by ahd based on safein (above).                         */
/*--------------------------------------------------------------------*/

void safeflush( void )
{

#ifdef _Windows

   return;

#elif defined(WIN32)

   if (hConsoleIn == INVALID_HANDLE_VALUE)
      InitConsoleInputHandle();

   FlushConsoleInputBuffer(hConsoleIn);

#elif defined( FAMILYAPI ) || defined(__OS2__)

    KbdFlushBuffer( 0 );      /* That's all!  (Makes you love rich
                                 API's, doesn't it?)                 */

#else

/*--------------------------------------------------------------------*/
/*                         DOS Keyboard flush                         */
/*--------------------------------------------------------------------*/

   union REGS regs;

   regs.h.ah = 0x0C;       /* Flush buffer, read keyboard            */
   regs.h.al = 0x00;       /* Don't actually read keyboard           */
   intdos( &regs, &regs ); /* Make it happen                         */

#endif /* _Windows */

} /* safeflush */
