/*
 * Copyright 2013 Open Source Robotics Foundation
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
#include <boost/filesystem.hpp>

#include "gazebo/common/Exception.hh"
#include "gazebo/common/LogRecord.hh"

/////////////////////////////////////////////////
/// \brief Test LogRecord constructor and a few accessors
TEST(LogRecord_TEST, Constructor)
{
  gazebo::common::LogRecord *recorder = gazebo::common::LogRecord::Instance();

  char *homePath = getenv("HOME");
  EXPECT_TRUE(homePath != NULL);

  boost::filesystem::path logPath = "/tmp/gazebo";
  if (homePath)
    logPath = boost::filesystem::path(homePath);
  logPath /= "/.gazebo/log/";

  // Make sure the log path is correct
  EXPECT_EQ(recorder->GetBasePath(), logPath.string());

  EXPECT_FALSE(recorder->GetPaused());
  EXPECT_FALSE(recorder->GetRunning());
  EXPECT_TRUE(recorder->GetFirstUpdate());

  // Init without a subdirectory
  EXPECT_FALSE(recorder->Init(""));
}

/////////////////////////////////////////////////
/// \brief Test LogRecord Start errors
TEST(LogRecord_TEST, StartErrors)
{
  gazebo::common::LogRecord *recorder = gazebo::common::LogRecord::Instance();

  // Start without an init
  {
    EXPECT_FALSE(recorder->Start("bz2"));
  }

  // Invalid encoding
  {
    EXPECT_TRUE(recorder->Init("test"));
    EXPECT_THROW(recorder->Start("garbage"), gazebo::common::Exception);
  }

  // Double start
  {
    EXPECT_TRUE(recorder->Start("bz2"));
    EXPECT_TRUE(recorder->GetRunning());
    EXPECT_FALSE(recorder->Start("bz2"));
  }
}

/////////////////////////////////////////////////
/// \brief Test LogRecord Init and Start
TEST(LogRecord_TEST, Start)
{
  gazebo::common::LogRecord *recorder = gazebo::common::LogRecord::Instance();
  recorder->Init("test");
  recorder->Start("bz2");

  // Make sure the right flags have been set
  EXPECT_FALSE(recorder->GetPaused());
  EXPECT_TRUE(recorder->GetRunning());
  EXPECT_TRUE(recorder->GetFirstUpdate());

  // Make sure the right encoding is set
  EXPECT_EQ(recorder->GetEncoding(), std::string("bz2"));

  // Make sure the log directories exist
  EXPECT_TRUE(boost::filesystem::exists(recorder->GetBasePath()));
  EXPECT_TRUE(boost::filesystem::is_directory(recorder->GetBasePath()));

  // Run time should be zero since no update has been triggered.
  EXPECT_EQ(recorder->GetRunTime(), gazebo::common::Time());

  // Stop recording.
  recorder->Stop();

  // Make sure everything has reset.
  EXPECT_FALSE(recorder->GetRunning());
  EXPECT_FALSE(recorder->GetPaused());
  EXPECT_EQ(recorder->GetRunTime(), gazebo::common::Time());
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
