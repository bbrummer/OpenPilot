/**
 ******************************************************************************
 *
 * @file       videogadgetwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup VideoGadgetPlugin Video Gadget Plugin
 * @{
 * @brief A place holder gadget plugin
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
#include "videogadgetconfiguration.h"
#include "videogadgetwidget.h"
#include "pipeline.h"

#include <QtCore>
#include <QDebug>
#include <QStringList>
#include <QTextEdit>
#include <QPushButton>
#include <QWidget>

VideoGadgetWidget::VideoGadgetWidget(QWidget *parent) :
    QFrame(parent)
{
    m_ui = new Ui_Form();
    m_ui->setupUi(this);

    m_ui->consoleTextBrowser->setVisible(false);

    connect(videoWidget(), &VideoWidget::stateChanged, this, &VideoGadgetWidget::onStateChanged);
    connect(videoWidget(), &VideoWidget::message, this, &VideoGadgetWidget::msg);

    connect(m_ui->startButton, &QPushButton::clicked, this, &VideoGadgetWidget::start);
    connect(m_ui->pauseButton, &QPushButton::clicked, this, &VideoGadgetWidget::pause);
    connect(m_ui->stopButton, &QPushButton::clicked, this, &VideoGadgetWidget::stop);

    connect(m_ui->consoleButton, &QPushButton::clicked, this, &VideoGadgetWidget::console);

    onStateChanged(Pipeline::Null, Pipeline::Null, Pipeline::Null);
}

VideoGadgetWidget::~VideoGadgetWidget()
{
    m_ui = 0;
}

void VideoGadgetWidget::setConfiguration(VideoGadgetConfiguration *config)
{
    videoWidget()->setVisible(config->m_displayVideo);
    // m_ui->control->setEnabled(config->m_displayControls);
    bool restart = false;
    if (videoWidget()->pipelineDesc() != config->m_pipelineDesc) {
        if (videoWidget()->isPlaying()) {
            restart = true;
            stop();
        }
        msg(QString("setting pipeline %0").arg(config->m_pipelineDesc));
        videoWidget()->setPipelineDesc(config->m_pipelineDesc);
    }
    if (restart || (!videoWidget()->isPlaying() && config->m_autoStart)) {
        start();
    }
}

void VideoGadgetWidget::start()
{
    msg(QString("starting..."));
    m_ui->startButton->setEnabled(false);
    videoWidget()->start();
}

void VideoGadgetWidget::pause()
{
    msg(QString("pausing..."));
    m_ui->pauseButton->setEnabled(false);
    videoWidget()->pause();
}

void VideoGadgetWidget::stop()
{
    msg(QString("stopping..."));
    videoWidget()->stop();
}

void VideoGadgetWidget::console()
{
    m_ui->consoleTextBrowser->setVisible(!m_ui->consoleTextBrowser->isVisible());
}

void VideoGadgetWidget::onStateChanged(Pipeline::State oldState, Pipeline::State newState, Pipeline::State pendingState)
{
    msg(QString("state changed: ") + VideoWidget::name(newState));
    m_ui->startButton->setEnabled(true);
    m_ui->pauseButton->setEnabled(true);
    m_ui->stopButton->setEnabled(true);

    m_ui->startButton->setVisible(false);
    m_ui->pauseButton->setVisible(false);
    m_ui->stopButton->setVisible(false);
    switch (newState) {
    case Pipeline::Ready:
        // start & stop (disabled)
        m_ui->startButton->setVisible(true);
        m_ui->stopButton->setEnabled(false);
        m_ui->stopButton->setVisible(true);
        break;
    case Pipeline::Paused:
        if (pendingState == Pipeline::Playing) {
            // pause (disabled) & stop
            m_ui->pauseButton->setEnabled(false);
            m_ui->pauseButton->setVisible(true);
            m_ui->stopButton->setVisible(true);
        }
        else if (pendingState == Pipeline::Ready) {
            // start & stop (disabled)
            m_ui->startButton->setVisible(true);
            m_ui->stopButton->setEnabled(false);
            m_ui->stopButton->setVisible(true);
        }
        else {
            // start & stop
            m_ui->startButton->setVisible(true);
            m_ui->stopButton->setVisible(true);
        }
        break;
    case Pipeline::Playing:
        // pause & stop
        m_ui->pauseButton->setVisible(true);
        m_ui->stopButton->setVisible(true);
        break;
    default:
        // start & stop
        m_ui->startButton->setVisible(true);
        m_ui->stopButton->setVisible(true);
        break;
    }
}

void VideoGadgetWidget::msg(const QString &str)
{
    if (m_ui) {
        m_ui->consoleTextBrowser->append(str);
    }
}

VideoWidget *VideoGadgetWidget::videoWidget()
{
    return m_ui->video;
}
