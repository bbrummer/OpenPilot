#ifndef _H_OSGQTQUICK_OSGNODE_H_
#define _H_OSGQTQUICK_OSGNODE_H_

#include "Export.hpp"

#include <QObject>

namespace osg {
class Node;
} // namespace osg

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGNode : public QObject {
    Q_OBJECT

public:
    explicit OSGNode(QObject *parent = 0);
    virtual ~OSGNode();

    osg::Node *node() const;
    void setNode(osg::Node *node);

    virtual void realize();

signals:
    void nodeChanged(osg::Node *node) const;

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGNODE_H_