#ifndef STAN_MATH_PRIM_FUN_LOG10_HPP
#define STAN_MATH_PRIM_FUN_LOG10_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <cmath>

namespace stan {
namespace math {

/**
 * Structure to wrap log10() so it can be vectorized.
 *
 * @tparam T type of variable
 * @param x variable
 * @return Log base-10 of x.
 */
struct log10_fun {
  template <typename T>
  static inline T fun(const T& x) {
    using std::log10;
    return log10(x);
  }
};

/**
 * Vectorized version of log10().
 *
 * @tparam T type of container
 * @param x container
 * @return Log base-10 applied to each value in x.
 */
template <typename T,
          require_not_container_st<is_container, std::is_arithmetic, T>...>
inline auto log10(const T& x) {
  return apply_scalar_unary<log10_fun, T>::apply(x);
}

/**
 * Version of log10() that accepts Eigen Matrix or matrix expressions.
 *
 * @tparam Derived derived type of x
 * @param x Matrix or matrix expression
 * @return Arc cosine of each variable in the container, in radians.
 */
template <typename T,
          require_container_st<is_container, std::is_arithmetic, T>...>
inline auto log10(const T& x) {
  return apply_vector_unary<T>::apply(x, [&](const auto& v) {
    return v.derived().array().log10();
  });
}


}  // namespace math
}  // namespace stan

#endif
