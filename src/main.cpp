/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2020  offa
 * Copyright (C) 2010-2016  piorekf <piorek@piorekf.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/mainwindow.h"
#include "version.h"
#include <QApplication>
#include <QPalette> 
#include <QStyleFactory> 

int main(int argc, char* argv[])
{
    QApplication app{argc, argv};
    QCoreApplication::setOrganizationName("offa");
    QCoreApplication::setApplicationName("Plug");
    QCoreApplication::setApplicationVersion(QString::fromStdString(plug::version()));
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;

    // Customize the color palette for the interface elements
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    // Install this palette
    qApp->setPalette(darkPalette);

    plug::MainWindow window;
    window.show();

    return app.exec();
}
