#ifndef STAN_MATH_PRIM_SCAL_FUN_IS_INTEGER_HPP
#define STAN_MATH_PRIM_SCAL_FUN_IS_INTEGER_HPP

#include <stan/math/prim/scal/fun/is_inf.hpp>
#include <cmath>

namespace stan {
namespace math {

/**
 * Returns true if the input is an integer and false otherwise.
 *
 * @param x Value to test.
 * @return <code>true</code> if the value is an integer
 */
template <typename T>
inline bool is_integer(T x) {
  using std::floor;
  return floor(x) == x && !is_inf(x);
}

}  // namespace math
}  // namespace stan

#endif
