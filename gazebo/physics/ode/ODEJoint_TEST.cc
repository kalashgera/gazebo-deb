/*
 * Copyright 2012 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gtest/gtest.h>
#include "gazebo/physics/physics.hh"
#include "test/ServerFixture.hh"

#define TOL 1e-6
#define TOL_CONT 2.0
using namespace gazebo;

class ODEJoint_TEST : public ServerFixture
{
};

////////////////////////////////////////////////////////////////////////
// Load example world with a few joints
// Measure force torques
// with active torque control at joints
////////////////////////////////////////////////////////////////////////
TEST_F(ODEJoint_TEST, GetForceTorqueWithAppliedForce)
{
  // Load our force torque test world
  Load("worlds/force_torque_demo2.world", true);

  // Get a pointer to the world, make sure world loads
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), "ode");

  physics->SetGravity(math::Vector3(0, 0, -50));

  // simulate 1 step
  world->StepWorld(1);
  double t = world->GetSimTime().Double();

  // get time step size
  double dt = world->GetPhysicsEngine()->GetMaxStepSize();
  EXPECT_GT(dt, 0);
  gzdbg << "dt : " << dt << "\n";

  // verify that time moves forward
  EXPECT_GT(t, 0);
  gzdbg << "t after one step : " << t << "\n";

  // get joint and get force torque
  physics::ModelPtr model_1 = world->GetModel("boxes");
  physics::JointPtr joint_01 = model_1->GetJoint("joint1");
  physics::JointPtr joint_12 = model_1->GetJoint("joint2");

  gzdbg << "------------------- PD CONTROL -------------------\n";
  static const double kp1 = 50000.0;
  static const double kp2 = 10000.0;
  for (unsigned int i = 0; i < 3388; ++i)
  {
    // pd control
    double j1State = joint_01->GetAngle(0u).Radian();
    double j2State = joint_12->GetAngle(0u).Radian();
    double p1Error = -j1State;
    double p2Error = -j2State;
    double effort1 = kp1 * p1Error;
    double effort2 = kp2 * p2Error;
    joint_01->SetForce(0u, effort1);
    joint_12->SetForce(0u, effort2);

    world->StepWorld(1);
    // test joint_01 wrench
    physics::JointWrench wrench_01 = joint_01->GetForceTorque(0u);

    if (i == 3387)
    {
      EXPECT_NEAR(wrench_01.body1Force.x,     0.0, TOL_CONT);
      EXPECT_NEAR(wrench_01.body1Force.y,     0.0, TOL_CONT);
      EXPECT_NEAR(wrench_01.body1Force.z,   300.0, TOL_CONT);
      EXPECT_NEAR(wrench_01.body1Torque.x,    0.0, TOL_CONT);
      EXPECT_NEAR(wrench_01.body1Torque.y, -200.0, TOL_CONT);
      EXPECT_NEAR(wrench_01.body1Torque.z,    0.0, TOL_CONT);

      EXPECT_NEAR(wrench_01.body2Force.x,  -wrench_01.body1Force.x,  TOL_CONT);
      EXPECT_NEAR(wrench_01.body2Force.y,  -wrench_01.body1Force.y,  TOL_CONT);
      EXPECT_NEAR(wrench_01.body2Force.z,  -wrench_01.body1Force.z,  TOL_CONT);
      EXPECT_NEAR(wrench_01.body2Torque.x, -wrench_01.body1Torque.x, TOL_CONT);
      EXPECT_NEAR(wrench_01.body2Torque.y, -wrench_01.body1Torque.y, TOL_CONT);
      EXPECT_NEAR(wrench_01.body2Torque.z, -wrench_01.body1Torque.z, TOL_CONT);

      gzdbg << "joint_01 force torque : "
            << "step [" << i
            << "] GetForce [" << joint_01->GetForce(0u)
            << "] command [" << effort1
            << "] force1 [" << wrench_01.body1Force
            << "] torque1 [" << wrench_01.body1Torque
            << "] force2 [" << wrench_01.body2Force
            << "] torque2 [" << wrench_01.body2Torque
            << "]\n";
    }

    // test joint_12 wrench
    physics::JointWrench wrench_12 = joint_12->GetForceTorque(0u);
    if (i == 3387)
    {
      EXPECT_NEAR(wrench_12.body1Force.x,     0.0, TOL_CONT);
      EXPECT_NEAR(wrench_12.body1Force.y,     0.0, TOL_CONT);
      EXPECT_NEAR(wrench_12.body1Force.z,    50.0, TOL_CONT);
      EXPECT_NEAR(wrench_12.body1Torque.x,    0.0, TOL_CONT);
      EXPECT_NEAR(wrench_12.body1Torque.y,  -25.0, TOL_CONT);
      EXPECT_NEAR(wrench_12.body1Torque.z,    0.0, TOL_CONT);

      EXPECT_NEAR(wrench_12.body2Force.x,  -wrench_12.body1Force.x,  TOL_CONT);
      EXPECT_NEAR(wrench_12.body2Force.y,  -wrench_12.body1Force.y,  TOL_CONT);
      EXPECT_NEAR(wrench_12.body2Force.z,  -wrench_12.body1Force.z,  TOL_CONT);
      EXPECT_NEAR(wrench_12.body2Torque.x, -wrench_12.body1Torque.x, TOL_CONT);
      EXPECT_NEAR(wrench_12.body2Torque.y, -wrench_12.body1Torque.y, TOL_CONT);
      EXPECT_NEAR(wrench_12.body2Torque.z, -wrench_12.body1Torque.z, TOL_CONT);

      gzdbg << "joint_12 force torque : "
            << "step [" << i
            << "] GetForce [" << joint_12->GetForce(0u)
            << "] command [" << effort2
            << "] force1 [" << wrench_12.body1Force
            << "] torque1 [" << wrench_12.body1Torque
            << "] force2 [" << wrench_12.body2Force
            << "] torque2 [" << wrench_12.body2Torque
            << "]\n";
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Test multi-axis universal joints
// with cfm damping
////////////////////////////////////////////////////////////////////////
TEST_F(ODEJoint_TEST, CFMDamping)
{
  // Load our force torque test world
  Load("worlds/cfm_damping_test.world", true);

  // Get a pointer to the world, make sure world loads
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), "ode");

  physics->SetGravity(math::Vector3(0, 0, -50));

  // simulate 1 step
  world->StepWorld(1);
  double t = world->GetSimTime().Double();

  // get time step size
  double dt = world->GetPhysicsEngine()->GetMaxStepSize();
  EXPECT_GT(dt, 0);
  gzdbg << "dt : " << dt << "\n";

  // verify that time moves forward
  EXPECT_GT(t, 0);
  gzdbg << "t after one step : " << t << "\n";

  // get joint and get force torque
  physics::ModelPtr model_1 = world->GetModel("model_1");
  physics::JointPtr joint_0 = model_1->GetJoint("joint_0");
  physics::JointPtr joint_1 = model_1->GetJoint("joint_1");

  gzdbg << "-------------------Test 1 (y)-------------------\n";
  physics->SetGravity(math::Vector3(0, 10, 0));
  world->StepWorld(100);
  EXPECT_NEAR(joint_0->GetAngle(0).Radian(), 0.0, 1e-6);
  EXPECT_NEAR(joint_1->GetAngle(0).Radian(), 0.0048295899143964149, 1e-5);
  EXPECT_NEAR(joint_1->GetAngle(1).Radian(), 0.0, 1e-6);
  gzdbg << "time [" << world->GetSimTime().Double()
        << "] j0 [" << joint_0->GetAngle(0).Radian()
        << "] j1(0) [" << joint_1->GetAngle(0).Radian()
        << "] j1(1) [" << joint_1->GetAngle(1).Radian()
        << "]\n";

  gzdbg << "-------------------Test 2 (x)-------------------\n";
  physics->SetGravity(math::Vector3(10, 0, 0));
  world->StepWorld(100);
  EXPECT_NEAR(joint_0->GetAngle(0).Radian(), 0.0, 1e-6);
  EXPECT_NEAR(joint_1->GetAngle(0).Radian(), 0.0050046318305403403, 1e-5);
  EXPECT_NEAR(joint_1->GetAngle(1).Radian(), -0.0048293115636619532, 1e-5);
  gzdbg << "time [" << world->GetSimTime().Double()
        << "] j0 [" << joint_0->GetAngle(0).Radian()
        << "] j1(0) [" << joint_1->GetAngle(0).Radian()
        << "] j1(1) [" << joint_1->GetAngle(1).Radian()
        << "]\n";

  gzdbg << "-------------------Test 3 (joint limit)-------------------\n";
  physics->SetGravity(math::Vector3(1000, 1000, 0));
  world->StepWorld(1000);
  EXPECT_NEAR(joint_0->GetAngle(0).Radian(), 0.0, 0.001);
  EXPECT_NEAR(joint_1->GetAngle(0).Radian(), 0.7, 0.001);
  EXPECT_NEAR(joint_1->GetAngle(1).Radian(), -0.7, 0.001);
  gzdbg << "time [" << world->GetSimTime().Double()
        << "] j0 [" << joint_0->GetAngle(0).Radian()
        << "] j1(0) [" << joint_1->GetAngle(0).Radian()
        << "] j1(1) [" << joint_1->GetAngle(1).Radian()
        << "]\n";
}

////////////////////////////////////////////////////////////////////////
// Load example world with a few joints
// Measure force torques
// Tip over the joints until joint stops are hit, then check force
// torques again
////////////////////////////////////////////////////////////////////////
TEST_F(ODEJoint_TEST, GetForceTorque)
{
  // Load our force torque test world
  Load("worlds/force_torque_demo.world", true);

  // Get a pointer to the world, make sure world loads
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), "ode");

  physics->SetGravity(math::Vector3(0, 0, -50));

  // simulate 1 step
  world->StepWorld(1);
  double t = world->GetSimTime().Double();

  // get time step size
  double dt = world->GetPhysicsEngine()->GetMaxStepSize();
  EXPECT_GT(dt, 0);
  gzdbg << "dt : " << dt << "\n";

  // verify that time moves forward
  EXPECT_GT(t, 0);
  gzdbg << "t after one step : " << t << "\n";

  // get joint and get force torque
  physics::ModelPtr model_1 = world->GetModel("model_1");
  physics::JointPtr joint_01 = model_1->GetJoint("joint_01");
  physics::JointPtr joint_12 = model_1->GetJoint("joint_12");

  gzdbg << "-------------------Test 1-------------------\n";
  for (unsigned int i = 0; i < 5; ++i)
  {
    world->StepWorld(1);
    // test joint_01 wrench
    physics::JointWrench wrench_01 = joint_01->GetForceTorque(0u);
    EXPECT_NEAR(wrench_01.body1Force.x,    0.0, TOL);
    EXPECT_NEAR(wrench_01.body1Force.y,    0.0, TOL);
    EXPECT_NEAR(wrench_01.body1Force.z, 1000.0, TOL);
    EXPECT_NEAR(wrench_01.body1Torque.x,   0.0, TOL);
    EXPECT_NEAR(wrench_01.body1Torque.y,   0.0, TOL);
    EXPECT_NEAR(wrench_01.body1Torque.z,   0.0, TOL);

    EXPECT_NEAR(wrench_01.body2Force.x,  -wrench_01.body1Force.x,  TOL);
    EXPECT_NEAR(wrench_01.body2Force.y,  -wrench_01.body1Force.y,  TOL);
    EXPECT_NEAR(wrench_01.body2Force.z,  -wrench_01.body1Force.z,  TOL);
    EXPECT_NEAR(wrench_01.body2Torque.x, -wrench_01.body1Torque.x, TOL);
    EXPECT_NEAR(wrench_01.body2Torque.y, -wrench_01.body1Torque.y, TOL);
    EXPECT_NEAR(wrench_01.body2Torque.z, -wrench_01.body1Torque.z, TOL);

    gzdbg << "joint_01 force torque : "
          << "force1 [" << wrench_01.body1Force
          << "] torque1 [" << wrench_01.body1Torque
          << "] force2 [" << wrench_01.body2Force
          << "] torque2 [" << wrench_01.body2Torque
          << "]\n";

    // test joint_12 wrench
    physics::JointWrench wrench_12 = joint_12->GetForceTorque(0u);
    EXPECT_NEAR(wrench_12.body1Force.x,    0.0, TOL);
    EXPECT_NEAR(wrench_12.body1Force.y,    0.0, TOL);
    EXPECT_NEAR(wrench_12.body1Force.z,  500.0, TOL);
    EXPECT_NEAR(wrench_12.body1Torque.x,   0.0, TOL);
    EXPECT_NEAR(wrench_12.body1Torque.y,   0.0, TOL);
    EXPECT_NEAR(wrench_12.body1Torque.z,   0.0, TOL);

    EXPECT_NEAR(wrench_12.body2Force.x,  -wrench_12.body1Force.x,  TOL);
    EXPECT_NEAR(wrench_12.body2Force.y,  -wrench_12.body1Force.y,  TOL);
    EXPECT_NEAR(wrench_12.body2Force.z,  -wrench_12.body1Force.z,  TOL);
    EXPECT_NEAR(wrench_12.body2Torque.x, -wrench_12.body1Torque.x, TOL);
    EXPECT_NEAR(wrench_12.body2Torque.y, -wrench_12.body1Torque.y, TOL);
    EXPECT_NEAR(wrench_12.body2Torque.z, -wrench_12.body1Torque.z, TOL);

    gzdbg << "joint_12 force torque : "
          << "force1 [" << wrench_12.body1Force
          << "] torque1 [" << wrench_12.body1Torque
          << "] force2 [" << wrench_12.body2Force
          << "] torque2 [" << wrench_12.body2Torque
          << "]\n";
  }

  // perturbe joints so top link topples over, then remeasure
  physics->SetGravity(math::Vector3(-30, 10, -50));
  // tune joint stop properties
  joint_01->SetAttribute("stop_erp", 0, 0.02);
  joint_12->SetAttribute("stop_erp", 0, 0.02);
  // wait for dynamics to stabilize
  world->StepWorld(2000);
  // check force torques in new system
  gzdbg << "\n-------------------Test 2-------------------\n";
  for (unsigned int i = 0; i < 5; ++i)
  {
    world->StepWorld(1);

    // test joint_01 wrench
    physics::JointWrench wrench_01 = joint_01->GetForceTorque(0u);
    EXPECT_NEAR(wrench_01.body1Force.x,   600.0,  6.0);
    EXPECT_NEAR(wrench_01.body1Force.y, -1000.0, 10.0);
    EXPECT_NEAR(wrench_01.body1Force.z,  -200.0,  2.0);
    EXPECT_NEAR(wrench_01.body1Torque.x,  750.0,  7.5);
    EXPECT_NEAR(wrench_01.body1Torque.y,  450.0,  4.5);
    EXPECT_NEAR(wrench_01.body1Torque.z,    0.0,  0.1);

    // since first link is world, these should be exact
    EXPECT_NEAR(wrench_01.body2Force.x,  -wrench_01.body1Force.x,  TOL);
    EXPECT_NEAR(wrench_01.body2Force.y,  -wrench_01.body1Force.y,  TOL);
    EXPECT_NEAR(wrench_01.body2Force.z,  -wrench_01.body1Force.z,  TOL);
    EXPECT_NEAR(wrench_01.body2Torque.x, -wrench_01.body1Torque.x, TOL);
    EXPECT_NEAR(wrench_01.body2Torque.y, -wrench_01.body1Torque.y, TOL);
    EXPECT_NEAR(wrench_01.body2Torque.z, -wrench_01.body1Torque.z, TOL);

    gzdbg << "joint_01 force torque : "
          << "force1 [" << wrench_01.body1Force
          << "] torque1 [" << wrench_01.body1Torque
          << "] force2 [" << wrench_01.body2Force
          << "] torque2 [" << wrench_01.body2Torque
          << "]\n";

    // test joint_12 wrench
    physics::JointWrench wrench_12 = joint_12->GetForceTorque(0u);
    EXPECT_NEAR(wrench_12.body1Force.x,   300.0,  3.0);
    EXPECT_NEAR(wrench_12.body1Force.y,  -500.0,  5.0);
    EXPECT_NEAR(wrench_12.body1Force.z,  -100.0,  1.0);
    EXPECT_NEAR(wrench_12.body1Torque.x,  250.0,  5.0);
    EXPECT_NEAR(wrench_12.body1Torque.y,  150.0,  3.0);
    EXPECT_NEAR(wrench_12.body1Torque.z,    0.0,  0.1);

    // A good check is that
    // the computed body1Torque shoud in fact be opposite of body1Torque
    EXPECT_NEAR(wrench_12.body2Force.x,  -wrench_12.body1Force.x,  1e-1);
    EXPECT_NEAR(wrench_12.body2Force.y,  -wrench_12.body1Force.y,  1e-1);
    EXPECT_NEAR(wrench_12.body2Force.z,  -wrench_12.body1Force.z,  1e-1);
    EXPECT_NEAR(wrench_12.body2Torque.x, -wrench_12.body1Torque.x, 1e-1);
    EXPECT_NEAR(wrench_12.body2Torque.y, -wrench_12.body1Torque.y, 1e-1);
    EXPECT_NEAR(wrench_12.body2Torque.z, -wrench_12.body1Torque.z, 1e-1);

    gzdbg << "joint_12 force torque : "
          << "force1 [" << wrench_12.body1Force
          << "] torque1 [" << wrench_12.body1Torque
          << "] force2 [" << wrench_12.body2Force
          << "] torque2 [" << wrench_12.body2Torque
          << "]\n";
  }

  // simulate a few steps
  int steps = 20;
  world->StepWorld(steps);
  t = world->GetSimTime().Double();
  EXPECT_GT(t, 0.99*dt*static_cast<double>(steps+1));
  gzdbg << "t after 20 steps : " << t << "\n";
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
