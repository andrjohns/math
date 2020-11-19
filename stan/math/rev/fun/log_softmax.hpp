#ifndef STAN_MATH_REV_FUN_LOG_SOFTMAX_HPP
#define STAN_MATH_REV_FUN_LOG_SOFTMAX_HPP

#include <stan/math/rev/core.hpp>
#include <stan/math/rev/core/typedefs.hpp>
#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <stan/math/prim/fun/log_softmax.hpp>
#include <stan/math/prim/fun/softmax.hpp>
#include <stan/math/prim/fun/to_ref.hpp>
#include <stan/math/prim/fun/typedefs.hpp>
#include <cmath>
#include <vector>

namespace stan {
namespace math {

namespace internal {

class log_softmax_elt_vari : public vari {
 private:
  vari** alpha_;
  const double* softmax_alpha_;
  const int size_;  // array sizes
  const int idx_;   // in in softmax output

 public:
  log_softmax_elt_vari(double val, vari** alpha, const double* softmax_alpha,
                       int size, int idx)
      : vari(val),
        alpha_(alpha),
        softmax_alpha_(softmax_alpha),
        size_(size),
        idx_(idx) {}
  void chain() {
    for (int m = 0; m < size_; ++m) {
      if (m == idx_) {
        alpha_[m]->adj_ += adj_ * (1 - softmax_alpha_[m]);
      } else {
        alpha_[m]->adj_ -= adj_ * softmax_alpha_[m];
      }
    }
  }
};

/**
 * Return the log softmax of the specified vector
 *
 * @tparam T Type of input vector.
 * @param[in] x Unconstrained input vector.
 * @return Softmax of the input.
 * @throw std::domain_error If the input vector is size 0.
 */
template <typename T,
	  require_st_var<T>* = nullptr,
	  require_not_var_matrix_t<T>* = nullptr>
auto log_softmax_impl(const T& alpha) {
  const int a_size = alpha.size();

  check_nonzero_size("log_softmax", "alpha", alpha);

  const auto& alpha_ref = to_ref(alpha);
  
  vari** alpha_vi_array
    = ChainableStack::instance_->memalloc_.alloc_array<vari*>(a_size);
  Eigen::Map<vector_vi>(alpha_vi_array, a_size) = alpha_ref.vi();
  
  vector_d alpha_d = alpha_ref.val();
  
  // fold logic of math::softmax() and math::log_softmax()
  // to save computations
  
  vector_d diff = (alpha_d.array() - alpha_d.maxCoeff());
  vector_d softmax_alpha_d = diff.array().exp();
  double sum = softmax_alpha_d.sum();
  vector_d log_softmax_alpha_d = diff.array() - std::log(sum);
  
  // end fold
  double* softmax_alpha_d_array
    = ChainableStack::instance_->memalloc_.alloc_array<double>(a_size);
  Eigen::Map<vector_d>(softmax_alpha_d_array, a_size)
    = softmax_alpha_d.array() / sum;
  
  vector_v log_softmax_alpha(a_size);
  for (int k = 0; k < a_size; ++k) {
    log_softmax_alpha(k) = var(new internal::log_softmax_elt_vari(log_softmax_alpha_d[k], alpha_vi_array, softmax_alpha_d_array,
								  a_size, k));
  }
  return log_softmax_alpha;
}

/**
 * Return the log softmax of the specified vector
 *
 * @tparam T Type of input vector.
 * @param[in] x Unconstrained input vector.
 * @return Softmax of the input.
 * @throw std::domain_error If the input vector is size 0.
 */
template <typename T,
	  require_var_matrix_t<T>* = nullptr>
T log_softmax_impl(const T& alpha) {
  check_nonzero_size("log_softmax", "alpha", alpha);

  const auto& theta = to_ref(alpha.val().array() - alpha.val().maxCoeff());

  return make_callback_vari((theta.array() - log(theta.exp().sum())).matrix(),
			    [alpha](const auto& res) mutable {
    alpha.adj().noalias()
      += res.adj_
      - (res.adj_.sum() * res.val_.array().exp()).matrix();
  });
}
}  // namespace internal

/**
 * Return the log softmax of the specified vector or container of vectors.
 *
 * The gradient calculations are unfolded.
 *
 * @tparam T Type of input vector or matrix.
 * @param[in] x Unconstrained input vector.
 * @return Softmax of the input.
 * @throw std::domain_error If the input vector is size 0.
 */
template <typename T, require_container_st<is_var, T>* = nullptr>
inline auto log_softmax(const T& x) {
  return apply_vector_unary<T>::apply(x, [](const auto& alpha) {
    return internal::log_softmax_impl(alpha);
  });
}

}  // namespace math
}  // namespace stan
#endif
