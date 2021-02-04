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


/*#ifdef _WIN32

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

#endif*/


#include "gui/main_window.h"

#include <QApplication>
#include <QPushButton>
#include <QWindow>
#include <QScreen>

#include <iostream>


int main(int argc, char** argv)
{
	//QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	auto high_dpi_policy = QApplication::highDpiScaleFactorRoundingPolicy();
	QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);

	QApplication app(argc, argv);

	//auto primary_screen = QApplication::primaryScreen();

	MainWindow main_window;
	auto window_device_pixel_ratio = main_window.devicePixelRatio();
	main_window.show();

	return app.exec();
}

