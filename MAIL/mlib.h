#ifndef MLIB_H
#define MLIB_H

/*--------------------------------------------------------------------*/
/*       m l i b . h                                                  */
/*                                                                    */
/*       Interactive support functions for UUPC/extended mail         */
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
 *    $Id: mlib.h 1.2 1993/07/31 16:28:59 ahd Exp $
 *
 *    Revision history:
 *    $Log: mlib.h $
 * Revision 1.2  1993/07/31  16:28:59  ahd
 * Changes in support of Robert Denny's Windows Support
 *
 *
 *    01 Oct 89      Add prototypes for Console_fgets, Is_Console
 *    07 Sep 90      More prototypes
 */

extern int Get_One( void );

extern boolean Console_fgets(char *buff,
                              int buflen, char *prompt);          /* ahd   */

int Invoke(const char *ecmd,
           const char *filename);

extern boolean Is_Console(FILE *stream);                          /* ahd   */

extern void ClearScreen( void );                                  /* ahd   */

#endif
