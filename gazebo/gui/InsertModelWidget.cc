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

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "sdf/sdf.hh"
#include "common/SystemPaths.hh"
#include "common/Console.hh"
#include "common/ModelDatabase.hh"

#include "rendering/Rendering.hh"
#include "rendering/Scene.hh"
#include "rendering/UserCamera.hh"
#include "rendering/Visual.hh"
#include "gui/Gui.hh"
#include "gui/GuiEvents.hh"

#include "transport/Node.hh"
#include "transport/Publisher.hh"

#include "gui/InsertModelWidget.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
InsertModelWidget::InsertModelWidget(QWidget *_parent)
: QWidget(_parent)
{
  this->setObjectName("insertModel");
  this->modelDatabaseItem = NULL;

  QVBoxLayout *mainLayout = new QVBoxLayout;
  this->fileTreeWidget = new QTreeWidget();
  this->fileTreeWidget->setColumnCount(1);
  this->fileTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->fileTreeWidget->header()->hide();
  connect(this->fileTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
      this, SLOT(OnModelSelection(QTreeWidgetItem *, int)));

  QFrame *frame = new QFrame;
  QVBoxLayout *frameLayout = new QVBoxLayout;
  frameLayout->addWidget(this->fileTreeWidget, 0);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frame->setLayout(frameLayout);

  mainLayout->addWidget(frame);
  this->setLayout(mainLayout);
  this->layout()->setContentsMargins(0, 0, 0, 0);

  // Create a system path watcher
  this->watcher = new QFileSystemWatcher();

  // Connect a callback that is triggered whenever a directory is changed.
  connect(this->watcher, SIGNAL(directoryChanged(const QString &)),
          this, SLOT(OnDirectoryChanged(const QString &)));

  // Update the list of models on the local system.
  this->UpdateAllLocalPaths();

  // Create a top-level tree item for the path
  this->modelDatabaseItem =
    new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(0),
        QStringList(QString("Connecting to model database...")));
    this->fileTreeWidget->addTopLevelItem(this->modelDatabaseItem);

  /// Non-blocking call to get all the models in the database.
  common::ModelDatabase::Instance()->GetModels(
      boost::bind(&InsertModelWidget::OnModels, this, _1));

  // Start a timer to check for the results from the ModelDatabase. We need
  // to do this so that the QT elements get added in the main thread.
  QTimer::singleShot(1000, this, SLOT(Update()));
}

/////////////////////////////////////////////////
InsertModelWidget::~InsertModelWidget()
{
  delete this->watcher;
}

/////////////////////////////////////////////////
void InsertModelWidget::Update()
{
  boost::mutex::scoped_lock lock(this->mutex);

  // If the model database has call the OnModels callback function, then
  // add all the models from the database.
  if (this->modelBuffer.size() > 0)
  {
    std::string uri = common::ModelDatabase::Instance()->GetURI();
    this->modelDatabaseItem->setText(0,
        QString("%1").arg(QString::fromStdString(uri)));

    if (!this->modelBuffer.empty())
    {
      for (std::map<std::string, std::string>::const_iterator iter =
          this->modelBuffer.begin(); iter != this->modelBuffer.end(); ++iter)
      {
        // Add a child item for the model
        QTreeWidgetItem *childItem = new QTreeWidgetItem(
            this->modelDatabaseItem,
            QStringList(QString("%1").arg(
                QString::fromStdString(iter->second))));
        childItem->setData(0, Qt::UserRole, QVariant(iter->first.c_str()));
        this->fileTreeWidget->addTopLevelItem(childItem);
      }
    }

    this->modelBuffer.clear();
  }
  else
    QTimer::singleShot(1000, this, SLOT(Update()));
}



/////////////////////////////////////////////////
void InsertModelWidget::OnModels(
    const std::map<std::string, std::string> &_models)
{
  boost::mutex::scoped_lock lock(this->mutex);
  this->modelBuffer = _models;
}

/////////////////////////////////////////////////
void InsertModelWidget::OnModelSelection(QTreeWidgetItem *_item,
                                         int /*_column*/)
{
  if (_item)
  {
    std::string path, filename;

    if (_item->parent())
      path = _item->parent()->text(0).toStdString() + "/";

    path = _item->data(0, Qt::UserRole).toString().toStdString();

    if (!path.empty())
    {
      QApplication::setOverrideCursor(Qt::BusyCursor);
      filename = common::ModelDatabase::Instance()->GetModelFile(path);
      gui::Events::createEntity("model", filename);

      this->fileTreeWidget->clearSelection();
      QApplication::setOverrideCursor(Qt::ArrowCursor);
    }
  }
}

