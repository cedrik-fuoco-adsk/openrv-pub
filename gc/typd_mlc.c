/*
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * opyright (c) 1999-2000 by Hewlett-Packard Company.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#include "private/gc_pmark.h"

/*
 * Some simple primitives for allocation with explicit type information.
 * Simple objects are allocated such that they contain a GC_descr at the
 * end (in the last allocated word).  This descriptor may be a procedure
 * which then examines an extended descriptor passed as its environment.
 *
 * Arrays are treated as simple objects if they have sufficiently simple
 * structure.  Otherwise they are allocated from an array kind that supplies
 * a special mark procedure.  These arrays contain a pointer to a
 * complex_descriptor as their last word.
 * This is done because the environment field is too small, and the collector
 * must trace the complex_descriptor.
 *
 * Note that descriptors inside objects may appear cleared, if we encounter a
 * false reference to an object on a free list.  In the GC_descr case, this
 * is OK, since a 0 descriptor corresponds to examining no fields.
 * In the complex_descriptor case, we explicitly check for that case.
 *
 * MAJOR PARTS OF THIS CODE HAVE NOT BEEN TESTED AT ALL and are not testable,
 * since they are not accessible through the current interface.
 */

#include "gc_typed.h"

#define TYPD_EXTRA_BYTES (sizeof(word) - EXTRA_BYTES)

STATIC GC_bool GC_explicit_typing_initialized = FALSE;

STATIC int GC_explicit_kind = 0;
/* Object kind for objects with indirect        */
/* (possibly extended) descriptors.             */

STATIC int GC_array_kind = 0;

/* Object kind for objects with complex         */
/* descriptors and GC_array_mark_proc.          */

/* Extended descriptors.  GC_typed_mark_proc understands these. */
/* These are used for simple objects that are larger than what  */
/* can be described by a BITMAP_BITS sized bitmap.              */
typedef struct
{
    word ed_bitmap;       /* lsb corresponds to first word.       */
    GC_bool ed_continued; /* next entry is continuation.  */
} ext_descr;

/* Array descriptors.  GC_array_mark_proc understands these.    */
/* We may eventually need to add provisions for headers and     */
/* trailers.  Hence we provide for tree structured descriptors, */
/* though we don't really use them currently.                   */
typedef union ComplexDescriptor
{
    struct LeafDescriptor
    { /* Describes simple array       */
        word ld_tag;
#define LEAF_TAG 1
        size_t ld_size;         /* bytes per element    */
                                /* multiple of ALIGNMENT        */
        size_t ld_nelements;    /* Number of elements.  */
        GC_descr ld_descriptor; /* A simple length, bitmap,     */
                                /* or procedure descriptor.     */
    } ld;

    struct ComplexArrayDescriptor
    {
        word ad_tag;
#define ARRAY_TAG 2
        size_t ad_nelements;
        union ComplexDescriptor* ad_element_descr;
    } ad;

    struct SequenceDescriptor
    {
        word sd_tag;
#define SEQUENCE_TAG 3
        union ComplexDescriptor* sd_first;
        union ComplexDescriptor* sd_second;
    } sd;
} complex_descriptor;

#define TAG ld.ld_tag

STATIC ext_descr* GC_ext_descriptors = NULL;
/* Points to array of extended  */
/* descriptors.                 */

STATIC size_t GC_ed_size = 0; /* Current size of above arrays.        */
#define ED_INITIAL_SIZE 100

STATIC size_t GC_avail_descr = 0; /* Next available slot.         */

STATIC int GC_typed_mark_proc_index = 0; /* Indices of my mark          */
STATIC int GC_array_mark_proc_index = 0; /* procedures.                 */

STATIC void GC_push_typed_structures_proc(void)
{
    GC_push_all((ptr_t)&GC_ext_descriptors,
                (ptr_t)&GC_ext_descriptors + sizeof(word));
}

