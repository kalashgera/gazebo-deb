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
/* Desc: Laser Visualization Class
 * Author: Nate Koenig
 * Date: 14 Dec 2007
 */

#include "transport/transport.hh"
#include "rendering/Scene.hh"
#include "rendering/DynamicLines.hh"
#include "rendering/LaserVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
LaserVisual::LaserVisual(const std::string &_name, VisualPtr _vis,
                         const std::string &_topicName)
: Visual(_name, _vis)
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->scene->GetName());

  this->laserScanSub = this->node->Subscribe(_topicName,
      &LaserVisual::OnScan, this);

  this->rayFan = this->CreateDynamicLine(rendering::RENDERING_TRIANGLE_FAN);

  this->rayFan->setMaterial("Gazebo/BlueLaser");
  this->rayFan->AddPoint(math::Vector3(0, 0, 0));
  this->SetVisibilityFlags(GZ_VISIBILITY_GUI);
}

/////////////////////////////////////////////////
LaserVisual::~LaserVisual()
{
  delete this->rayFan;
  this->rayFan = NULL;
}

/////////////////////////////////////////////////
void LaserVisual::OnScan(ConstLaserScanStampedPtr &_msg)
{
  // Skip the update if the user is moving the laser.
  if (this->GetScene()->GetSelectedVisual() &&
      this->GetRootVisual()->GetName() ==
      this->GetScene()->GetSelectedVisual()->GetName())
  {
    return;
  }

  double angle = _msg->scan().angle_min();
  double r;
  math::Vector3 pt;
  math::Pose offset = msgs::Convert(_msg->scan().world_pose()) -
                      this->GetWorldPose();

  this->rayFan->SetPoint(0, offset.pos);
  for (int i = 0; i < _msg->scan().ranges_size(); i++)
  {
    r = _msg->scan().ranges(i) + _msg->scan().range_min();
    pt.x = 0 + r * cos(angle);
    pt.y = 0 + r * sin(angle);
    pt.z = 0;
    pt += offset.pos;

    if (i+1 >= static_cast<int>(this->rayFan->GetPointCount()))
      this->rayFan->AddPoint(pt);
    else
      this->rayFan->SetPoint(i+1, pt);

    angle += _msg->scan().angle_step();
  }
}

/////////////////////////////////////////////////
void LaserVisual::SetEmissive(const common::Color &/*_color*/)
{
}
