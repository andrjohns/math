#ifndef STAN_MATH_PRIM_FUN_FABS_HPP
#define STAN_MATH_PRIM_FUN_FABS_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <stan/math/prim/fun/match_wrapper.hpp>
#include <cmath>

namespace stan {
namespace math {

/**
 * Structure to wrap fabs() so that it can be vectorized.
 *
 * @tparam T type of variable
 * @param x variable
 * @return Absolute value of x.
 */
struct fabs_fun {
  template <typename T>
  static inline T fun(const T& x) {
    using std::fabs;
    return fabs(x);
  }
};

/**
 * Vectorized version of fabs().
 *
 * @tparam T type of container
 * @param x container
 * @return Absolute value of each value in x.
 */
template <typename T,
          require_not_container_st<is_container, std::is_arithmetic, T>...>
inline auto fabs(const T& x) {
  return apply_scalar_unary<fabs_fun, T>::apply(x);
}

/**
 * Version of fabs() that accepts Eigen Matrix or matrix expressions.
 *
 * @tparam Derived derived type of x
 * @param x Matrix or matrix expression
 * @return Absolute value of each value in x.
 */
template <typename T,
          require_container_st<is_container, std::is_arithmetic, T>...>
inline auto fabs(const T& x) {
  return apply_vector_unary<T>::apply(x, [&](const auto& v) {
    return v.derived().array().abs();
  });
}

}  // namespace math
}  // namespace stan

#endif