/* Add a multiword bitmap to GC_ext_descriptors arrays.  Return */
/* starting index.                                              */
/* Returns -1 on failure.                                       */
/* Caller does not hold allocation lock.                        */
STATIC signed_word GC_add_ext_descriptor(GC_bitmap bm, word nbits)
{
    size_t nwords = divWORDSZ(nbits + WORDSZ - 1);
    signed_word result;
    size_t i;
    word last_part;
    size_t extra_bits;
    DCL_LOCK_STATE;

    LOCK();
    while (GC_avail_descr + nwords >= GC_ed_size)
    {
        ext_descr* new;
        size_t new_size;
        word ed_size = GC_ed_size;

        if (ed_size == 0)
        {
            GC_push_typed_structures = GC_push_typed_structures_proc;
            UNLOCK();
            new_size = ED_INITIAL_SIZE;
        }
        else
        {
            UNLOCK();
            new_size = 2 * ed_size;
            if (new_size > MAX_ENV)
                return (-1);
        }
        new = (ext_descr*)GC_malloc_atomic(new_size * sizeof(ext_descr));
        if (new == 0)
            return (-1);
        LOCK();
        if (ed_size == GC_ed_size)
        {
            if (GC_avail_descr != 0)
            {
                BCOPY(GC_ext_descriptors, new,
                      GC_avail_descr * sizeof(ext_descr));
            }
            GC_ed_size = new_size;
            GC_ext_descriptors = new;
        } /* else another thread already resized it in the meantime */
    }
    result = GC_avail_descr;
    for (i = 0; i < nwords - 1; i++)
    {
        GC_ext_descriptors[result + i].ed_bitmap = bm[i];
        GC_ext_descriptors[result + i].ed_continued = TRUE;
    }
    last_part = bm[i];
    /* Clear irrelevant bits. */
    extra_bits = nwords * WORDSZ - nbits;
    last_part <<= extra_bits;
    last_part >>= extra_bits;
    GC_ext_descriptors[result + i].ed_bitmap = last_part;
    GC_ext_descriptors[result + i].ed_continued = FALSE;
    GC_avail_descr += nwords;
    UNLOCK();
    return (result);
}

/* Table of bitmap descriptors for n word long all pointer objects.     */
STATIC GC_descr GC_bm_table[WORDSZ / 2];

/* Return a descriptor for the concatenation of 2 nwords long objects,  */
/* each of which is described by descriptor.                            */
/* The result is known to be short enough to fit into a bitmap          */
/* descriptor.                                                          */
/* Descriptor is a GC_DS_LENGTH or GC_DS_BITMAP descriptor.             */
STATIC GC_descr GC_double_descr(GC_descr descriptor, word nwords)
{
    if ((descriptor & GC_DS_TAGS) == GC_DS_LENGTH)
    {
        descriptor = GC_bm_table[BYTES_TO_WORDS((word)descriptor)];
    };
    descriptor |= (descriptor & ~GC_DS_TAGS) >> nwords;
    return (descriptor);
}

STATIC complex_descriptor*
GC_make_sequence_descriptor(complex_descriptor* first,
                            complex_descriptor* second);

/* Build a descriptor for an array with nelements elements,     */
/* each of which can be described by a simple descriptor.       */
/* We try to optimize some common cases.                        */
/* If the result is COMPLEX, then a complex_descr* is returned  */
/* in *complex_d.                                                       */
/* If the result is LEAF, then we built a LeafDescriptor in     */
/* the structure pointed to by leaf.                            */
/* The tag in the leaf structure is not set.                    */
/* If the result is SIMPLE, then a GC_descr                     */
/* is returned in *simple_d.                                    */
/* If the result is NO_MEM, then                                */
/* we failed to allocate the descriptor.                        */
/* The implementation knows that GC_DS_LENGTH is 0.             */
/* *leaf, *complex_d, and *simple_d may be used as temporaries  */
/* during the construction.                                     */
#define COMPLEX 2
#define LEAF 1
#define SIMPLE 0
#define NO_MEM (-1)

