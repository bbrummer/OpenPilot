#include "OSGViewport.hpp"

#include "OSGNode.hpp"
#include "OSGCamera.hpp"
#include "QuickWindowViewer.hpp"
#include "Utility.hpp"

#include <osg/Node>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/Optimizer>

#include <osgEarth/MapNode>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>

#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QDebug>

#include <QThread>
#include <QApplication>

namespace osgQtQuick {

/*

Why not use setRenderTarget

export OSG_MULTIMONITOR_MULTITHREAD_WIN32_NVIDIA_WORKAROUND=On
export OSG_NOTIFY_LEVEL=DEBUG
http://trac.openscenegraph.org/projects/osg//wiki/Support/TipsAndTricks

Switch to 5.4 and use this :http://doc-snapshot.qt-project.org/qt5-5.4/qquickframebufferobject.html#details
http://doc-snapshot.qt-project.org/qt5-5.4/qtquick-visualcanvas-scenegraph.html#scene-graph-and-rendering
http://www.kdab.com/opengl-in-qt-5-1-part-1/

THIS MENTIONS GLUT...
http://www.multigesture.net/articles/how-to-compile-openscenegraph-2-x-using-mingw/
http://www.mingw.org/category/wiki/opengl

INTERESTING : http://qt-project.org/forums/viewthread/24535

https://groups.google.com/forum/#!msg/osg-users/HDabWUVaR2w/C6rPKKeKoYkJ

http://trac.openscenegraph.org/projects/osg/wiki/Community/OpenGL-ES

http://upstream.rosalinux.ru/changelogs/openscenegraph/3.2.0/changelog.html

MESA https://groups.google.com/forum/#!topic/osg-users/_2MqkA-wH1Q

GL version https://groups.google.com/forum/#!topic/osg-users/UiyYQ0Fw7MQ

http://trac.openscenegraph.org/projects/osg//wiki/Support/PlatformSpecifics/Mingw

Qt & OpenGL
http://blog.qt.digia.com/blog/2009/12/16/qt-graphics-and-performance-an-overview/
http://blog.qt.digia.com/blog/2010/01/06/qt-graphics-and-performance-opengl/
*/

// BUGS
// if resizing a gadget below the viewport, the viewport will stop shrinking at 64 pixel
// but it is possible to continue resizing the gadget and the gadget decorator will disapear behing the GL view
// will

// Errors/Freeze/Crashes:

// BLANK : Error: cannot draw stage due to undefined viewport.
// ABNORMAL TERMINATION : createDIB: CreateDIBSection failed.

/*
CRASH
osgQtQuick::OSGViewport : Update called for a item without content
VERTEX glCompileShader "atmos_vertex_main" FAILED
VERTEX glCompileShader "main(vert)" FAILED
FRAGMENT glCompileShader "main(frag)" FAILED
glLinkProgram "SimpleSky Scene Lighting" FAILED
Program "SimpleSky Scene Lighting" infolog:
Vertex info
-----------
An error occurred

Fragment info
-------------
An error occurred

[osgEarth]* [VirtualProgram] Program link failure!
VERTEX glCompileShader "main(vert)" FAILED
FRAGMENT glCompileShader "main(frag)" FAILED
glLinkProgram "osgEarth.ModelNode" FAILED
Program "osgEarth.ModelNode" infolog:
Vertex info
-----------
An error occurred

Fragment info
-------------
An error occurred

[osgEarth]* [VirtualProgram] Program link failure!
*/

/*
osgQtQuick::OSGViewport : Update called for a item without content
VERTEX glCompileShader "atmos_vertex_main" FAILED
VERTEX glCompileShader "main(vert)" FAILED
FRAGMENT glCompileShader "main(frag)" FAILED
glLinkProgram "SimpleSky Scene Lighting" FAILED
Program "SimpleSky Scene Lighting" infolog:
Vertex info
-----------
An error occurred

Fragment info
-------------
An error occurred

[osgEarth]* [VirtualProgram] Program link failure!
Warning: detected OpenGL error 'out of memory' at After Renderer::compile
terminate called after throwing an instance of 'std::bad_alloc'
  what():  std::bad_alloc

This application has requested the Runtime to terminate it in an unusual way.
Please contact the application's support team for more information.
*/


struct OSGViewport::Hidden : public QObject
{
    Q_OBJECT

