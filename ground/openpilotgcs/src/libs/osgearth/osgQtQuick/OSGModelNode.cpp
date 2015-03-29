#include "OSGModelNode.hpp"

#include <osg/Quat>

#include <osgEarth/MapNode>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <osgEarth/ElevationQuery>

#include <osgEarthSymbology/Style>
#include <osgEarthSymbology/ModelSymbol>

#include <osgEarthAnnotation/ModelNode>

#include <QDebug>

namespace osgQtQuick {
struct OSGModelNode::Hidden : public QObject {
    Q_OBJECT

    struct NodeUpdateCallback : public osg::NodeCallback {
public:
        NodeUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGModelNode *parent) : QObject(parent), self(parent), modelData(NULL), sceneData(NULL), dirty(false)
    {}

    ~Hidden()
    {}

    bool acceptModelData(OSGNode *node)
    {
        qDebug() << "OSGModelNode - acceptModelData" << node;
        if (modelData == node) {
            return false;
        }

        if (modelData) {
            disconnect(modelData);
        }

        modelData = node;

        if (modelData) {
            acceptModelNode(modelData->node());
            connect(modelData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onModelNodeChanged(osg::Node *)));
        }

        return true;
    }


    bool acceptModelNode(osg::Node *node)
    {
        qDebug() << "OSGModelNode acceptModelNode" << node;
        if (!node) {
            qWarning() << "OSGModelNode - acceptModelNode - node is null";
            return false;
        }

        if (!sceneData || !sceneData->node()) {
            qWarning() << "OSGModelNode - acceptModelNode - no scene data";
            return false;
        }

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
        if (!mapNode) {
            qWarning() << "OSGModelNode - acceptModelNode - scene data does not contain a map node";
            return false;
        }

        // establish the coordinate system we wish to use:
        // const osgEarth::SpatialReference* latLong = osgEarth::SpatialReference::get("wgs84");

// osgEarth::Config conf("style");
// conf.set("auto_scale", true);
// osgEarth::Symbology::Style style(conf);
        osgEarth::Symbology::Style style;

        // construct the symbology
        osgEarth::Symbology::ModelSymbol *modelSymbol = style.getOrCreate<osgEarth::Symbology::ModelSymbol>();
        modelSymbol->setModel(node);

        // make a ModelNode
        modelNode = new osgEarth::Annotation::ModelNode(mapNode, style);

        qDebug() << "OSGModelNode - acceptModelNode - model radius:" << modelNode->getBound().radius();

        if (!nodeUpdateCallback.valid()) {
            nodeUpdateCallback = new NodeUpdateCallback(this);
        }
        modelNode->addUpdateCallback(nodeUpdateCallback.get());

        mapNode->addChild(modelNode);

        self->setNode(modelNode);

        dirty = true;

        return true;
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGModelNode - acceptSceneData" << node;
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            // TODO find a better way
            if (modelData && modelData->node()) {
                acceptModelNode(modelData->node());
            }
            connect(sceneData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onSceneNodeChanged(osg::Node *)));
        }

        return true;
    }

    // TODO model update gets jitter if camera is thrown (i.e animated)
    void updateNode()
    {
        if (!dirty || !modelNode.valid()) {
            return;
        }
        dirty = false;
        modelNode->setPosition(clampToTerrain ? clampedPositionGeoPoint() : positionGeoPoint());
        modelNode->setLocalRotation(localRotation());
    }

    osgEarth::GeoPoint positionGeoPoint()
    {
        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
                                    position.x(), position.y(), position.z(), osgEarth::ALTMODE_ABSOLUTE);

        return geoPoint;
    }

    osgEarth::GeoPoint clampedPositionGeoPoint()
    {
        osgEarth::GeoPoint geoPoint = positionGeoPoint();

        osgEarth::MapNode *mapNode  = osgEarth::MapNode::findMapNode(sceneData->node());

        if (!mapNode) {
            qWarning() << "OSGModelNode - updatePositionClamped : scene data does not contain a map node";
            return geoPoint;
        }

        // establish an elevation query interface based on the features' SRS.
        osgEarth::ElevationQuery eq(mapNode->getMap());

        double elevation;
        if (eq.getElevation(geoPoint, elevation, 0.0)) {
            // TODO need to notify intoTerrainChanged...
            intoTerrain = ((position.z() - modelNode->getBound().radius()) < elevation);
            if (intoTerrain) {
                qDebug() << "OSGModelNode - updatePositionClamped : clamping" << position.z() - modelNode->getBound().radius() << "/" << elevation;
                geoPoint = osgEarth::GeoPoint(osgEarth::SpatialReference::get("wgs84"), position.x(), position.y(),
                                              position.z() + modelNode->getBound().radius(), osgEarth::ALTMODE_ABSOLUTE);
            }
        } else {
            qDebug() << "OSGModelNode - updatePositionClamped : failed to get elevation";;
        }
        return geoPoint;
    }

    osg::Quat localRotation()
    {
        osg::Quat q = osg::Quat(
            osg::DegreesToRadians(attitude.x()), osg::Vec3d(1, 0, 0),
            osg::DegreesToRadians(attitude.y()), osg::Vec3d(0, 1, 0),
            osg::DegreesToRadians(attitude.z()), osg::Vec3d(0, 0, 1));

        return q;
    }

    OSGModelNode *const self;

    OSGNode *modelData;
    OSGNode *sceneData;

    osg::ref_ptr<osgEarth::Annotation::ModelNode> modelNode;
    // osg::ref_ptr<osg::Node> cameraNode;
    osg::ref_ptr<NodeUpdateCallback> nodeUpdateCallback;

    bool dirty;

    bool clampToTerrain;

    QVector3D attitude;
    QVector3D position;

    bool intoTerrain;