STATIC int GC_make_array_descriptor(size_t nelements, size_t size,
                                    GC_descr descriptor, GC_descr* simple_d,
                                    complex_descriptor** complex_d,
                                    struct LeafDescriptor* leaf)
{
#define OPT_THRESHOLD 50
    /* For larger arrays, we try to combine descriptors of adjacent */
    /* descriptors to speed up marking, and to reduce the amount    */
    /* of space needed on the mark stack.                           */
    if ((descriptor & GC_DS_TAGS) == GC_DS_LENGTH)
    {
        if (descriptor == (GC_descr)size)
        {
            *simple_d = nelements * descriptor;
            return (SIMPLE);
        }
        else if ((word)descriptor == 0)
        {
            *simple_d = (GC_descr)0;
            return (SIMPLE);
        }
    }
    if (nelements <= OPT_THRESHOLD)
    {
        if (nelements <= 1)
        {
            if (nelements == 1)
            {
                *simple_d = descriptor;
                return (SIMPLE);
            }
            else
            {
                *simple_d = (GC_descr)0;
                return (SIMPLE);
            }
        }
    }
    else if (size <= BITMAP_BITS / 2 && (descriptor & GC_DS_TAGS) != GC_DS_PROC
             && (size & (sizeof(word) - 1)) == 0)
    {
        int result = GC_make_array_descriptor(
            nelements / 2, 2 * size,
            GC_double_descr(descriptor, BYTES_TO_WORDS(size)), simple_d,
            complex_d, leaf);
        if ((nelements & 1) == 0)
        {
            return (result);
        }
        else
        {
            struct LeafDescriptor* one_element =
                (struct LeafDescriptor*)GC_malloc_atomic(
                    sizeof(struct LeafDescriptor));

            if (result == NO_MEM || one_element == 0)
                return (NO_MEM);
            one_element->ld_tag = LEAF_TAG;
            one_element->ld_size = size;
            one_element->ld_nelements = 1;
            one_element->ld_descriptor = descriptor;
            switch (result)
            {
            case SIMPLE:
            {
                struct LeafDescriptor* beginning =
                    (struct LeafDescriptor*)GC_malloc_atomic(
                        sizeof(struct LeafDescriptor));
                if (beginning == 0)
                    return (NO_MEM);
                beginning->ld_tag = LEAF_TAG;
                beginning->ld_size = size;
                beginning->ld_nelements = 1;
                beginning->ld_descriptor = *simple_d;
                *complex_d = GC_make_sequence_descriptor(
                    (complex_descriptor*)beginning,
                    (complex_descriptor*)one_element);
                break;
            }
            case LEAF:
            {
                struct LeafDescriptor* beginning =
                    (struct LeafDescriptor*)GC_malloc_atomic(
                        sizeof(struct LeafDescriptor));
                if (beginning == 0)
                    return (NO_MEM);
                beginning->ld_tag = LEAF_TAG;
                beginning->ld_size = leaf->ld_size;
                beginning->ld_nelements = leaf->ld_nelements;
                beginning->ld_descriptor = leaf->ld_descriptor;
                *complex_d = GC_make_sequence_descriptor(
                    (complex_descriptor*)beginning,
                    (complex_descriptor*)one_element);
                break;
            }
            case COMPLEX:
                *complex_d = GC_make_sequence_descriptor(
                    *complex_d, (complex_descriptor*)one_element);
                break;
            }
            return (COMPLEX);
        }
    }

    leaf->ld_size = size;
    leaf->ld_nelements = nelements;
    leaf->ld_descriptor = descriptor;
    return (LEAF);
}

STATIC complex_descriptor*
GC_make_sequence_descriptor(complex_descriptor* first,
                            complex_descriptor* second)
{
    struct SequenceDescriptor* result = (struct SequenceDescriptor*)GC_malloc(
        sizeof(struct SequenceDescriptor));
    /* Can't result in overly conservative marking, since tags are      */
    /* very small integers. Probably faster than maintaining type       */
    /* info.                                                            */
    if (result != 0)
    {
        result->sd_tag = SEQUENCE_TAG;
        result->sd_first = first;
        result->sd_second = second;
    }
    return ((complex_descriptor*)result);
}

