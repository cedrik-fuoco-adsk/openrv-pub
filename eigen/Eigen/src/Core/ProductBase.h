// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009-2010 Gael Guennebaud <gael.guennebaud@inria.fr>
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

#ifndef EIGEN_PRODUCTBASE_H
#define EIGEN_PRODUCTBASE_H

/** \class ProductBase
 * \ingroup Core_Module
 *
 */

namespace internal
{
    template <typename Derived, typename _Lhs, typename _Rhs>
    struct traits<ProductBase<Derived, _Lhs, _Rhs>>
    {
        typedef MatrixXpr XprKind;
        typedef typename remove_all<_Lhs>::type Lhs;
        typedef typename remove_all<_Rhs>::type Rhs;
        typedef typename scalar_product_traits<
            typename Lhs::Scalar, typename Rhs::Scalar>::ReturnType Scalar;
        typedef typename promote_storage_type<
            typename traits<Lhs>::StorageKind,
            typename traits<Rhs>::StorageKind>::ret StorageKind;
        typedef typename promote_index_type<typename traits<Lhs>::Index,
                                            typename traits<Rhs>::Index>::type
            Index;

        enum
        {
            RowsAtCompileTime = traits<Lhs>::RowsAtCompileTime,
            ColsAtCompileTime = traits<Rhs>::ColsAtCompileTime,
            MaxRowsAtCompileTime = traits<Lhs>::MaxRowsAtCompileTime,
            MaxColsAtCompileTime = traits<Rhs>::MaxColsAtCompileTime,
            Flags = (MaxRowsAtCompileTime == 1 ? RowMajorBit : 0)
                    | EvalBeforeNestingBit | EvalBeforeAssigningBit
                    | NestByRefBit,
            // Note that EvalBeforeNestingBit and NestByRefBit
            // are not used in practice because nested is overloaded for
            // products
            CoeffReadCost = 0 // FIXME why is it needed ?
        };
    };
} // namespace internal

#define EIGEN_PRODUCT_PUBLIC_INTERFACE(Derived)           \
    typedef ProductBase<Derived, Lhs, Rhs> Base;          \
    EIGEN_DENSE_PUBLIC_INTERFACE(Derived)                 \
    typedef typename Base::LhsNested LhsNested;           \
    typedef typename Base::_LhsNested _LhsNested;         \
    typedef typename Base::LhsBlasTraits LhsBlasTraits;   \
    typedef typename Base::ActualLhsType ActualLhsType;   \
    typedef typename Base::_ActualLhsType _ActualLhsType; \
    typedef typename Base::RhsNested RhsNested;           \
    typedef typename Base::_RhsNested _RhsNested;         \
    typedef typename Base::RhsBlasTraits RhsBlasTraits;   \
    typedef typename Base::ActualRhsType ActualRhsType;   \
    typedef typename Base::_ActualRhsType _ActualRhsType; \
    using Base::m_lhs;                                    \
    using Base::m_rhs;

template <typename Derived, typename Lhs, typename Rhs>
class ProductBase : public MatrixBase<Derived>
{
public:
    typedef MatrixBase<Derived> Base;
    EIGEN_DENSE_PUBLIC_INTERFACE(ProductBase)

    typedef typename Lhs::Nested LhsNested;
    typedef typename internal::remove_all<LhsNested>::type _LhsNested;
    typedef internal::blas_traits<_LhsNested> LhsBlasTraits;
    typedef typename LhsBlasTraits::DirectLinearAccessType ActualLhsType;
    typedef typename internal::remove_all<ActualLhsType>::type _ActualLhsType;
    typedef typename internal::traits<Lhs>::Scalar LhsScalar;

    typedef typename Rhs::Nested RhsNested;
    typedef typename internal::remove_all<RhsNested>::type _RhsNested;
    typedef internal::blas_traits<_RhsNested> RhsBlasTraits;
    typedef typename RhsBlasTraits::DirectLinearAccessType ActualRhsType;
    typedef typename internal::remove_all<ActualRhsType>::type _ActualRhsType;
    typedef typename internal::traits<Rhs>::Scalar RhsScalar;

