/* exif-mnote-data-pentax.h
 *
 * Copyright � 2002 Lutz M�ller <lutz@users.sourceforge.net>
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

#ifndef __EXIF_MNOTE_DATA_PENTAX_H__
#define __EXIF_MNOTE_DATA_PENTAX_H__

#include <libexif/exif-dll.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-mnote-data.h>
#include <libexif/exif-mnote-data-priv.h>
#include <libexif/pentax/mnote-pentax-entry.h>
#include <libexif/exif-mem.h>

enum PentaxVersion
{
    pentaxV1 = 1,
    pentaxV2 = 2,
    pentaxV3 = 4,
    casioV2 = 4
};

typedef struct _ExifMnoteDataPentax ExifMnoteDataPentax;

struct _ExifMnoteDataPentax
{
    ExifMnoteData parent;

    MnotePentaxEntry* entries;
    unsigned int count;

    ExifByteOrder order;
    unsigned int offset;

    enum PentaxVersion version;
};

EXIF_EXPORT ExifMnoteData* exif_mnote_data_pentax_new(ExifMem*);

#endif /* __EXIF_MNOTE_DATA_PENTAX_H__ */
