/* unzip.h -- IO for uncompress .zip files using zlib
   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant

   This unzip package allow extract file from .ZIP file, compatible with
  PKZip 2.04g WinZip, InfoZip tools and compatible.

   Multi volume ZipFile (span) are not supported.
   Encryption compatible with pkzip 2.04g only supported
   Old compressions used by old PKZip 1.x are not supported


   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/

/* for more info about .ZIP format, see
      http://www.info-zip.org/pub/infozip/doc/appnote-981119-iz.zip
      http://www.info-zip.org/pub/infozip/doc/
   PkWare has also a specification at :
      ftp://ftp.pkware.com/probdesc.zip
*/

#ifndef _unz_H
#define _unz_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _ZLIB_H
#include "zlib.h"
#endif

#ifndef _ZLIBIOAPI_H
#include "ioapi.h"
#endif

#ifdef MINIZ_DLL
#ifdef MINIZ_INTERNAL
#define MINIZ_EXTERN extern __declspec(dllexport)
#else
#define MINIZ_EXTERN extern __declspec(dllimport)
#endif
#else
#define MINIZ_EXTERN
#endif

#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
    /* like the STRICT of WIN32, we define a pointer that cannot be converted
        from (void*) without cast */
    typedef struct TagunzFile__
    {
        int unused;
    } unzFile__;

    typedef unzFile__* unzFile;
#else
typedef voidp unzFile;
#endif