/////////////////////////////////////////////////
void InsertModelWidget::UpdateLocalPath(const std::string &_path)
{
  if (_path.empty())
    return;

  QString qpath = QString::fromStdString(_path);
  QTreeWidgetItem *topItem = NULL;

  QList<QTreeWidgetItem *> matchList = this->fileTreeWidget->findItems(qpath,
      Qt::MatchExactly);

  boost::filesystem::path dir(_path);

  // Create a top-level tree item for the path
  if (matchList.size() == 0)
  {
    topItem = new QTreeWidgetItem(
        static_cast<QTreeWidgetItem*>(0), QStringList(qpath));
    this->fileTreeWidget->addTopLevelItem(topItem);

    // Add the new path to the directory watcher
    if (boost::filesystem::exists(dir))
      this->watcher->addPath(qpath);
  }
  else
    topItem = matchList.first();

  // Remove current items.
  topItem->takeChildren();

  std::list<boost::filesystem::path> resultSet;

  if (boost::filesystem::exists(dir) &&
      boost::filesystem::is_directory(dir))
  {
    std::vector<boost::filesystem::path> paths;

    // Get all the paths in alphabetical order
    std::copy(boost::filesystem::directory_iterator(dir),
        boost::filesystem::directory_iterator(),
        std::back_inserter(paths));
    std::sort(paths.begin(), paths.end());

    // Iterate over all the models in the current gazebo path
    for (std::vector<boost::filesystem::path>::iterator dIter = paths.begin();
        dIter != paths.end(); ++dIter)
    {
      std::string modelName;
      boost::filesystem::path fullPath = _path / dIter->filename();
      boost::filesystem::path manifest = fullPath;

      if (!boost::filesystem::is_directory(fullPath))
      {
        if (dIter->filename() == "manifest.xml")
        {
          boost::filesystem::path tmpPath = boost::filesystem::path(_path) /
            "database.config";
          gzwarn << "manifest.xml for a model database is deprecated. "
                 << "Please rename " << fullPath <<  " to "
                 << tmpPath << "\n";
        }
        else if (dIter->filename() != "database.config")
        {
          gzwarn << "Invalid filename or directory[" << fullPath
            << "] in GAZEBO_MODEL_PATH. It's not a good idea to put extra "
            << "files in a GAZEBO_MODEL_PATH because the file structure may"
            << " be modified by Gazebo.\n";
        }
        continue;
      }

      // First try to get the GZ_MODEL_MANIFEST_FILENAME. If that file doesn't
      // exist, try to get the deprecated version.
      if (boost::filesystem::exists(manifest / GZ_MODEL_MANIFEST_FILENAME))
        manifest /= GZ_MODEL_MANIFEST_FILENAME;
      else if (boost::filesystem::exists(manifest / "manifest.xml"))
      {
        gzwarn << "The manifest.xml for a Gazebo model is deprecated. "
          << "Please rename manifest.xml to "
          << GZ_MODEL_MANIFEST_FILENAME << " for model "
          << (*dIter) << "\n";

        manifest /= "manifest.xml";
      }

      if (!boost::filesystem::exists(manifest) || manifest == fullPath)
      {
        gzerr << "model.config file is missing in directory["
              << fullPath << "]\n";
        continue;
      }

      TiXmlDocument xmlDoc;
      if (xmlDoc.LoadFile(manifest.string()))
      {
        TiXmlElement *modelXML = xmlDoc.FirstChildElement("model");
        if (!modelXML || !modelXML->FirstChildElement("name"))
          gzerr << "No model name in manifest[" << manifest << "]\n";
        else
          modelName = modelXML->FirstChildElement("name")->GetText();

        // Add a child item for the model
        QTreeWidgetItem *childItem = new QTreeWidgetItem(topItem,
            QStringList(QString::fromStdString(modelName)));

        childItem->setData(0, Qt::UserRole,
            QVariant((std::string("file://") + fullPath.string()).c_str()));

        this->fileTreeWidget->addTopLevelItem(childItem);
      }
    }
  }

  // Make all top-level items expanded. Trying to reduce mouse clicks.
  this->fileTreeWidget->expandItem(topItem);
}

/////////////////////////////////////////////////
void InsertModelWidget::UpdateAllLocalPaths()
{
  std::list<std::string> gazeboPaths =
    common::SystemPaths::Instance()->GetModelPaths();

  // Iterate over all the gazebo paths
  for (std::list<std::string>::iterator iter = gazeboPaths.begin();
      iter != gazeboPaths.end(); ++iter)
  {
    // This is the full model path
    this->UpdateLocalPath((*iter));
  }
}

/////////////////////////////////////////////////
void InsertModelWidget::OnDirectoryChanged(const QString &_path)
{
  this->UpdateLocalPath(_path.toStdString());
}
