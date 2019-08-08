#include <stan/math/fwd/mat.hpp>
#include <test/unit/math/prim/mat/fun/expect_matrix_eq.hpp>
#include <gtest/gtest.h>

TEST(AgradFwdMatrixAppendCol, fd) {
  using Eigen::MatrixXd;
  using stan::math::append_col;
  using stan::math::matrix_fd;

  matrix_fd a(2, 2);
  MatrixXd ad(2, 2);
  MatrixXd b(2, 2);

  a << 2.0, 3.0, 9.0, -1.0;
  a.d() << 2.0, 3.0, 4.0, 5.0;
  ad << 2.0, 3.0, 9.0, -1.0;
  b << 4.0, 3.0, 0.0, 1.0;

  matrix_fd ab_append_col = append_col(a, b);
  MatrixXd adb_append_col = append_col(ad, b);

  expect_matrix_eq(a.d(), ab_append_col.d().block(0,0,2,2));
  expect_matrix_eq(ab_append_col.val(), adb_append_col);
}

TEST(AgradFwdRowVectorAppendCol, fd) {
  using Eigen::RowVectorXd;
  using stan::math::append_col;
  using stan::math::row_vector_fd;

  row_vector_fd a(4);
  RowVectorXd ad(4);
  RowVectorXd b(3);

  a << 2.0, 3.0, 9.0, -1.0;
  a.d() << 2.0, 3.0, 4.0, 5.0;
  ad << 2.0, 3.0, 9.0, -1.0;
  b << 4.0, 3.0, 0.4;

  row_vector_fd ab_append_col = append_col(a, b);
  RowVectorXd adb_append_col = append_col(ad, b);

  expect_matrix_eq(a.d(), ab_append_col.d().block(0,0,1,4));
  expect_matrix_eq(ab_append_col.val(), adb_append_col);
}

TEST(AgradFwdMatrixAppendCol, ffd) {
  using Eigen::MatrixXd;
  using stan::math::append_col;
  using stan::math::matrix_ffd;

  matrix_ffd a(2, 2);
  MatrixXd ad(2, 2);
  MatrixXd b(2, 2);

  a << 2.0, 3.0, 9.0, -1.0;
  a.d() << 2.0, 3.0, 4.0, 5.0;
  ad << 2.0, 3.0, 9.0, -1.0;
  b << 4.0, 3.0, 0.0, 1.0;

  matrix_ffd ab_append_col = append_col(a, b);
  MatrixXd adb_append_col = append_col(ad, b);

  expect_matrix_eq(a.d().val(), ab_append_col.d().val().block(0,0,2,2));
  expect_matrix_eq(ab_append_col.val().val(), adb_append_col);
}

TEST(AgradFwdRowVectorAppendCol, ffd) {
  using Eigen::RowVectorXd;
  using stan::math::append_col;
  using stan::math::row_vector_ffd;

  row_vector_ffd a(4);
  RowVectorXd ad(4);
  RowVectorXd b(3);

  a << 2.0, 3.0, 9.0, -1.0;
  a.d() << 2.0, 3.0, 4.0, 5.0;
  ad << 2.0, 3.0, 9.0, -1.0;
  b << 4.0, 3.0, 0.4;

  row_vector_ffd ab_append_col = append_col(a, b);
  RowVectorXd adb_append_col = append_col(ad, b);

  expect_matrix_eq(a.d().val(), ab_append_col.d().val().block(0,0,1,4));
  expect_matrix_eq(ab_append_col.val().val(), adb_append_col);
}
