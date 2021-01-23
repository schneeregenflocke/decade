/*
Decade
Copyright (c) 2019-2021 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../packages/calendar_config.h"
#include "../packages/date_store.h"
#include "../packages/group_store.h"
#include "../packages/page_config.h"
#include "../packages/shape_config.h"
#include "../packages/title_config.h"

#include "../calendar_view.h"

#include "license_info_dialog.h"

#include <QWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>

#include <sigslot/signal.hpp>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/nvp.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <memory>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr) :
        QMainWindow(parent)
    {
        setMinimumSize(800, 600);
        setCentralWidget(new QWidget);
        
        QMenu* file_menu = menuBar()->addMenu(tr("&File"));
        QMenu* info_menu = menuBar()->addMenu(tr("&Info"));
        QAction* open_license_dialog = info_menu->addAction(tr("&Open Source Licenses"));
        

        
        QStatusBar* status_bar = new QStatusBar(this);
        setStatusBar(status_bar);

        LicenseDialog* license_dialog = new LicenseDialog(this);
        connect(open_license_dialog, &QAction::triggered, license_dialog, &LicenseDialog::open);
    }


private:

    void InitMenu()
    {
    }

    void OpenGLReady()
    {
    }

    void EstablishConnections()
    {

    }

    void LoadXML(const std::string& filepath)
    {
        std::ifstream filestream(filepath);
        boost::archive::xml_iarchive oarchive(filestream);
        
        oarchive >> BOOST_SERIALIZATION_NVP(date_groups_store);
        oarchive >> BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
        oarchive >> BOOST_SERIALIZATION_NVP(page_setup_store);
        oarchive >> BOOST_SERIALIZATION_NVP(title_config_store);
        oarchive >> BOOST_SERIALIZATION_NVP(shape_configuration_storage);
        oarchive >> BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
    }

    void SaveXML(const std::string& filepath)
    {
        std::ofstream filestream(filepath);
        boost::archive::xml_oarchive oarchive(filestream);

        oarchive << BOOST_SERIALIZATION_NVP(date_groups_store);
        oarchive << BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
        oarchive << BOOST_SERIALIZATION_NVP(page_setup_store);
        oarchive << BOOST_SERIALIZATION_NVP(title_config_store);
        oarchive << BOOST_SERIALIZATION_NVP(shape_configuration_storage);
        oarchive << BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
    }

    void SlotImportCSV()
    { 
    }

    void SlotExportCSV()
    {
    }

    void SlotExportPNG()
    {
    }

    

    std::unique_ptr<CalendarPage> calendar;

    // Storages
    DateGroupStore date_groups_store;
    DateIntervalBundleStore date_interval_bundle_store;
    TransformDateIntervalBundle transform_date_interval_bundle;
    PageSetupStore page_setup_store;
    TitleConfigStore title_config_store;
    ShapeConfigurationStorage shape_configuration_storage;
    CalendarConfigStorage calendar_configuration_storage;
};
