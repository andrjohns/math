#ifndef STAN_MATH_PRIM_FUN_SCALBN_HPP
#define STAN_MATH_PRIM_FUN_SCALBN_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/value_of_rec.hpp>
#include <stan/math/prim/functor/apply_scalar_binary.hpp>
#include <cmath>

namespace stan {
namespace math {

template <typename T, typename = require_autodiff_t<T>>
double scalbn(const T& x, int n) {
  return std::scalbn(value_of_rec(x), n);
}

/**
 * Enables the vectorised application of the scalbn
 * function, when the first and/or second arguments are containers.
 *
 * @tparam T1 type of first input
 * @tparam T2 type of second input
 * @param a First input
 * @param b Second input
 * @return scalbn function applied to the two inputs.
 */
template <typename T1, typename T2, require_any_container_t<T1, T2>* = nullptr>
inline auto scalbn(const T1& a, const T2& b) {
  return apply_scalar_binary(
      a, b, [&](const auto& c, const auto& d) { return scalbn(c, d); });
}

}  // namespace math
}  // namespace stan

#endif