#ifdef UNDEFINED
complex_descriptor* GC_make_complex_array_descriptor(word nelements,
                                                     complex_descriptor* descr)
{
    struct ComplexArrayDescriptor* result =
        (struct ComplexArrayDescriptor*)GC_malloc(
            sizeof(struct ComplexArrayDescriptor));

    if (result != 0)
    {
        result->ad_tag = ARRAY_TAG;
        result->ad_nelements = nelements;
        result->ad_element_descr = descr;
    }
    return ((complex_descriptor*)result);
}
#endif

STATIC ptr_t* GC_eobjfreelist = NULL;

STATIC ptr_t* GC_arobjfreelist = NULL;

STATIC mse* GC_typed_mark_proc(word* addr, mse* mark_stack_ptr,
                               mse* mark_stack_limit, word env);

STATIC mse* GC_array_mark_proc(word* addr, mse* mark_stack_ptr,
                               mse* mark_stack_limit, word env);

/* Caller does not hold allocation lock. */
STATIC void GC_init_explicit_typing(void)
{
    register unsigned i;
    DCL_LOCK_STATE;

    GC_STATIC_ASSERT(sizeof(struct LeafDescriptor) % sizeof(word) == 0);
    LOCK();
    if (GC_explicit_typing_initialized)
    {
        UNLOCK();
        return;
    }
    GC_explicit_typing_initialized = TRUE;
    /* Set up object kind with simple indirect descriptor. */
    GC_eobjfreelist = (ptr_t*)GC_new_free_list_inner();
    GC_explicit_kind = GC_new_kind_inner(
        (void**)GC_eobjfreelist,
        (((word)WORDS_TO_BYTES(-1)) | GC_DS_PER_OBJECT), TRUE, TRUE);
    /* Descriptors are in the last word of the object. */
    GC_typed_mark_proc_index = GC_new_proc_inner(GC_typed_mark_proc);
    /* Set up object kind with array descriptor. */
    GC_arobjfreelist = (ptr_t*)GC_new_free_list_inner();
    GC_array_mark_proc_index = GC_new_proc_inner(GC_array_mark_proc);
    GC_array_kind = GC_new_kind_inner((void**)GC_arobjfreelist,
                                      GC_MAKE_PROC(GC_array_mark_proc_index, 0),
                                      FALSE, TRUE);
    for (i = 0; i < WORDSZ / 2; i++)
    {
        GC_descr d = (((word)(-1)) >> (WORDSZ - i)) << (WORDSZ - i);
        d |= GC_DS_BITMAP;
        GC_bm_table[i] = d;
    }
    UNLOCK();
}

STATIC mse* GC_typed_mark_proc(word* addr, mse* mark_stack_ptr,
                               mse* mark_stack_limit, word env)
{
    word bm = GC_ext_descriptors[env].ed_bitmap;
    word* current_p = addr;
    word current;
    ptr_t greatest_ha = GC_greatest_plausible_heap_addr;
    ptr_t least_ha = GC_least_plausible_heap_addr;
    DECLARE_HDR_CACHE;

    INIT_HDR_CACHE;
    for (; bm != 0; bm >>= 1, current_p++)
    {
        if (bm & 1)
        {
            current = *current_p;
            FIXUP_POINTER(current);
            if ((ptr_t)current >= least_ha && (ptr_t)current <= greatest_ha)
            {
                PUSH_CONTENTS((ptr_t)current, mark_stack_ptr, mark_stack_limit,
                              (ptr_t)current_p, exit1);
            }
        }
    }
    if (GC_ext_descriptors[env].ed_continued)
    {
        /* Push an entry with the rest of the descriptor back onto the  */
        /* stack.  Thus we never do too much work at once.  Note that   */
        /* we also can't overflow the mark stack unless we actually     */
        /* mark something.                                              */
        mark_stack_ptr++;
        if (mark_stack_ptr >= mark_stack_limit)
        {
            mark_stack_ptr = GC_signal_mark_stack_overflow(mark_stack_ptr);
        }
        mark_stack_ptr->mse_start = (ptr_t)(addr + WORDSZ);
        mark_stack_ptr->mse_descr =
            GC_MAKE_PROC(GC_typed_mark_proc_index, env + 1);
    }
    return (mark_stack_ptr);
}