    // Diagonal of a product: no need to evaluate the arguments because they are
    // going to be evaluated only once
    typedef CoeffBasedProduct<LhsNested, RhsNested, 0>
        FullyLazyCoeffBaseProductType;

public:
    typedef typename Base::PlainObject PlainObject;

    ProductBase(const Lhs& lhs, const Rhs& rhs)
        : m_lhs(lhs)
        , m_rhs(rhs)
    {
        eigen_assert(lhs.cols() == rhs.rows() && "invalid matrix product"
                     && "if you wanted a coeff-wise or a dot product use the "
                        "respective explicit functions");
    }

    inline Index rows() const { return m_lhs.rows(); }

    inline Index cols() const { return m_rhs.cols(); }

    template <typename Dest> inline void evalTo(Dest& dst) const
    {
        dst.setZero();
        scaleAndAddTo(dst, Scalar(1));
    }

    template <typename Dest> inline void addTo(Dest& dst) const
    {
        scaleAndAddTo(dst, 1);
    }

    template <typename Dest> inline void subTo(Dest& dst) const
    {
        scaleAndAddTo(dst, -1);
    }

    template <typename Dest>
    inline void scaleAndAddTo(Dest& dst, Scalar alpha) const
    {
        derived().scaleAndAddTo(dst, alpha);
    }

    const _LhsNested& lhs() const { return m_lhs; }

    const _RhsNested& rhs() const { return m_rhs; }

    // Implicit conversion to the nested type (trigger the evaluation of the
    // product)
    operator const PlainObject&() const
    {
        m_result.resize(m_lhs.rows(), m_rhs.cols());
        derived().evalTo(m_result);
        return m_result;
    }

    const Diagonal<const FullyLazyCoeffBaseProductType, 0> diagonal() const
    {
        return FullyLazyCoeffBaseProductType(m_lhs, m_rhs);
    }

    template <int Index>
    const Diagonal<FullyLazyCoeffBaseProductType, Index> diagonal() const
    {
        return FullyLazyCoeffBaseProductType(m_lhs, m_rhs);
    }

    const Diagonal<FullyLazyCoeffBaseProductType, Dynamic>
    diagonal(Index index) const
    {
        return FullyLazyCoeffBaseProductType(m_lhs, m_rhs).diagonal(index);
    }

    // restrict coeff accessors to 1x1 expressions. No need to care about
    // mutators here since this isnt a Lvalue expression
    typename Base::CoeffReturnType coeff(Index row, Index col) const
    {
#ifdef EIGEN2_SUPPORT
        return lhs().row(row).cwiseProduct(rhs().col(col).transpose()).sum();
#else
        EIGEN_STATIC_ASSERT_SIZE_1x1(Derived)
            eigen_assert(this->rows() == 1 && this->cols() == 1);
        return derived().coeff(row, col);
#endif
    }

    typename Base::CoeffReturnType coeff(Index i) const
    {
        EIGEN_STATIC_ASSERT_SIZE_1x1(Derived)
            eigen_assert(this->rows() == 1 && this->cols() == 1);
        return derived().coeff(i);
    }

    const Scalar& coeffRef(Index row, Index col) const
    {
        EIGEN_STATIC_ASSERT_SIZE_1x1(Derived)
            eigen_assert(this->rows() == 1 && this->cols() == 1);
        return derived().coeffRef(row, col);
    }

    const Scalar& coeffRef(Index i) const
    {
        EIGEN_STATIC_ASSERT_SIZE_1x1(Derived)
            eigen_assert(this->rows() == 1 && this->cols() == 1);
        return derived().coeffRef(i);
    }

protected:
    const LhsNested m_lhs;
    const RhsNested m_rhs;

    mutable PlainObject m_result;
};

// here we need to overload the nested rule for products
// such that the nested type is a const reference to a plain matrix
namespace internal
{
    template <typename Lhs, typename Rhs, int Mode, int N, typename PlainObject>
    struct nested<GeneralProduct<Lhs, Rhs, Mode>, N, PlainObject>
    {
        typedef PlainObject const& type;
    };
} // namespace internal

