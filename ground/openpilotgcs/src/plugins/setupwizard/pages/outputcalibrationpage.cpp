/**
 ******************************************************************************
 *
 * @file       outputcalibrationpage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup OutputCalibrationPage
 * @{
 * @brief
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

#include "outputcalibrationpage.h"
#include "ui_outputcalibrationpage.h"
#include "systemalarms.h"
#include "uavobjectmanager.h"

const QString OutputCalibrationPage::MULTI_SVG_FILE     = QString(":/setupwizard/resources/multirotor-shapes.svg");
const QString OutputCalibrationPage::FIXEDWING_SVG_FILE = QString(":/setupwizard/resources/fixedwing-shapes-wizard.svg");

OutputCalibrationPage::OutputCalibrationPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent), ui(new Ui::OutputCalibrationPage), m_vehicleBoundsItem(0),
    m_currentWizardIndex(-1), m_calibrationUtil(0)
{
    ui->setupUi(this);

    qDebug() << "calling output calibration page";
    m_vehicleRenderer = new QSvgRenderer();

    // move the code that was here to setupVehicle() so we can determine which image to use.
    m_vehicleScene    = new QGraphicsScene(this);
    ui->vehicleView->setScene(m_vehicleScene);
}

OutputCalibrationPage::~OutputCalibrationPage()
{
    if (m_calibrationUtil) {
        delete m_calibrationUtil;
        m_calibrationUtil = 0;
    }
    delete ui;
}

void OutputCalibrationPage::loadSVGFile(QString file)
{
    if (QFile::exists(file) && m_vehicleRenderer->load(file) && m_vehicleRenderer->isValid()) {
        ui->vehicleView->setScene(m_vehicleScene);
    }
}

void OutputCalibrationPage::setupActuatorMinMaxAndNeutral(int motorChannelStart, int motorChannelEnd, int totalUsedChannels)
{
    // Default values for the actuator settings, extra important for
    // servos since a value out of range can actually destroy the
    // vehicle if unlucky.
    // Motors are not that important. REMOVE propellers always!!

    for (int servoid = 0; servoid < 12; servoid++) {
        if (servoid >= motorChannelStart && servoid <= motorChannelEnd) {
            // Set to motor safe values
            m_actuatorSettings[servoid].channelMin     = 1000;
            m_actuatorSettings[servoid].channelNeutral = 1000;
            m_actuatorSettings[servoid].channelMax     = 1900;
        } else if (servoid < totalUsedChannels) {
            // Set to servo safe values
            m_actuatorSettings[servoid].channelMin     = 1500;
            m_actuatorSettings[servoid].channelNeutral = 1500;
            m_actuatorSettings[servoid].channelMax     = 1500;
        } else {
            // "Disable" these channels
            m_actuatorSettings[servoid].channelMin     = 1000;
            m_actuatorSettings[servoid].channelNeutral = 1000;
            m_actuatorSettings[servoid].channelMax     = 1000;
        }
    }
}

void OutputCalibrationPage::setupVehicle()
{
    m_actuatorSettings = getWizard()->getActuatorSettings();
    m_wizardIndexes.clear();
    m_vehicleElementIds.clear();
    m_vehicleHighlightElementIndexes.clear();
    m_channelIndex.clear();
    m_currentWizardIndex = 0;
    m_vehicleScene->clear();

    switch (getWizard()->getVehicleSubType()) {
    case SetupWizard::MULTI_ROTOR_TRI_Y:
        // Loads the SVG file resourse and sets the scene
        loadSVGFile(MULTI_SVG_FILE);

        // The m_wizardIndexes array contains the index of the QStackedWidget
        // in the page to use for each step.
        m_wizardIndexes << 0 << 1 << 1 << 1 << 2;

        // All element ids to load from the svg file and manage.
        m_vehicleElementIds << "tri" << "tri-frame" << "tri-m1" << "tri-m2" << "tri-m3" << "tri-s1";

        // The index of the elementId to highlight ( not dim ) for each step
        // this is the index in the m_vehicleElementIds - 1.
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;

        // The channel number to configure for each step.
        m_channelIndex << 0 << 0 << 1 << 2 << 3;

        setupActuatorMinMaxAndNeutral(0, 2, 3);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::MULTI_ROTOR_QUAD_X:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "quad-x" << "quad-x-frame" << "quad-x-m1" << "quad-x-m2" << "quad-x-m3" << "quad-x-m4";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 0 << 1 << 2 << 3;
        setupActuatorMinMaxAndNeutral(0, 3, 4);
        break;
    case SetupWizard::MULTI_ROTOR_QUAD_PLUS:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "quad-p" << "quad-p-frame" << "quad-p-m1" << "quad-p-m2" << "quad-p-m3" << "quad-p-m4";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 0 << 1 << 2 << 3;
        setupActuatorMinMaxAndNeutral(0, 3, 4);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa" << "hexa-frame" << "hexa-m1" << "hexa-m2" << "hexa-m3" << "hexa-m4" << "hexa-m5" << "hexa-m6";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_COAX_Y:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-y6" << "hexa-y6-frame" << "hexa-y6-m2" << "hexa-y6-m1" << "hexa-y6-m4" << "hexa-y6-m3" << "hexa-y6-m6" << "hexa-y6-m5";
        m_vehicleHighlightElementIndexes << 0 << 2 << 1 << 4 << 3 << 6 << 5;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_H:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-h" << "hexa-h-frame" << "hexa-h-m1" << "hexa-h-m2" << "hexa-h-m3" << "hexa-h-m4" << "hexa-h-m5" << "hexa-h-m6";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_X:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-x" << "hexa-x-frame" << "hexa-x-m1" << "hexa-x-m2" << "hexa-x-m3" << "hexa-x-m4" << "hexa-x-m5" << "hexa-x-m6";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    // Fixed Wing
    case SetupWizard::FIXED_WING_DUAL_AILERON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2 << 2;
        m_vehicleElementIds << "aileron" << "aileron-frame" << "aileron-motor" << "aileron-ail-left" << "aileron-ail-right" << "aileron-elevator" << "aileron-rudder";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 2 << 0 << 5 << 1 << 3;

        setupActuatorMinMaxAndNeutral(2, 2, 5);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_AILERON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2;
        m_vehicleElementIds << "aileron-single" << "ail2-frame" << "ail2-motor" << "ail2-aileron" << "ail2-elevator" << "ail2-rudder";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 2 << 0 << 1 << 3;

        setupActuatorMinMaxAndNeutral(2, 2, 4);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_ELEVON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2;
        m_vehicleElementIds << "elevon" << "elevon-frame" << "elevon-motor" << "elevon-left" << "elevon-right";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3;
        m_channelIndex << 0 << 2 << 0 << 1;

        setupActuatorMinMaxAndNeutral(2, 2, 3);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_VTAIL:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2 << 2;
        m_vehicleElementIds << "vtail" << "vtail-frame" << "vtail-motor" << "vtail-ail-left" << "vtail-ail-right" << "vtail-rudder-left" << "vtail-rudder-right";
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 2 << 0 << 5 << 3 << 1;

        setupActuatorMinMaxAndNeutral(2, 2, 5);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    default:
        break;
    }

    if (m_calibrationUtil) {
        delete m_calibrationUtil;
        m_calibrationUtil = 0;
    }
    m_calibrationUtil = new OutputCalibrationUtil();

    setupVehicleItems();
}

void OutputCalibrationPage::setupVehicleItems()
{
    m_vehicleItems.clear();
    m_vehicleBoundsItem = new QGraphicsSvgItem();
    m_vehicleBoundsItem->setSharedRenderer(m_vehicleRenderer);
    m_vehicleBoundsItem->setElementId(m_vehicleElementIds[0]);
    m_vehicleBoundsItem->setZValue(-1);
    m_vehicleBoundsItem->setOpacity(0);
    m_vehicleScene->addItem(m_vehicleBoundsItem);

    QRectF parentBounds = m_vehicleRenderer->boundsOnElement(m_vehicleElementIds[0]);

    for (int i = 1; i < m_vehicleElementIds.size(); i++) {
        QGraphicsSvgItem *item = new QGraphicsSvgItem();
        item->setSharedRenderer(m_vehicleRenderer);
        item->setElementId(m_vehicleElementIds[i]);
        item->setZValue(i);
        item->setOpacity(1.0);

        QRectF itemBounds = m_vehicleRenderer->boundsOnElement(m_vehicleElementIds[i]);
        item->setPos(itemBounds.x() - parentBounds.x(), itemBounds.y() - parentBounds.y());

        m_vehicleScene->addItem(item);
        m_vehicleItems << item;
    }
}

void OutputCalibrationPage::startWizard()
{
    ui->calibrationStack->setCurrentIndex(m_wizardIndexes[0]);
    setupVehicleHighlightedPart();
}

void OutputCalibrationPage::setupVehicleHighlightedPart()
{
    qreal dimOpaque = m_currentWizardIndex == 0 ? 1.0 : 0.3;
    qreal highlightOpaque = 1.0;
    int highlightedIndex  = m_vehicleHighlightElementIndexes[m_currentWizardIndex];

    for (int i = 0; i < m_vehicleItems.size(); i++) {
        QGraphicsSvgItem *item = m_vehicleItems[i];
        item->setOpacity((highlightedIndex == i) ? highlightOpaque : dimOpaque);
    }
}

void OutputCalibrationPage::setWizardPage()
{
    qDebug() << "Wizard index: " << m_currentWizardIndex;

    QApplication::processEvents();

    int currentPageIndex = m_wizardIndexes[m_currentWizardIndex];
    qDebug() << "Current page: " << currentPageIndex;
    ui->calibrationStack->setCurrentIndex(currentPageIndex);

    int currentChannel = getCurrentChannel();
    qDebug() << "Current channel: " << currentChannel + 1;
    if (currentChannel >= 0) {
        if (currentPageIndex == 1) {
            ui->motorNeutralSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
        } else if (currentPageIndex == 2) {
            if (m_actuatorSettings[currentChannel].channelMax < m_actuatorSettings[currentChannel].channelMin &&
                !ui->reverseCheckbox->isChecked()) {
                ui->reverseCheckbox->setChecked(true);
            } else {
                ui->reverseCheckbox->setChecked(false);
            }
            enableServoSliders(false);
            if (ui->reverseCheckbox->isChecked()) {
                ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
                ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
            } else {
                ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
                ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
            }
        }
    }
    setupVehicleHighlightedPart();
}

void OutputCalibrationPage::initializePage()
{
    if (m_vehicleScene) {
        setupVehicle();
        startWizard();
    }
}

bool OutputCalibrationPage::validatePage()
{
    if (isFinished()) {
        getWizard()->setActuatorSettings(m_actuatorSettings);
        return true;
    } else {
        m_currentWizardIndex++;
        setWizardPage();
        return false;
    }
}

void OutputCalibrationPage::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    if (m_vehicleBoundsItem) {
        ui->vehicleView->setSceneRect(m_vehicleBoundsItem->boundingRect());
        ui->vehicleView->fitInView(m_vehicleBoundsItem, Qt::KeepAspectRatio);
    }
}

void OutputCalibrationPage::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (m_vehicleBoundsItem) {
        ui->vehicleView->setSceneRect(m_vehicleBoundsItem->boundingRect());
        ui->vehicleView->fitInView(m_vehicleBoundsItem, Qt::KeepAspectRatio);
    }
}

void OutputCalibrationPage::customBackClicked()
{
    if (m_currentWizardIndex > 0) {
        m_currentWizardIndex--;
        setWizardPage();
    } else {
        getWizard()->back();
    }
}

quint16 OutputCalibrationPage::getCurrentChannel()
{
    return m_channelIndex[m_currentWizardIndex];
}

void OutputCalibrationPage::enableButtons(bool enable)
{
    getWizard()->button(QWizard::NextButton)->setEnabled(enable);
    getWizard()->button(QWizard::CustomButton1)->setEnabled(enable);
    getWizard()->button(QWizard::CancelButton)->setEnabled(enable);
    getWizard()->button(QWizard::BackButton)->setEnabled(enable);
    QApplication::processEvents();
}

void OutputCalibrationPage::on_motorNeutralButton_toggled(bool checked)
{
    ui->motorNeutralButton->setText(checked ? tr("Stop") : tr("Start"));
    ui->motorNeutralSlider->setEnabled(checked);
    quint16 channel   = getCurrentChannel();
    quint16 safeValue = m_actuatorSettings[channel].channelMin;
    onStartButtonToggle(ui->motorNeutralButton, channel, m_actuatorSettings[channel].channelNeutral, safeValue, ui->motorNeutralSlider);
}

void OutputCalibrationPage::onStartButtonToggle(QAbstractButton *button, quint16 channel, quint16 value, quint16 safeValue, QSlider *slider)
{
    if (button->isChecked()) {
        if (checkAlarms()) {
            enableButtons(false);
            enableServoSliders(true);
            OutputCalibrationUtil::startOutputCalibration();
            m_calibrationUtil->startChannelOutput(channel, safeValue);
            slider->setValue(value);
            m_calibrationUtil->setChannelOutputValue(value);
        } else {
            button->setChecked(false);
        }
    } else {
        m_calibrationUtil->stopChannelOutput();
        OutputCalibrationUtil::stopOutputCalibration();
        enableServoSliders(false);
        enableButtons(true);
    }
    debugLogChannelValues();
}

void OutputCalibrationPage::enableServoSliders(bool enabled)
{
    ui->servoCenterAngleSlider->setEnabled(enabled);
    ui->servoMinAngleSlider->setEnabled(enabled);
    ui->servoMaxAngleSlider->setEnabled(enabled);
    ui->reverseCheckbox->setEnabled(!enabled);
}

bool OutputCalibrationPage::checkAlarms()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavObjectManager = pm->getObject<UAVObjectManager>();

    Q_ASSERT(uavObjectManager);
    SystemAlarms *systemAlarms    = SystemAlarms::GetInstance(uavObjectManager);
    Q_ASSERT(systemAlarms);
    SystemAlarms::DataFields data = systemAlarms->getData();

    if (data.Alarm[SystemAlarms::ALARM_ACTUATOR] != SystemAlarms::ALARM_OK) {
        QMessageBox mbox(this);
        mbox.setText(QString(tr("The actuator module is in an error state.\n\n"
                                "Please make sure the correct firmware version is used then "
                                "restart the wizard and try again. If the problem persists please "
                                "consult the openpilot.org support forum.")));
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.setIcon(QMessageBox::Critical);

        getWizard()->setWindowFlags(getWizard()->windowFlags() & ~Qt::WindowStaysOnTopHint);

        mbox.exec();

        getWizard()->setWindowFlags(getWizard()->windowFlags() | Qt::WindowStaysOnTopHint);
        getWizard()->setWindowIcon(qApp->windowIcon());
        getWizard()->show();
        return false;
    }
    return true;
}

void OutputCalibrationPage::debugLogChannelValues()
{
    quint16 channel = getCurrentChannel();

    qDebug() << "ChannelMin    : " << m_actuatorSettings[channel].channelMin;
    qDebug() << "ChannelNeutral: " << m_actuatorSettings[channel].channelNeutral;
    qDebug() << "ChannelMax    : " << m_actuatorSettings[channel].channelMax;
}

void OutputCalibrationPage::on_motorNeutralSlider_valueChanged(int value)
{
    Q_UNUSED(value);
    if (ui->motorNeutralButton->isChecked()) {
        quint16 value = ui->motorNeutralSlider->value();
        m_calibrationUtil->setChannelOutputValue(value);
        m_actuatorSettings[getCurrentChannel()].channelNeutral = value;
        debugLogChannelValues();
    }
}

void OutputCalibrationPage::on_servoButton_toggled(bool checked)
{
    ui->servoButton->setText(checked ? tr("Stop") : tr("Start"));
    quint16 channel   = getCurrentChannel();
    quint16 safeValue = m_actuatorSettings[channel].channelNeutral;
    onStartButtonToggle(ui->servoButton, channel, safeValue, safeValue, ui->servoCenterAngleSlider);
}

void OutputCalibrationPage::on_servoCenterAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value   = ui->servoCenterAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);
    quint16 channel = getCurrentChannel();
    m_actuatorSettings[channel].channelNeutral = value;

    // Adjust min and max
    if (ui->reverseCheckbox->isChecked()) {
        if (value >= m_actuatorSettings[channel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[channel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[channel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[channel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    }
    debugLogChannelValues();
}

void OutputCalibrationPage::on_servoMinAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoMinAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);
    m_actuatorSettings[getCurrentChannel()].channelMin = value;

    // Adjust neutral and max
    if (ui->reverseCheckbox->isChecked()) {
        if (value <= m_actuatorSettings[getCurrentChannel()].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[getCurrentChannel()].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    } else {
        if (value >= m_actuatorSettings[getCurrentChannel()].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[getCurrentChannel()].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    }
    debugLogChannelValues();
}

void OutputCalibrationPage::on_servoMaxAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoMaxAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);
    m_actuatorSettings[getCurrentChannel()].channelMax = value;

    // Adjust neutral and min
    if (ui->reverseCheckbox->isChecked()) {
        if (value >= m_actuatorSettings[getCurrentChannel()].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[getCurrentChannel()].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[getCurrentChannel()].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[getCurrentChannel()].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
    }
    debugLogChannelValues();
}

void OutputCalibrationPage::on_reverseCheckbox_toggled(bool checked)
{
    if (checked && m_actuatorSettings[getCurrentChannel()].channelMax > m_actuatorSettings[getCurrentChannel()].channelMin) {
        quint16 oldMax = m_actuatorSettings[getCurrentChannel()].channelMax;
        m_actuatorSettings[getCurrentChannel()].channelMax = m_actuatorSettings[getCurrentChannel()].channelMin;
        m_actuatorSettings[getCurrentChannel()].channelMin = oldMax;
    } else if (!checked && m_actuatorSettings[getCurrentChannel()].channelMax < m_actuatorSettings[getCurrentChannel()].channelMin) {
        quint16 oldMax = m_actuatorSettings[getCurrentChannel()].channelMax;
        m_actuatorSettings[getCurrentChannel()].channelMax = m_actuatorSettings[getCurrentChannel()].channelMin;
        m_actuatorSettings[getCurrentChannel()].channelMin = oldMax;
    }
    ui->servoCenterAngleSlider->setInvertedAppearance(checked);
    ui->servoCenterAngleSlider->setInvertedControls(checked);
    ui->servoMinAngleSlider->setInvertedAppearance(checked);
    ui->servoMinAngleSlider->setInvertedControls(checked);
    ui->servoMaxAngleSlider->setInvertedAppearance(checked);
    ui->servoMaxAngleSlider->setInvertedControls(checked);

    if (ui->reverseCheckbox->isChecked()) {
        ui->servoMaxAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelMax);
        ui->servoCenterAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelNeutral);
        ui->servoMinAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelMin);
    } else {
        ui->servoMinAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelMin);
        ui->servoCenterAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelNeutral);
        ui->servoMaxAngleSlider->setValue(m_actuatorSettings[getCurrentChannel()].channelMax);
    }
}
