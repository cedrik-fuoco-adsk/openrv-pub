/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote
products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are
disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any
direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef _CV_GEOM_H_
#define _CV_GEOM_H_

/* Finds distance between two points */
CV_INLINE float icvDistanceL2_32f(CvPoint2D32f pt1, CvPoint2D32f pt2)
{
    float dx = pt2.x - pt1.x;
    float dy = pt2.y - pt1.y;

    return cvSqrt(dx * dx + dy * dy);
}

int icvIntersectLines(double x1, double dx1, double y1, double dy1, double x2,
                      double dx2, double y2, double dy2, double* t2);

void icvCreateCenterNormalLine(CvSubdiv2DEdge edge, double* a, double* b,
                               double* c);

void icvIntersectLines3(double* a0, double* b0, double* c0, double* a1,
                        double* b1, double* c1, CvPoint2D32f* point);

#define _CV_BINTREE_LIST()                                                     \
    struct _CvTrianAttr* prev_v;  /* pointer to the parent  element on the     \
                                     previous level of the tree  */            \
    struct _CvTrianAttr* next_v1; /* pointer to the child  element on the next \
                                     level of the tree  */                     \
    struct _CvTrianAttr* next_v2; /* pointer to the child  element on the next \
                                     level of the tree  */

typedef struct _CvTrianAttr
{
    CvPoint pt;  /* Coordinates x and y of the vertex  which don't lie on the
                    base line LMIAT  */
    char sign;   /*  sign of the triangle   */
    double area; /*   area of the triangle    */
    double r1;   /*  The ratio of the height of triangle to the base of the
                    triangle  */
    double r2; /*   The ratio of the projection of the left side of the triangle
                  on the base to the base */
    _CV_BINTREE_LIST() /* structure double list   */
} _CvTrianAttr;

/* curvature: 0 - 1-curvature, 1 - k-cosine curvature. */
CvStatus icvApproximateChainTC89(CvChain* chain, int header_size,
                                 CvMemStorage* storage, CvSeq** contour,
                                 int method);

#endif /*_IPCVGEOM_H_*/

/* End of file. */
