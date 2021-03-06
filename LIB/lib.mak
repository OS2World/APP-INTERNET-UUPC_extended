# *--------------------------------------------------------------------*
# *     l i b . m a k                                                  *
# *                                                                    *
# *     Changes Copyright (c) 1989-1993 by Kendra Electronic           *
# *     Wonderworks.                                                   *
# *                                                                    *
# *     All rights reserved except those explicitly granted by the     *
# *     UUPC/extended license agreement.                               *
# *--------------------------------------------------------------------*

#       $Id: lib.mak 1.11 1993/09/29 04:48:23 ahd Exp $
#
#       Revision history:
#       $Log: lib.mak $
#    Revision 1.11  1993/09/29  04:48:23  ahd
#    Add usr signal handler
#    Use one long tlib command to build library
#
#    Revision 1.10  1993/09/24  03:42:24  ahd
#    Add OS/2 error module (pos2err.c)
#
#    Revision 1.9  1993/09/20  04:36:42  ahd
#    TCP/IP support from Dave Watt
#    't' protocol support
#    BC++ 1.0 for OS/2 support
#
#    Revision 1.8  1993/07/31  16:21:21  ahd
#    Windows 3.x support
#
#    Revision 1.7  1993/07/22  23:19:01  ahd
#    Make library build more generic
#
#    Revision 1.6  1993/04/05  04:31:55  ahd
#    Add time stamp, size to data returned by directory searches
#
#    Revision 1.5  1993/03/06  22:47:34  ahd
#    Move active into shared library
#
#       Revision 1.4  1992/11/27  14:37:34  ahd
#       Add scrsize() to build
#
#       Revision 1.3  1992/11/23  02:46:06  ahd
#       Addd strpool and normalize to build list
#
#       Revision 1.2  1992/11/17  13:47:42  ahd
#       Drop type of input file
#

!include $(UUPCDEFS)

.c.obj:
  $(CC) -c $(CCX) -I$: { $<}

.asm.obj:
        $(TASM) $(TASMOPT) $<,$(OBJ)\$&;

.path.c   = $(LIB)

#       The names of various object files that we create.

LIBLST1= $(OBJ)\active.obj\
         $(OBJ)\arbmath.obj $(OBJ)\arpadate.obj $(OBJ)\bugout.obj\
         $(OBJ)\catcher.obj $(OBJ)\chdir.obj $(OBJ)\checkptr.obj\
         $(OBJ)\configur.obj $(OBJ)\dater.obj $(OBJ)\dos2unix.obj\
         $(OBJ)\expath.obj $(OBJ)\execute.obj
LIBLST2= $(OBJ)\export.obj $(OBJ)\filebkup.obj $(OBJ)\fopen.obj\
         $(OBJ)\getargs.obj $(OBJ)\getopt.obj $(OBJ)\getseq.obj\
         $(OBJ)\hostable.obj $(OBJ)\hostatus.obj
LIBLST3= $(OBJ)\hostrset.obj $(OBJ)\import.obj $(OBJ)\importng.obj\
         $(OBJ)\kanjicnv.obj $(OBJ)\lock.obj $(OBJ)\logger.obj\
         $(OBJ)\mkdir.obj $(OBJ)\mkfilenm.obj $(OBJ)\mkmbox.obj
LIBLST4= $(OBJ)\mktempnm.obj $(OBJ)\printerr.obj\
         $(OBJ)\printmsg.obj $(OBJ)\pushpop.obj $(OBJ)\readnext.obj\
         $(OBJ)\rename.obj $(OBJ)\safeio.obj $(OBJ)\normaliz.obj
LIBLST5= $(OBJ)\safeout.obj $(OBJ)\security.obj $(OBJ)\ssleep.obj\
         $(OBJ)\stater.obj $(OBJ)\usertabl.obj $(OBJ)\validcmd.obj\
         $(OBJ)\strpool.obj $(OBJ)\trumpet.obj $(OBJ)\usrcatch.obj \
         $(TIMESTMP)
LIBDOS = $(OBJ)\scrsize.obj $(OBJ)\ndir.obj
LIBOS2 = $(OBJ)\scrsize2.obj $(OBJ)\ndiros2.obj  $(OBJ)\pos2err.obj
LIBWIN = $(OBJ)\scrsize.obj $(OBJ)\ndirwin.obj  $(OBJ)\winutil.obj \
         $(OBJ)\pwinsock.obj
LIBLST = $(LIBLST1) $(LIBLST2) $(LIBLST3) $(LIBLST4) $(LIBLST5)

!if $d(__OS2__)
LIBALL = $(LIBLST) $(LIBOS2)
!elif $d(WINDOWS)
LIBALL = $(LIBLST) $(LIBWIN)
!else
LIBALL = $(LIBLST) $(LIBDOS)
!endif

# *--------------------------------------------------------------------*
# *     Force a regeneration of the time stamp/version module.         *
# *--------------------------------------------------------------------*

regen:  $(LIB)\timestmp.c
        - erase $(TIMESTMP)

# *--------------------------------------------------------------------*
# *     The timestamp module has a reference to this MAKEFILE,         *
# *     which insures whenever we change the version number the        *
# *     time stamp gets re-generated.                                  *
# *--------------------------------------------------------------------*

$(TIMESTMP): $(LIB)\timestmp.c $(UUPCCFG) $(REGEN) \
                $(MAKEFILE) \
                $(LIB)\lib.mak \
                $(MAIL)\mail.mak \
                $(UUCP)\uucp.mak \
                $(UUCICO)\uucico.mak \
                $(UTIL)\util.mak

# *--------------------------------------------------------------------*
# *     Common library build                                           *
# *--------------------------------------------------------------------*

$(UUPCLIB): $(LIBALL)
#       &TLIB /C /E $< -+$?
        - erase $(WORKFILE)
!if $d(__OS2__)
        &echo -+$? ^& >> $(WORKFILE)
!else
        &echo -+$? & >> $(WORKFILE)
!endif
        echo ,NUL >> $(WORKFILE)
        TLIB /C /E $< @$(WORKFILE)
        - erase $(TEMP)\$&.BAK

# *--------------------------------------------------------------------*
# *               We don't optimize the sleep routine!                 *
# *--------------------------------------------------------------------*

ssleep.obj: ssleep.c
        $(CC) -c $(CCX) -Od -I$: { $<}

$(LIB)\win32ver.h: $(MAKEFILE) $(REGEN)
        copy &&|
/*--------------------------------------------------------------------*/
/*         DO NOT EDIT -- AUTOMATICALLY GENERATED BY MAKEFILE         */
/*--------------------------------------------------------------------*/

#ifndef UUPCV
#define UUPCV "$(VERS)"         // UUPC/extended version number
#endif
| $<