private slots:

    void onModelNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode - onModelNodeChanged" << node;
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCallback.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCallback.get());
                }
            }
            if (node) {
                acceptModelNode(node);
            }
        }
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode - onSceneNodeChanged" << node;
        // TODO needs to be improved...
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCallback.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCallback.get());
                }
            }
            if (modelData->node()) {
                acceptModelNode(modelData->node());
            }
        }
    }
};

/* struct Hidden::NodeUpdateCallback */

void OSGModelNode::Hidden::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    h->updateNode();
    traverse(node, nv);
}

OSGModelNode::OSGModelNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGModelNode - <init>";
}

OSGModelNode::~OSGModelNode()
{
    qDebug() << "OSGModelNode - <destruct>";
}

OSGNode *OSGModelNode::modelData()
{
    return h->modelData;
}

void OSGModelNode::setModelData(OSGNode *node)
{
    if (h->acceptModelData(node)) {
        emit modelDataChanged(node);
    }
}

OSGNode *OSGModelNode::sceneData()
{
    return h->sceneData;
}

void OSGModelNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

bool OSGModelNode::clampToTerrain() const
{
    return h->clampToTerrain;
}

void OSGModelNode::setClampToTerrain(bool arg)
{
    if (h->clampToTerrain != arg) {
        h->clampToTerrain = arg;
        h->dirty = true;
        emit clampToTerrainChanged(clampToTerrain());
    }
}

QVector3D OSGModelNode::attitude() const
{
    return h->attitude;
}

void OSGModelNode::setAttitude(QVector3D arg)
{
    if (h->attitude != arg) {
        h->attitude = arg;
        h->dirty    = true;
        emit attitudeChanged(attitude());
    }
}

QVector3D OSGModelNode::position() const
{
    return h->position;
}

void OSGModelNode::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        h->dirty    = true;
        emit positionChanged(position());
    }
}

bool OSGModelNode::intoTerrain() const
{
    return h->intoTerrain;
}
} // namespace osgQtQuick

#include "OSGModelNode.moc"
