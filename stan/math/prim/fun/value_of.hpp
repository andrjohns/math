#ifndef STAN_MATH_PRIM_FUN_VALUE_OF_HPP
#define STAN_MATH_PRIM_FUN_VALUE_OF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <cstddef>
#include <vector>

namespace stan {
namespace math {

/**
 * Inputs that are arithmetic types or containers of airthmetric types
 * are returned from value_of unchanged
 *
 * @tparam T Input type
 * @param[in] x Input argument
 * @return Forwarded input argument
 **/
template <typename T,
	  require_arithmetic_t<stan::return_type_t<T>>* = nullptr>
inline decltype(auto) value_of(T&& x) {
  return std::forward<decltype(x)>(x);
}

/**
 * For std::vectors of non-arithmetic types, return a std::vector composed
 * of value_of applied to each element.
 *
 * @tparam T Input element type
 * @param[in] x Input std::vector
 * @return std::vector of values
 **/
template <typename T,
	  require_not_arithmetic_t<stan::return_type_t<T>>* = nullptr>
inline auto value_of(const std::vector<T>& x) {
  std::vector<std::decay_t<decltype(value_of(std::declval<T>()))>> out;
  out.reserve(x.size());
  for (size_t n = 0; n < x.size(); ++n)
    out.emplace_back(value_of(x[n]));
  return out;
}

/**
 * For Eigen matrices and expressions of non-arithmetic types, return an expression
 * that represents the Eigen::Matrix resulting from applying value_of elementwise
 *
 * @tparam EigMat type of the matrix
 *
 * @param[in] M Matrix to be converted
 * @return Matrix of values
 **/
template <typename EigMat,
	  require_eigen_t<EigMat>* = nullptr,
	  require_not_arithmetic_t<stan::return_type_t<EigMat>>* = nullptr>
inline auto value_of(EigMat&& M) {
  /*return make_holder(
      [](auto& a) {
        return a.unaryExpr([](const auto& scal) { return value_of(scal); });
      },
      std::forward<EigMat>(M));*/
  return M.unaryExpr([](const auto& scal) { return value_of(scal); }).eval();
}

}  // namespace math
}  // namespace stan

#endif