template <typename NestedProduct> class ScaledProduct;

// Note that these two operator* functions are not defined as member
// functions of ProductBase, because, otherwise we would have to
// define all overloads defined in MatrixBase. Furthermore, Using
// "using Base::operator*" would not work with MSVC.
//
// Also note that here we accept any compatible scalar types
template <typename Derived, typename Lhs, typename Rhs>
const ScaledProduct<Derived>
operator*(const ProductBase<Derived, Lhs, Rhs>& prod,
          typename Derived::Scalar x)
{
    return ScaledProduct<Derived>(prod.derived(), x);
}

template <typename Derived, typename Lhs, typename Rhs>
typename internal::enable_if<
    !internal::is_same<typename Derived::Scalar,
                       typename Derived::RealScalar>::value,
    const ScaledProduct<Derived>>::type
operator*(const ProductBase<Derived, Lhs, Rhs>& prod,
          typename Derived::RealScalar x)
{
    return ScaledProduct<Derived>(prod.derived(), x);
}

template <typename Derived, typename Lhs, typename Rhs>
const ScaledProduct<Derived>
operator*(typename Derived::Scalar x,
          const ProductBase<Derived, Lhs, Rhs>& prod)
{
    return ScaledProduct<Derived>(prod.derived(), x);
}

template <typename Derived, typename Lhs, typename Rhs>
typename internal::enable_if<
    !internal::is_same<typename Derived::Scalar,
                       typename Derived::RealScalar>::value,
    const ScaledProduct<Derived>>::type
operator*(typename Derived::RealScalar x,
          const ProductBase<Derived, Lhs, Rhs>& prod)
{
    return ScaledProduct<Derived>(prod.derived(), x);
}

namespace internal
{
    template <typename NestedProduct>
    struct traits<ScaledProduct<NestedProduct>>
        : traits<ProductBase<ScaledProduct<NestedProduct>,
                             typename NestedProduct::_LhsNested,
                             typename NestedProduct::_RhsNested>>
    {
        typedef typename traits<NestedProduct>::StorageKind StorageKind;
    };
} // namespace internal

template <typename NestedProduct>
class ScaledProduct
    : public ProductBase<ScaledProduct<NestedProduct>,
                         typename NestedProduct::_LhsNested,
                         typename NestedProduct::_RhsNested>
{
public:
    typedef ProductBase<ScaledProduct<NestedProduct>,
                        typename NestedProduct::_LhsNested,
                        typename NestedProduct::_RhsNested>
        Base;
    typedef typename Base::Scalar Scalar;
    typedef typename Base::PlainObject PlainObject;

    //     EIGEN_PRODUCT_PUBLIC_INTERFACE(ScaledProduct)

    ScaledProduct(const NestedProduct& prod, Scalar x)
        : Base(prod.lhs(), prod.rhs())
        , m_prod(prod)
        , m_alpha(x)
    {
    }

    template <typename Dest> inline void evalTo(Dest& dst) const
    {
        dst.setZero();
        scaleAndAddTo(dst, m_alpha);
    }

    template <typename Dest> inline void addTo(Dest& dst) const
    {
        scaleAndAddTo(dst, m_alpha);
    }

    template <typename Dest> inline void subTo(Dest& dst) const
    {
        scaleAndAddTo(dst, -m_alpha);
    }

    template <typename Dest>
    inline void scaleAndAddTo(Dest& dst, Scalar alpha) const
    {
        m_prod.derived().scaleAndAddTo(dst, alpha);
    }

    const Scalar& alpha() const { return m_alpha; }

protected:
    const NestedProduct& m_prod;
    Scalar m_alpha;
};

/** \internal
 * Overloaded to perform an efficient C = (A*B).lazy() */
template <typename Derived>
template <typename ProductDerived, typename Lhs, typename Rhs>
Derived& MatrixBase<Derived>::lazyAssign(
    const ProductBase<ProductDerived, Lhs, Rhs>& other)
{
    other.derived().evalTo(derived());
    return derived();
}

#endif // EIGEN_PRODUCTBASE_H