    struct PreDraw: public osg::Camera::DrawCallback {
    public:
        PreDraw(Hidden *h);

        void operator ()(osg::RenderInfo &renderInfo) const;

        mutable Hidden *h;
    };
    friend struct PreDraw;

    struct PostDraw: public osg::Camera::DrawCallback {
    public:
        PostDraw(Hidden *h);

        void operator ()(osg::RenderInfo &renderInfo) const;

        mutable Hidden *h;
    };
    friend struct PostDraw;

public:

    Hidden(OSGViewport *quickItem) : QObject(quickItem),
        window(0),
        quickItem(quickItem),
        sceneData(0),
        camera(0),
        drawingMode(Native),
        fbo(0),
        texture(0),
        textureNode(0)
    {
        initOSG();
        acceptQuickItem();
        acceptDrawingMode(drawingMode);
    }

    ~Hidden() {
        if (fbo) {
            delete fbo;
        }
        if (texture) {
            delete texture;
        }
        if (textureNode) {
            delete textureNode;
        }
        window = NULL;
        quickItem = NULL;
    }

    QPointF mousePoint(QMouseEvent *event) {
//        qreal x = 0.01 * (event->x() - quickItem->width() / 2);
//        qreal y = 0.01 * (event->y() - quickItem->height() / 2);
        qreal x = 2.0 * (event->x() - quickItem->width() / 2) / quickItem->width();
        qreal y = 2.0 * (event->y() - quickItem->height() / 2) / quickItem->height();
        return QPointF(x, y);
    }

