/* exif-mnote-data-canon.h
 *
 * Copyright � 2002, 2003 Lutz M�ller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __EXIF_MNOTE_DATA_CANON_H__
#define __EXIF_MNOTE_DATA_CANON_H__

#include <libexif/exif-dll.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-mnote-data.h>
#include <libexif/exif-mnote-data-priv.h>
#include <libexif/exif-mem.h>
#include <libexif/exif-data.h>

typedef struct _ExifMnoteDataCanon ExifMnoteDataCanon;

#include <libexif/canon/mnote-canon-entry.h>

struct _ExifMnoteDataCanon
{
    ExifMnoteData parent;

    MnoteCanonEntry* entries;
    unsigned int count;

    ExifByteOrder order;
    unsigned int offset;

    ExifDataOption options;
};

EXIF_EXPORT ExifMnoteData* exif_mnote_data_canon_new(ExifMem* mem,
                                                     ExifDataOption o);

#endif /* __EXIF_MNOTE_DATA_CANON_H__ */
