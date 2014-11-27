/********************************************************************************
 * @file       osgearthviewwidget.h
 * @author     The OpenPilot Team Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OsgEarthview Plugin
 * @{
 * @brief Osg Earth view of UAV
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef OSGEARTHVIEWWIDGET_H_
#define OSGEARTHVIEWWIDGET_H_

#include "osgviewerwidget.h"

class Ui_OsgEarthview;

class OsgEarthviewWidget : public QWidget {
    Q_OBJECT

public:
    OsgEarthviewWidget(QWidget *parent = 0);
    ~OsgEarthviewWidget();

    void setSceneFile(QString sceneFile);

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    //OsgViewerWidget *viewWidget;
    Ui_OsgEarthview *m_widget;
};
#endif /* OSGEARTHVIEWWIDGET_H_ */