    void setKeyboardModifiers(QInputEvent* event)
    {
        int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
        unsigned int mask = 0;
        if ( modkey & Qt::ShiftModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
        if ( modkey & Qt::ControlModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
        if ( modkey & Qt::AltModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
        view->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGViewport - acceptSceneData" << node;
        if (sceneData == node) {
            return true;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            acceptNode(sceneData->node());
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGViewport - acceptNode" << node;
        if (!node) {
            qWarning() << "OSGViewport - acceptNode - node is null";
            view->setSceneData(NULL);
            return true;
        }

        // expose option to turn optimzer on/off
        //osgUtil::Optimizer optimizer;
        //optimizer.optimize(node);

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (mapNode) {
            qDebug() << "OSGViewport - acceptNode - found map node" << mapNode;
            // TODO should not be done here
            // TODO will the AutoClipPlaneCullCallback be destroyed???
            view->getCamera()->setCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
        }

        osgEarth::Util::SkyNode *skyNode = osgQtQuick::findTopMostNodeOfType<osgEarth::Util::SkyNode>(node);
        if (skyNode) {
            qDebug() << "OSGViewport - acceptNode - found sky node" << skyNode;
            // TODO should not be done here
            skyNode->attach(view.get(), 0);
        }

        view->setSceneData(node);

        return true;
    }

    bool acceptDrawingMode(OSGViewport::DrawingMode mode) {
        qDebug() << "OSGViewport - acceptDrawingMode" << mode;
        if (drawingMode == mode) {
            return false;
        }

        drawingMode = mode;

        if (drawingMode == OSGViewport::Buffer) {
            quickItem->setFlag(QQuickItem::ItemHasContents, true);
            if(!preDraw.valid()) {
                preDraw = new PreDraw(this);
                view->getCamera()->setPreDrawCallback(preDraw.get());
            }
            if(!postDraw.valid()) {
                postDraw = new PostDraw(this);
                view->getCamera()->setPostDrawCallback(postDraw.get());
            }
        }
        else {
            //quickItem->setFlag(QQuickItem::ItemHasContents, true);
        }

        return true;
    }

    bool acceptCamera(OSGCamera *camera) {
        qDebug() << "OSGViewport - acceptCamera" << camera;
        if (this->camera == camera) {
            return true;
        }

        this->camera = camera;

        if (this->camera) {
            this->camera->installCamera(view.get());
            updateViewport();
        }

        return true;
    }

    void updateFBO()
    {
        qDebug() << "OSGViewport - updateFBO";

//        if (textureNode) {
//            delete textureNode;
//        }
        if (texture) {
            delete texture;
        }
        if (fbo) {
            delete fbo;
        }

        // TODO this generates OpenGL errors ...
        // TODO ModelView exhibits some Z fighting
        QRectF rect = quickItem->mapRectToItem(0, quickItem->boundingRect());
        QSize size(rect.size().toSize());
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        fbo = new QOpenGLFramebufferObject(size, format);
        texture = quickItem->window()->createTextureFromId(fbo->texture(), size);
        //textureNode = new QSGSimpleTextureNode();
        if (!textureNode) {
            textureNode = new QSGSimpleTextureNode();
        }
        textureNode->setRect(0, quickItem->height(), quickItem->width(), -quickItem->height());
        textureNode->setTexture(texture);
        textureNode->markDirty(QSGNode::DirtyGeometry);
        //        quickItem->update();
    }

    QQuickWindow *window;
    OSGViewport *quickItem;

    osg::ref_ptr<osgViewer::View> view;

    OSGNode *sceneData;

    OSGCamera *camera;

    OSGViewport::DrawingMode drawingMode;

    QOpenGLFramebufferObject *fbo;
    QSGTexture *texture;
    QSGSimpleTextureNode *textureNode;

    osg::ref_ptr<PreDraw> preDraw;
    osg::ref_ptr<PostDraw> postDraw;

    static QtKeyboardMap keyMap;

public slots:

    void updateViewport() {
        qDebug() << "OSGViewport - updateViewport";// << quickItem;
        if (!quickItem->window()) {
            qDebug() << "OSGViewport - updateViewport - quick item has no window";
            return;
        }
        if (!camera) {
            qDebug() << "OSGViewport - updateViewport - no camera";
            return;
        }
        qDebug() << "OSGViewport - updateViewport " << QThread::currentThread() << QApplication::instance()->thread();

        if (drawingMode == OSGViewport::Native) {
            QRectF rect = quickItem->mapRectToItem(0, quickItem->boundingRect());
            qDebug() << "OSGViewport - updateViewport" << rect;
            camera->setViewport(view->getCamera(), rect.x(), quickItem->window()->height() - (rect.y() + rect.height()),
                    rect.width(), rect.height());
        }
        if (drawingMode == OSGViewport::Buffer) {
            QSize size(quickItem->boundingRect().size().toSize());
//            if(view->getCamera()->getGraphicsContext()) {
//                view->getCamera()->getGraphicsContext()->resized( 0, 0, size.width(), size.height() );
//            }
            qDebug() << "OSGViewport - updateViewport" << size;
            camera->setViewport(view->getCamera(), 0, 0, size.width(), size.height());
            if (size.width() > 0 && size.height() > 0) {
                if (!texture || texture->textureSize() != size) {
                    //updateFBO();
                    quickItem->update();
                }
            }
            else {
                qDebug() << "OSGViewport - updateViewport - invalid size";
            }
        }
    }

private slots:

    void onWindowChanged(QQuickWindow *window)
    {
        qDebug() << "OSGViewport - onWindowChanged" << window;
        // TODO if window is destroyed, the associated QuickWindowViewer should be destroyed too
        QuickWindowViewer *qwv;
        if (this->window && (qwv = QuickWindowViewer::instance(this->window))) {
            qDebug() << "OSGViewport - onWindowChanged" << "removing view";
            disconnect(this->window);
            qwv->compositeViewer()->removeView(view.get());
        }
        this->window = window;
        if (window && (qwv = QuickWindowViewer::instance(window))) {
            qDebug() << "OSGViewport - onWindowChanged" << "adding view";
            view->getCamera()->setGraphicsContext(qwv->graphicsContext());
            updateViewport();
            qwv->compositeViewer()->addView(view.get());
            connect(window, SIGNAL(widthChanged(int)), this, SLOT(updateViewport()));
            connect(window, SIGNAL(heightChanged(int)), this, SLOT(updateViewport()));
        }
    }

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGViewport - onNodeChanged" << node;
        if (view.valid()) {
            acceptNode(node);
        }
    }

private:
    void initOSG() {
        view = new osgViewer::View();
        // TODO will the StatsHandler be destroyed???
        view->addEventHandler(new osgViewer::StatsHandler());
        //viewer->addEventHandler(new osgGA::StateSetManipulator());
        //viewer->addEventHandler(new osgViewer::ThreadingHandler());
    }

    void acceptQuickItem() {
        Q_ASSERT(quickItem);

        qDebug() << "OSGViewport - acceptQuickItem" << quickItem << quickItem->window();

        //quickItem->setFlag(QQuickItem::ItemHasContents, true);
        connect(quickItem, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(onWindowChanged(QQuickWindow*)));
    }

};

QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

/* struct Hidden::PreDraw */

OSGViewport::Hidden::PreDraw::PreDraw(Hidden *h) : h(h) {}

void OSGViewport::Hidden::PreDraw::operator ()(osg::RenderInfo &/*renderInfo*/) const
{
    if (h->fbo) {
        if (!h->fbo->bind()) {
            qWarning() << "PreDraw - failed to bind fbo!";
        }
    }
    else {
        qCritical() << "PreDraw - no fbo!";
    }
}

/* struct Hidden::PostDraw */

OSGViewport::Hidden::PostDraw::PostDraw(Hidden *h) : h(h) {}

void OSGViewport::Hidden::PostDraw::operator ()(osg::RenderInfo &/*renderInfo*/) const
{
    if (h->fbo) {
        if (!h->fbo->bindDefault()) {
            qWarning() << "PostDraw - failed to unbind fbo!";
        }
    }
    else {
        qCritical() << "PostDraw - no fbo!";
    }
}

/* class OSGViewport */

OSGViewport::OSGViewport(QQuickItem *parent) : QQuickItem(parent), h(new Hidden(this))
{
    qDebug() << "OSGViewport - <init>";
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
    qDebug() << "OSGViewport - <destruct>";
}

OSGViewport::DrawingMode OSGViewport::drawingMode() const
{
    return h->drawingMode;
}

void OSGViewport::setDrawingMode(OSGViewport::DrawingMode mode)
{
    if (h->acceptDrawingMode(mode)) {
        emit drawingModeChanged(drawingMode());
    }
}

OSGNode *OSGViewport::sceneData()
{
    return h->sceneData;
}

void OSGViewport::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

OSGCamera* OSGViewport::camera()
{
    return h->camera;
}

void OSGViewport::setCamera(OSGCamera *camera)
{
    if (h->acceptCamera(camera)) {
        emit cameraChanged(camera);
    }
}

QColor OSGViewport::color() const
{
    const osg::Vec4 osgColor = h->view->getCamera()->getClearColor();
    return QColor::fromRgbF(osgColor.r(), osgColor.g(), osgColor.b(), osgColor.a());
}

void OSGViewport::setColor(const QColor &color)
{
    osg::Vec4 osgColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    if (h->view->getCamera()->getClearColor() != osgColor) {
        h->view->getCamera()->setClearColor(osgColor);
        emit colorChanged(color);
    }
}

void OSGViewport::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    qDebug() << "OSGViewport - geometryChanged" << newGeometry << oldGeometry;
    qDebug() << "OSGViewport - geometryChanged" <<  QThread::currentThread() << QApplication::instance()->thread();
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    qDebug() << "OSGViewport - geometryChanged - called parent";
    if (window() && newGeometry != oldGeometry) {
        h->updateViewport();
    }
}

void OSGViewport::mousePressEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseMotion(pos.x(), pos.y());
}

void OSGViewport::mouseReleaseEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    h->view->getEventQueue()->mouseScroll(
                event->orientation() == Qt::Vertical ?
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT) );

}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    h->setKeyboardModifiers( event );
    int value = h->keyMap.remapKey( event );
    h->view->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
//    if( _forwardKeyEvents )
//        inherited::keyPressEvent( event );
}

void OSGViewport::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->ignore();
    } else {
        h->setKeyboardModifiers(event);
        int value = h->keyMap.remapKey(event);
        h->view->getEventQueue()->keyRelease(value);
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
//    if( _forwardKeyEvents )
//        inherited::keyReleaseEvent( event );
}

QSGNode *OSGViewport::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);

    if (h->drawingMode != Buffer) {
        qWarning() << "OSGViewport - updatePaintNode - called in non Buffer mode";
        //h->quickItem->setFlag(QQuickItem::ItemHasContents, false);
        return NULL;
    }

//    if (!h->textureNode) {
//        h->textureNode = new QSGSimpleTextureNode();
//    }
    qDebug() << "OSGViewport - updatePaintNode - old node" << oldNode;
    h->updateFBO();
    qDebug() << "OSGViewport - updatePaintNode - new node" << h->textureNode;
    return h->textureNode;
}

} // namespace osgQtQuick

#include "OSGViewport.moc"