#define UNZ_OK (0)
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO (Z_ERRNO)
#define UNZ_EOF (0)
#define UNZ_PARAMERROR (-102)
#define UNZ_BADZIPFILE (-103)
#define UNZ_INTERNALERROR (-104)
#define UNZ_CRCERROR (-105)

    /* tm_unz contain date/time info */
    typedef struct tm_unz_s
    {
        uInt tm_sec;  /* seconds after the minute - [0,59] */
        uInt tm_min;  /* minutes after the hour - [0,59] */
        uInt tm_hour; /* hours since midnight - [0,23] */
        uInt tm_mday; /* day of the month - [1,31] */
        uInt tm_mon;  /* months since January - [0,11] */
        uInt tm_year; /* years - [1980..2044] */
    } tm_unz;

    /* unz_global_info structure contain global data about the ZIPfile
       These data comes from the end of central dir */
    typedef struct unz_global_info_s
    {
        uLong number_entry; /* total number of entries in
                   the central dir on this disk */
        uLong size_comment; /* size of the global comment of the zipfile */
    } unz_global_info;

    /* unz_file_info contain information about a file in the zipfile */
    typedef struct unz_file_info_s
    {
        uLong version;            /* version made by                 2 bytes */
        uLong version_needed;     /* version needed to extract       2 bytes */
        uLong flag;               /* general purpose bit flag        2 bytes */
        uLong compression_method; /* compression method              2 bytes */
        uLong dosDate;            /* last mod file date in Dos fmt   4 bytes */
        uLong crc;                /* crc-32                          4 bytes */
        uLong compressed_size;    /* compressed size                 4 bytes */
        uLong uncompressed_size;  /* uncompressed size               4 bytes */
        uLong size_filename;      /* filename length                 2 bytes */
        uLong size_file_extra;    /* extra field length              2 bytes */
        uLong size_file_comment;  /* file comment length             2 bytes */

        uLong disk_num_start; /* disk number start               2 bytes */
        uLong internal_fa;    /* internal file attributes        2 bytes */
        uLong external_fa;    /* external file attributes        4 bytes */

        tm_unz tmu_date;
    } unz_file_info;

    MINIZ_EXTERN int unzStringFileNameCompare OF((const char* fileName1,
                                                  const char* fileName2,
                                                  int iCaseSensitivity));
    /*
       Compare two filename (fileName1,fileName2).
       If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
       If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
                                    or strcasecmp)
       If iCaseSenisivity = 0, case sensitivity is defaut of your operating
       system (like 1 on Unix, 2 on Windows)
    */

    MINIZ_EXTERN unzFile unzOpen OF((const char* path));
    /*
      Open a Zip file. path contain the full pathname (by example,
         on a Windows XP computer "c:\\zlib\\zlib113.zip" or on an Unix computer
         "zlib/zlib113.zip".
         If the zipfile cannot be opened (file don't exist or in not valid), the
           return value is NULL.
         Else, the return value is a unzFile Handle, usable with other function
           of this unzip package.
    */

    MINIZ_EXTERN unzFile unzOpen2 OF((const char* path,
                                      zlib_filefunc_def* pzlib_filefunc_def));
    /*
       Open a Zip file, like unzOpen, but provide a set of file low level API
          for read/write the zip file (see ioapi.h)
    */

    MINIZ_EXTERN int unzClose OF((unzFile file));
    /*
      Close a ZipFile opened with unzipOpen.
      If there is files inside the .Zip opened with unzOpenCurrentFile (see
      later), these files MUST be closed with unzipCloseCurrentFile before call
      unzipClose. return UNZ_OK if there is no problem. */

    MINIZ_EXTERN int unzGetGlobalInfo OF((unzFile file,
                                          unz_global_info* pglobal_info));
    /*
      Write info about the ZipFile in the *pglobal_info structure.
      No preparation of the structure is needed
      return UNZ_OK if there is no problem. */

    MINIZ_EXTERN int unzGetGlobalComment OF((unzFile file, char* szComment,
                                             uLong uSizeBuf));
    /*
      Get the global comment string of the ZipFile, in the szComment buffer.
      uSizeBuf is the size of the szComment buffer.
      return the number of byte copied or an error code <0
    */

    /***************************************************************************/
    /* Unzip package allow you browse the directory of the zipfile */

    MINIZ_EXTERN int unzGoToFirstFile OF((unzFile file));
    /*
      Set the current file of the zipfile to the first file.
      return UNZ_OK if there is no problem
    */

    MINIZ_EXTERN int unzGoToNextFile OF((unzFile file));
    /*
      Set the current file of the zipfile to the next file.
      return UNZ_OK if there is no problem
      return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
    */

    MINIZ_EXTERN int unzLocateFile OF((unzFile file, const char* szFileName,
                                       int iCaseSensitivity));

    /*
      Try locate the file szFileName in the zipfile.
      For the iCaseSensitivity signification, see unzStringFileNameCompare

      return value :
      UNZ_OK if the file is found. It becomes the current file.
      UNZ_END_OF_LIST_OF_FILE if the file is not found
    */

    /* ****************************************** */
    /* Ryan supplied functions */
    /* unz_file_info contain information about a file in the zipfile */
    typedef struct unz_file_pos_s
    {
        uLong pos_in_zip_directory; /* offset in zip file directory */
        uLong num_of_file;          /* # of file */
    } unz_file_pos;

    MINIZ_EXTERN int unzGetFilePos(unzFile file, unz_file_pos* file_pos);

    MINIZ_EXTERN int unzGoToFilePos(unzFile file, unz_file_pos* file_pos);

    /* ****************************************** */

    MINIZ_EXTERN int unzGetCurrentFileInfo OF(
        (unzFile file, unz_file_info* pfile_info, char* szFileName,
         uLong fileNameBufferSize, void* extraField, uLong extraFieldBufferSize,
         char* szComment, uLong commentBufferSize));
    /*
      Get Info about the current file
      if pfile_info!=NULL, the *pfile_info structure will contain somes info
      about the current file if szFileName!=NULL, the filemane string will be
      copied in szFileName (fileNameBufferSize is the size of the buffer) if
      extraField!=NULL, the extra field information will be copied in extraField
                (extraFieldBufferSize is the size of the buffer).
                This is the Central-header version of the extra field
      if szComment!=NULL, the comment string of the file will be copied in
      szComment (commentBufferSize is the size of the buffer)
    */

    /***************************************************************************/
    /* for reading the content of the current zipfile, you can open it, read
       data from it, and close it (you can close it before reading all the file)
       */

    MINIZ_EXTERN int unzOpenCurrentFile OF((unzFile file));
    /*
      Open for reading data the current file in the zipfile.
      If there is no error, the return value is UNZ_OK.
    */

    MINIZ_EXTERN int unzOpenCurrentFilePassword OF((unzFile file,
                                                    const char* password));
    /*
      Open for reading data the current file in the zipfile.
      password is a crypting password
      If there is no error, the return value is UNZ_OK.
    */

    MINIZ_EXTERN int unzOpenCurrentFile2 OF((unzFile file, int* method,
                                             int* level, int raw));
    /*
      Same than unzOpenCurrentFile, but open for read raw the file (not
      uncompress) if raw==1 *method will receive method of compression, *level
      will receive level of compression note : you can set level parameter as
      NULL (if you did not want known level, but you CANNOT set method parameter
      as NULL
    */

    MINIZ_EXTERN int unzOpenCurrentFile3 OF((unzFile file, int* method,
                                             int* level, int raw,
                                             const char* password));
    /*
      Same than unzOpenCurrentFile, but open for read raw the file (not
      uncompress) if raw==1 *method will receive method of compression, *level
      will receive level of compression note : you can set level parameter as
      NULL (if you did not want known level, but you CANNOT set method parameter
      as NULL
    */

    MINIZ_EXTERN int unzCloseCurrentFile OF((unzFile file));
    /*
      Close the file in zip opened with unzOpenCurrentFile
      Return UNZ_CRCERROR if all the file was read but the CRC is not good
    */

    MINIZ_EXTERN int unzReadCurrentFile OF((unzFile file, voidp buf,
                                            unsigned len));
    /*
      Read bytes from the current file (opened by unzOpenCurrentFile)
      buf contain buffer where data must be copied
      len the size of buf.

      return the number of byte copied if somes bytes are copied
      return 0 if the end of file was reached
      return <0 with error code if there is an error
        (UNZ_ERRNO for IO error, or zLib error for uncompress error)
    */

    MINIZ_EXTERN z_off_t unztell OF((unzFile file));
    /*
      Give the current position in uncompressed data
    */

    MINIZ_EXTERN int unzeof OF((unzFile file));
    /*
      return 1 if the end of file was reached, 0 elsewhere
    */

    MINIZ_EXTERN int unzGetLocalExtrafield OF((unzFile file, voidp buf,
                                               unsigned len));
    /*
      Read extra field from the current file (opened by unzOpenCurrentFile)
      This is the local-header version of the extra field (sometimes, there is
        more info in the local-header version than in the central-header)

      if buf==NULL, it return the size of the local extra field

      if buf!=NULL, len is the size of the buffer, the extra header is copied in
        buf.
      the return value is the number of bytes copied in buf, or (if <0)
        the error code
    */

    /***************************************************************************/

    /* Get the current file offset */
    MINIZ_EXTERN uLong unzGetOffset(unzFile file);

    /* Set the current file offset */
    MINIZ_EXTERN int unzSetOffset(unzFile file, uLong pos);

#ifdef __cplusplus
}
#endif

#endif /* _unz_H */
