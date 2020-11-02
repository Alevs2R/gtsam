/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010-2019, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * testPowerMethod.cpp
 *
 * @file   testPowerMethod.cpp
 * @date   Sept 2020
 * @author Jing Wu
 * @brief  Check eigenvalue and eigenvector computed by power method
 */

#include <CppUnitLite/TestHarness.h>
#include <gtsam/base/Matrix.h>
#include <gtsam/base/VectorSpace.h>
#include <gtsam/inference/Symbol.h>
#include <gtsam/linear/GaussianFactorGraph.h>
#include <gtsam/linear/PowerMethod.h>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <iostream>
#include <random>

using namespace std;
using namespace gtsam;
using symbol_shorthand::X;

/* ************************************************************************* */
TEST(PowerMethod, powerIteration) {
  // test power iteration, beta is set to 0
  Sparse A(6, 6);
  A.coeffRef(0, 0) = 6;
  A.coeffRef(1, 1) = 5;
  A.coeffRef(2, 2) = 4;
  A.coeffRef(3, 3) = 3;
  A.coeffRef(4, 4) = 2;
  A.coeffRef(5, 5) = 1;
  Vector initial = (Vector(6) << 0.24434602, 0.22829942, 0.70094486, 0.15463092, 0.55871359,
       0.2465342).finished();
  PowerMethod<Sparse> pf(A, initial);
  pf.compute(100, 1e-5);
  EXPECT_LONGS_EQUAL(6, pf.eigenvector().rows());

  Vector6 actual1 = pf.eigenvector();
  const double ritzValue = actual1.dot(A * actual1);
  const double ritzResidual = (A * actual1 - ritzValue * actual1).norm();
  EXPECT_DOUBLES_EQUAL(0, ritzResidual, 1e-5);

  const double ev1 = 6.0;
  EXPECT_DOUBLES_EQUAL(ev1, pf.eigenvalue(), 1e-5);
}

/* ************************************************************************* */
TEST(PowerMethod, useFactorGraph) {
  // Let's make a scalar synchronization graph with 4 nodes
  GaussianFactorGraph fg;
  auto model = noiseModel::Unit::Create(1);
  for (size_t j = 0; j < 3; j++) {
    fg.add(X(j), -I_1x1, X(j + 1), I_1x1, Vector1::Zero(), model);
  }
  fg.add(X(3), -I_1x1, X(0), I_1x1, Vector1::Zero(), model);  // extra row

  // Get eigenvalues and eigenvectors with Eigen
  auto L = fg.hessian();
  Eigen::EigenSolver<Matrix> solver(L.first);

  // find the index of the max eigenvalue
  size_t maxIdx = 0;
  for (auto i = 0; i < solver.eigenvalues().rows(); ++i) {
    if (solver.eigenvalues()(i).real() >= solver.eigenvalues()(maxIdx).real())
      maxIdx = i;
  }
  // Store the max eigenvalue and its according eigenvector
  const auto ev1 = solver.eigenvalues()(maxIdx).real();
  auto ev2 = solver.eigenvectors().col(maxIdx).real();

  Vector initial = Vector4::Random();
  PowerMethod<Matrix> pf(L.first, initial);
  pf.compute(50, 1e-4);
  EXPECT_DOUBLES_EQUAL(ev1, pf.eigenvalue(), 1e-8);
  auto actual2 = pf.eigenvector();
  EXPECT(assert_equal(ev2, actual2, 3e-5));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
