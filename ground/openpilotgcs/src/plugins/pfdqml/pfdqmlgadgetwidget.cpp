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

#include "pfdqmlgadgetwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "utils/svgimageprovider.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>

PfdQmlGadgetWidget::PfdQmlGadgetWidget(QWindow *parent) :
    QQuickView(parent),
    m_speedUnit("m/s"),
    m_speedFactor(1.0),
    m_altitudeUnit("m"),
    m_altitudeFactor(1.0),
    m_actualPositionUsed(false),
    m_latitude(46.671478),
    m_longitude(10.158932),
    m_altitude(2000),
    m_openGLEnabled(false),
    m_terrainEnabled(false),
    m_earthFile("")
{
    setResizeMode(SizeRootObjectToView);

    // setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

    QStringList objectsToExport;
    objectsToExport <<
        "VelocityState" <<
        "PositionState" <<
        "AttitudeState" <<
        "AccelState" <<
        "VelocityDesired" <<
        "PathDesired" <<
        "AltitudeHoldDesired" <<
        "GPSPositionSensor" <<
        "GCSTelemetryStats" <<
        "SystemAlarms" <<
        "NedAccel" <<
        "FlightBatteryState" <<
        "ActuatorDesired" <<
        "TakeOffLocation" <<
        "PathPlan" <<
        "WaypointActive" <<
        "OPLinkStatus" <<
        "FlightStatus" <<
        "SystemStats" <<
        "StabilizationDesired" <<
        "VtolPathFollowerSettings" <<
        "HwSettings" <<
        "ManualControlCommand" <<
        "SystemSettings" <<
        "RevoSettings" <<
        "MagState" <<
        "FlightBatterySettings";

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    foreach(const QString &objectName, objectsToExport) {
        UAVObject *object = objManager->getObject(objectName);

        if (object) {
            engine()->rootContext()->setContextProperty(objectName, object);
        } else {
            qWarning() << "Failed to load object" << objectName;
        }
    }

    // to expose settings values
    engine()->rootContext()->setContextProperty("qmlWidget", this);
}

PfdQmlGadgetWidget::~PfdQmlGadgetWidget()
{}

void PfdQmlGadgetWidget::setQmlFile(QString fn)
{
    m_qmlFileName = fn;

    engine()->removeImageProvider("svg");
    SvgImageProvider *svgProvider = new SvgImageProvider(fn);
    engine()->addImageProvider("svg", svgProvider);

    engine()->clearComponentCache();

    // it's necessary to allow qml side to query svg element position
    engine()->rootContext()->setContextProperty("svgRenderer", svgProvider);
    engine()->setBaseUrl(QUrl::fromLocalFile(fn));

    qDebug() << Q_FUNC_INFO << fn;
    setSource(QUrl::fromLocalFile(fn));

    foreach(const QQmlError &error, errors()) {
        qDebug() << error.description();
    }
}

void PfdQmlGadgetWidget::setSpeedUnit(QString unit)
{
    if (m_speedUnit != unit) {
        m_speedUnit = unit;
        emit speedUnitChanged(unit);
    }
}

void PfdQmlGadgetWidget::setSpeedFactor(double factor)
{
    if (m_speedFactor != factor) {
        m_speedFactor = factor;
        emit speedFactorChanged(factor);
    }
}

void PfdQmlGadgetWidget::setAltitudeUnit(QString unit)
{
    if (m_altitudeUnit != unit) {
        m_altitudeUnit = unit;
        emit altitudeUnitChanged(unit);
    }
}

void PfdQmlGadgetWidget::setAltitudeFactor(double factor)
{
    if (m_altitudeFactor != factor) {
        m_altitudeFactor = factor;
        emit altitudeFactorChanged(factor);
    }
}

// Switch between PositionState UAVObject position
// and pre-defined latitude/longitude/altitude properties
void PfdQmlGadgetWidget::setActualPositionUsed(bool arg)
{
    if (m_actualPositionUsed != arg) {
        m_actualPositionUsed = arg;
        emit actualPositionUsedChanged(arg);
    }
}

void PfdQmlGadgetWidget::setLatitude(double arg)
{
    // not sure qFuzzyCompare is accurate enough for geo coordinates
    if (m_latitude != arg) {
        m_latitude = arg;
        emit latitudeChanged(arg);
    }
}

void PfdQmlGadgetWidget::setLongitude(double arg)
{
    if (m_longitude != arg) {
        m_longitude = arg;
        emit longitudeChanged(arg);
    }
}

void PfdQmlGadgetWidget::setAltitude(double arg)
{
    if (!qFuzzyCompare(m_altitude, arg)) {
        m_altitude = arg;
        emit altitudeChanged(arg);
    }
}
void PfdQmlGadgetWidget::setOpenGLEnabled(bool arg)
{
    m_openGLEnabled = arg;
    // TODO ???
    //setTerrainEnabled(m_terrainEnabled);
}

void PfdQmlGadgetWidget::setEarthFile(QString arg)
{
    if (m_earthFile != arg) {
        m_earthFile = arg;
        emit earthFileChanged(arg);
    }
}

void PfdQmlGadgetWidget::setTerrainEnabled(bool arg)
{
    bool wasEnabled = terrainEnabled();
    m_terrainEnabled = arg;
    if (wasEnabled != terrainEnabled()) {
        emit terrainEnabledChanged(terrainEnabled());
    }
}