/* Return the size of the object described by d.  It would be faster to */
/* store this directly, or to compute it as part of                     */
/* GC_push_complex_descriptor, but hopefully it doesn't matter.         */
STATIC word GC_descr_obj_size(complex_descriptor* d)
{
    switch (d->TAG)
    {
    case LEAF_TAG:
        return (d->ld.ld_nelements * d->ld.ld_size);
    case ARRAY_TAG:
        return (d->ad.ad_nelements * GC_descr_obj_size(d->ad.ad_element_descr));
    case SEQUENCE_TAG:
        return (GC_descr_obj_size(d->sd.sd_first)
                + GC_descr_obj_size(d->sd.sd_second));
    default:
        ABORT("Bad complex descriptor");
        /*NOTREACHED*/ return 0; /*NOTREACHED*/
    }
}

/* Push descriptors for the object at addr with complex descriptor d    */
/* onto the mark stack.  Return 0 if the mark stack overflowed.         */
STATIC mse* GC_push_complex_descriptor(word* addr, complex_descriptor* d,
                                       mse* msp, mse* msl)
{
    register ptr_t current = (ptr_t)addr;
    register word nelements;
    register word sz;
    register word i;

    switch (d->TAG)
    {
    case LEAF_TAG:
    {
        register GC_descr descr = d->ld.ld_descriptor;

        nelements = d->ld.ld_nelements;
        if (msl - msp <= (ptrdiff_t)nelements)
            return (0);
        sz = d->ld.ld_size;
        for (i = 0; i < nelements; i++)
        {
            msp++;
            msp->mse_start = current;
            msp->mse_descr = descr;
            current += sz;
        }
        return (msp);
    }
    case ARRAY_TAG:
    {
        register complex_descriptor* descr = d->ad.ad_element_descr;

        nelements = d->ad.ad_nelements;
        sz = GC_descr_obj_size(descr);
        for (i = 0; i < nelements; i++)
        {
            msp = GC_push_complex_descriptor((word*)current, descr, msp, msl);
            if (msp == 0)
                return (0);
            current += sz;
        }
        return (msp);
    }
    case SEQUENCE_TAG:
    {
        sz = GC_descr_obj_size(d->sd.sd_first);
        msp = GC_push_complex_descriptor((word*)current, d->sd.sd_first, msp,
                                         msl);
        if (msp == 0)
            return (0);
        current += sz;
        msp = GC_push_complex_descriptor((word*)current, d->sd.sd_second, msp,
                                         msl);
        return (msp);
    }
    default:
        ABORT("Bad complex descriptor");
        /*NOTREACHED*/ return 0; /*NOTREACHED*/
    }
}

/*ARGSUSED*/
STATIC mse* GC_array_mark_proc(word* addr, mse* mark_stack_ptr,
                               mse* mark_stack_limit, word env)
{
    hdr* hhdr = HDR(addr);
    size_t sz = hhdr->hb_sz;
    size_t nwords = BYTES_TO_WORDS(sz);
    complex_descriptor* descr = (complex_descriptor*)(addr[nwords - 1]);
    mse* orig_mark_stack_ptr = mark_stack_ptr;
    mse* new_mark_stack_ptr;

    if (descr == 0)
    {
        /* Found a reference to a free list entry.  Ignore it. */
        return (orig_mark_stack_ptr);
    }
    /* In use counts were already updated when array descriptor was     */
    /* pushed.  Here we only replace it by subobject descriptors, so    */
    /* no update is necessary.                                          */
    new_mark_stack_ptr = GC_push_complex_descriptor(addr, descr, mark_stack_ptr,
                                                    mark_stack_limit - 1);
    if (new_mark_stack_ptr == 0)
    {
        /* Doesn't fit.  Conservatively push the whole array as a unit  */
        /* and request a mark stack expansion.                          */
        /* This cannot cause a mark stack overflow, since it replaces   */
        /* the original array entry.                                    */
        GC_mark_stack_too_small = TRUE;
        new_mark_stack_ptr = orig_mark_stack_ptr + 1;
        new_mark_stack_ptr->mse_start = (ptr_t)addr;
        new_mark_stack_ptr->mse_descr = sz | GC_DS_LENGTH;
    }
    else
    {
        /* Push descriptor itself */
        new_mark_stack_ptr++;
        new_mark_stack_ptr->mse_start = (ptr_t)(addr + nwords - 1);
        new_mark_stack_ptr->mse_descr = sizeof(word) | GC_DS_LENGTH;
    }
    return new_mark_stack_ptr;
}

