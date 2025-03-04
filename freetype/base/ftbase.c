/***************************************************************************/
/*                                                                         */
/*  ftbase.c                                                               */
/*                                                                         */
/*    Single object library component (body only).                         */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2006, 2007 by                   */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#include <ft2build.h>

#define FT_MAKE_OPTION_SINGLE_OBJECT

#include "ftcalc.c"
#include "ftdbgmem.c"
#include "ftgloadr.c"
#include "ftnames.c"
#include "ftobjs.c"
#include "ftoutln.c"
#include "ftrfork.c"
#include "ftstream.c"
#include "fttrigon.c"
#include "ftutil.c"

#if defined(__APPLE__) && !defined(DARWIN_NO_CARBON)
#include <ftmac.c>
#endif

/* END */
