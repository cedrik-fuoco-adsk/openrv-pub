/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@zoy.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __FTGlyphImpl__
#define __FTGlyphImpl__

#include "FTGL/ftgl.h"

class FTGlyphImpl
{
    friend class FTGlyph;

protected:
    FTGlyphImpl(FT_GlyphSlot glyph, bool useDisplayList = true);

    virtual ~FTGlyphImpl();

    float Advance() const;

    const FTBBox& BBox() const;

    FT_Error Error() const;

    /**
     * The advance distance for this glyph
     */
    FTPoint advance;

    /**
     * The bounding box of this glyph.
     */
    FTBBox bBox;

    /**
     * Current error code. Zero means no error.
     */
    FT_Error err;
};

#endif //  __FTGlyphImpl__