GC_API GC_descr GC_CALL GC_make_descriptor(GC_bitmap bm, size_t len)
{
    signed_word last_set_bit = len - 1;
    GC_descr result;
    signed_word i;
#define HIGH_BIT (((word)1) << (WORDSZ - 1))

    if (!GC_explicit_typing_initialized)
        GC_init_explicit_typing();
    while (last_set_bit >= 0 && !GC_get_bit(bm, last_set_bit))
        last_set_bit--;
    if (last_set_bit < 0)
        return (0 /* no pointers */);
#if ALIGNMENT == CPP_WORDSZ / 8
    {
        register GC_bool all_bits_set = TRUE;
        for (i = 0; i < last_set_bit; i++)
        {
            if (!GC_get_bit(bm, i))
            {
                all_bits_set = FALSE;
                break;
            }
        }
        if (all_bits_set)
        {
            /* An initial section contains all pointers.  Use length descriptor.
             */
            return (WORDS_TO_BYTES(last_set_bit + 1) | GC_DS_LENGTH);
        }
    }
#endif
    if ((word)last_set_bit < BITMAP_BITS)
    {
        /* Hopefully the common case.                   */
        /* Build bitmap descriptor (with bits reversed) */
        result = HIGH_BIT;
        for (i = last_set_bit - 1; i >= 0; i--)
        {
            result >>= 1;
            if (GC_get_bit(bm, i))
                result |= HIGH_BIT;
        }
        result |= GC_DS_BITMAP;
        return (result);
    }
    else
    {
        signed_word index;

        index = GC_add_ext_descriptor(bm, (word)last_set_bit + 1);
        if (index == -1)
            return (WORDS_TO_BYTES(last_set_bit + 1) | GC_DS_LENGTH);
        /* Out of memory: use conservative      */
        /* approximation.                       */
        result = GC_MAKE_PROC(GC_typed_mark_proc_index, (word)index);
        return result;
    }
}

GC_API void* GC_CALL GC_malloc_explicitly_typed(size_t lb, GC_descr d)
{
    ptr_t op;
    ptr_t* opp;
    size_t lg;
    DCL_LOCK_STATE;

    lb += TYPD_EXTRA_BYTES;
    if (SMALL_OBJ(lb))
    {
        lg = GC_size_map[lb];
        opp = &(GC_eobjfreelist[lg]);
        LOCK();
        if ((op = *opp) == 0)
        {
            UNLOCK();
            op = (ptr_t)GENERAL_MALLOC((word)lb, GC_explicit_kind);
            if (0 == op)
                return 0;
            lg = GC_size_map[lb]; /* May have been uninitialized. */
        }
        else
        {
            *opp = obj_link(op);
            obj_link(op) = 0;
            GC_bytes_allocd += GRANULES_TO_BYTES(lg);
            UNLOCK();
        }
        ((word*)op)[GRANULES_TO_WORDS(lg) - 1] = d;
    }
    else
    {
        op = (ptr_t)GENERAL_MALLOC((word)lb, GC_explicit_kind);
        if (op != NULL)
        {
            lg = BYTES_TO_GRANULES(GC_size(op));
            ((word*)op)[GRANULES_TO_WORDS(lg) - 1] = d;
        }
    }
    return ((void*)op);
}

