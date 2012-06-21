/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testSimpleCamera.cpp
 * @author Frank Dellaert
 * @brief test SimpleCamera class
 */

#include <cmath>
#include <iostream>

#include <CppUnitLite/TestHarness.h>
#include <gtsam/base/Testable.h>
#include <gtsam/base/numericalDerivative.h>
#include <gtsam/geometry/SimpleCamera.h>

using namespace std;
using namespace gtsam;

static const Cal3_S2 K(625, 625, 0, 0, 0);

static const Pose3 pose1(Matrix_(3,3,
				      1., 0., 0.,
				      0.,-1., 0.,
				      0., 0.,-1.
				      ),
			      Point3(0,0,0.5));
 
static const SimpleCamera camera(pose1, K);

static const Point3 point1(-0.08,-0.08, 0.0);
static const Point3 point2(-0.08, 0.08, 0.0);
static const Point3 point3( 0.08, 0.08, 0.0);
static const Point3 point4( 0.08,-0.08, 0.0);

/* ************************************************************************* */
TEST( SimpleCamera, constructor)
{
  CHECK(assert_equal( camera.calibration(), K));
  CHECK(assert_equal( camera.pose(), pose1));
}

/* ************************************************************************* */
TEST( SimpleCamera, level2)
{
	// Create a level camera, looking in Y-direction
	Pose2 pose2(0.4,0.3,M_PI/2.0);
	SimpleCamera camera = SimpleCamera::level(K, pose2, 0.1);

	// expected
	Point3 x(1,0,0),y(0,0,-1),z(0,1,0);
	Rot3 wRc(x,y,z);
	Pose3 expected(wRc,Point3(0.4,0.3,0.1));
  CHECK(assert_equal( camera.pose(), expected));
}

/* ************************************************************************* */
TEST( SimpleCamera, lookat)
{
	// Create a level camera, looking in Y-direction
	Point3 C(10.0,0.0,0.0);
	SimpleCamera camera = SimpleCamera::lookat(C, Point3(), Point3(0.0,0.0,1.0));

	// expected
	Point3 xc(0,1,0),yc(0,0,-1),zc(-1,0,0);
	Pose3 expected(Rot3(xc,yc,zc),C);
  CHECK(assert_equal( camera.pose(), expected));

  Point3 C2(30.0,0.0,10.0);
  SimpleCamera camera2 = SimpleCamera::lookat(C2, Point3(), Point3(0.0,0.0,1.0));

  Matrix R = camera2.pose().rotation().matrix();
  Matrix I = trans(R)*R;
  CHECK(assert_equal(I, eye(3)));
}

/* ************************************************************************* */
TEST( SimpleCamera, project)
{
  CHECK(assert_equal( camera.project(point1), Point2(-100,  100) ));
  CHECK(assert_equal( camera.project(point2), Point2(-100, -100) ));
  CHECK(assert_equal( camera.project(point3), Point2( 100, -100) ));
  CHECK(assert_equal( camera.project(point4), Point2( 100,  100) ));
}

/* ************************************************************************* */
TEST( SimpleCamera, backproject)
{
  CHECK(assert_equal( camera.backproject(Point2(-100,  100), 0.5),  point1));
  CHECK(assert_equal( camera.backproject(Point2(-100, -100), 0.5),  point2));
  CHECK(assert_equal( camera.backproject(Point2( 100, -100), 0.5),  point3));
  CHECK(assert_equal( camera.backproject(Point2( 100,  100), 0.5),  point4));
}

/* ************************************************************************* */
TEST( SimpleCamera, backproject2)
{
	Point3 origin;
	Rot3 rot(1., 0., 0., 0., 0., 1., 0., -1., 0.); // a camera looking down
	SimpleCamera camera(Pose3(rot, origin), K);

	Point3 actual = camera.backproject(Point2(), 1.);
	Point3 expected(0., 1., 0.);
	pair<Point2, bool> x = camera.projectSafe(expected);

	CHECK(assert_equal(expected, actual));
	CHECK(assert_equal(Point2(), x.first));
	CHECK(x.second);
}

/* ************************************************************************* */
static Point2 project2(const Pose3& pose, const Point3& point) {
	return SimpleCamera(pose,K).project(point);
}

TEST( SimpleCamera, Dproject_point_pose)
{
	Matrix Dpose, Dpoint;
	Point2 result = camera.project(point1, Dpose, Dpoint);
	Matrix numerical_pose  = numericalDerivative21(project2, pose1, point1);
	Matrix numerical_point = numericalDerivative22(project2, pose1, point1);
	CHECK(assert_equal(result, Point2(-100,  100) ));
	CHECK(assert_equal(Dpose,  numerical_pose, 1e-7));
	CHECK(assert_equal(Dpoint, numerical_point,1e-7));
}

/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */


