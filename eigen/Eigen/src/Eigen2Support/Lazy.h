// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Benoit Jacob <jacob.benoit.1@gmail.com>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#ifndef EIGEN_LAZY_H
#define EIGEN_LAZY_H

/** \deprecated it is only used by lazy() which is deprecated
 *
 * \returns an expression of *this with added flags
 *
 * Example: \include MatrixBase_marked.cpp
 * Output: \verbinclude MatrixBase_marked.out
 *
 * \sa class Flagged, extract(), part()
 */
template <typename Derived>
template <unsigned int Added>
inline const Flagged<Derived, Added, 0> MatrixBase<Derived>::marked() const
{
    return derived();
}

/** \deprecated use MatrixBase::noalias()
 *
 * \returns an expression of *this with the EvalBeforeAssigningBit flag removed.
 *
 * Example: \include MatrixBase_lazy.cpp
 * Output: \verbinclude MatrixBase_lazy.out
 *
 * \sa class Flagged, marked()
 */
template <typename Derived>
inline const Flagged<Derived, 0, EvalBeforeAssigningBit>
MatrixBase<Derived>::lazy() const
{
    return derived();
}

/** \internal
 * Overloaded to perform an efficient C += (A*B).lazy() */
template <typename Derived>
template <typename ProductDerived, typename Lhs, typename Rhs>
Derived& MatrixBase<Derived>::operator+=(
    const Flagged<ProductBase<ProductDerived, Lhs, Rhs>, 0,
                  EvalBeforeAssigningBit>& other)
{
    other._expression().derived().addTo(derived());
    return derived();
}

/** \internal
 * Overloaded to perform an efficient C -= (A*B).lazy() */
template <typename Derived>
template <typename ProductDerived, typename Lhs, typename Rhs>
Derived& MatrixBase<Derived>::operator-=(
    const Flagged<ProductBase<ProductDerived, Lhs, Rhs>, 0,
                  EvalBeforeAssigningBit>& other)
{
    other._expression().derived().subTo(derived());
    return derived();
}

#endif // EIGEN_LAZY_H