GC_API void* GC_CALL GC_malloc_explicitly_typed_ignore_off_page(size_t lb,
                                                                GC_descr d)
{
    ptr_t op;
    ptr_t* opp;
    size_t lg;
    DCL_LOCK_STATE;

    lb += TYPD_EXTRA_BYTES;
    if (SMALL_OBJ(lb))
    {
        lg = GC_size_map[lb];
        opp = &(GC_eobjfreelist[lg]);
        LOCK();
        if ((op = *opp) == 0)
        {
            UNLOCK();
            op = (ptr_t)GENERAL_MALLOC_IOP(lb, GC_explicit_kind);
            if (0 == op)
                return 0;
            lg = GC_size_map[lb]; /* May have been uninitialized. */
        }
        else
        {
            *opp = obj_link(op);
            obj_link(op) = 0;
            GC_bytes_allocd += GRANULES_TO_BYTES(lg);
            UNLOCK();
        }
        ((word*)op)[GRANULES_TO_WORDS(lg) - 1] = d;
    }
    else
    {
        op = (ptr_t)GENERAL_MALLOC_IOP(lb, GC_explicit_kind);
        if (op != NULL)
        {
            lg = BYTES_TO_WORDS(GC_size(op));
            ((word*)op)[GRANULES_TO_WORDS(lg) - 1] = d;
        }
    }
    return ((void*)op);
}

GC_API void* GC_CALL GC_calloc_explicitly_typed(size_t n, size_t lb, GC_descr d)
{
    ptr_t op;
    ptr_t* opp;
    size_t lg;
    GC_descr simple_descr;
    complex_descriptor* complex_descr;
    register int descr_type;
    struct LeafDescriptor leaf;
    DCL_LOCK_STATE;

    descr_type = GC_make_array_descriptor((word)n, (word)lb, d, &simple_descr,
                                          &complex_descr, &leaf);
    switch (descr_type)
    {
    case NO_MEM:
        return (0);
    case SIMPLE:
        return (GC_malloc_explicitly_typed(n * lb, simple_descr));
    case LEAF:
        lb *= n;
        lb += sizeof(struct LeafDescriptor) + TYPD_EXTRA_BYTES;
        break;
    case COMPLEX:
        lb *= n;
        lb += TYPD_EXTRA_BYTES;
        break;
    }
    if (SMALL_OBJ(lb))
    {
        lg = GC_size_map[lb];
        opp = &(GC_arobjfreelist[lg]);
        LOCK();
        if ((op = *opp) == 0)
        {
            UNLOCK();
            op = (ptr_t)GENERAL_MALLOC((word)lb, GC_array_kind);
            if (0 == op)
                return (0);
            lg = GC_size_map[lb]; /* May have been uninitialized. */
        }
        else
        {
            *opp = obj_link(op);
            obj_link(op) = 0;
            GC_bytes_allocd += GRANULES_TO_BYTES(lg);
            UNLOCK();
        }
    }
    else
    {
        op = (ptr_t)GENERAL_MALLOC((word)lb, GC_array_kind);
        if (0 == op)
            return (0);
        lg = BYTES_TO_GRANULES(GC_size(op));
    }
    if (descr_type == LEAF)
    {
        /* Set up the descriptor inside the object itself. */
        volatile struct LeafDescriptor* lp =
            (struct LeafDescriptor*)((word*)op + GRANULES_TO_WORDS(lg)
                                     - (BYTES_TO_WORDS(
                                            sizeof(struct LeafDescriptor))
                                        + 1));

        lp->ld_tag = LEAF_TAG;
        lp->ld_size = leaf.ld_size;
        lp->ld_nelements = leaf.ld_nelements;
        lp->ld_descriptor = leaf.ld_descriptor;
        ((volatile word*)op)[GRANULES_TO_WORDS(lg) - 1] = (word)lp;
    }
    else
    {
        size_t lw = GRANULES_TO_WORDS(lg);

        ((word*)op)[lw - 1] = (word)complex_descr;
        /* Make sure the descriptor is cleared once there is any danger  */
        /* it may have been collected.                                   */
        if (GC_general_register_disappearing_link((void**)((word*)op + lw - 1),
                                                  op)
            == GC_NO_MEMORY)
        {
            /* Couldn't register it due to lack of memory.  Punt.        */
            /* This will probably fail too, but gives the recovery code  */
            /* a chance.                                                 */
            return (GC_malloc(n * lb));
        }
    }
    return ((void*)op);
}
