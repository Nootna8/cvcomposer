// Copyright 2016 Erwan MATHIEU <wawanbreton@gmail.com>
//
// This file is part of CvComposer.
//
// CvComposer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CvComposer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CvComposer.  If not, see <http://www.gnu.org/licenses/>.

#include "composerscene.h"

#include <QDebug>
#include <QMimeData>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>

#include "model/composermodel.h"
#include "model/connection.h"
#include "gui/nodestypesmanager.h"
#include "model/node.h"
#include "gui/genericnodeitem.h"
#include "gui/customitems.h"
#include "gui/connectionitem.h"
#include "gui/plugitem.h"


ComposerScene::ComposerScene(QObject *parent) :
    QGraphicsScene(parent),
    _model(new ComposerModel(this)),
    _editedConnection(),
    _editedNode(),
    _connections()
{
    _editedConnection.item = NULL;
    _editedConnection.plugInput = NULL;
    _editedConnection.plugOutput = NULL;

    _editedNode.item = NULL;

    connect(_model, SIGNAL(connectionAdded(Connection *)),
                    SLOT(onConnectionAdded(Connection *)));
    connect(_model, SIGNAL(connectionRemoved(Connection *)),
                    SLOT(onConnectionRemoved(Connection *)));
}

void ComposerScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    dragMoveEvent(event);
}

void ComposerScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if(event->mimeData()->hasFormat("application/x-cvcomposerfilter"))
    {
        event->acceptProposedAction();
    }
}

void ComposerScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if(event->mimeData()->hasFormat("application/x-cvcomposerfilter"))
    {
        event->acceptProposedAction();

        QString nodeType = QString::fromUtf8(event->mimeData()->data("application/x-cvcomposerfilter"));
        QString nodeName = QString::fromUtf8(event->mimeData()->data("application/x-cvcomposername"));

        Node *node = new Node(nodeType, nodeName);
        _model->addNode(node);

        GenericNodeItem *item = new GenericNodeItem(node);
        item->setPos(event->scenePos());
        addItem(item);
        _nodes << item;

        foreach(PlugItem *plugItem, item->getInputs() + item->getOutputs())
        {
            connect(plugItem, SIGNAL(positionChanged()), SLOT(onPlugItemPositionChanged()));
        }
    }
}

void ComposerScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        if(item)
        {
            if(item->type() == QGraphicsProxyWidget::Type)
            {
                // Let the parent do its job fully
                return QGraphicsScene::mousePressEvent(event);
            }
            else if(item->type() == CustomItems::Plug)
            {
                PlugItem *plug = static_cast<PlugItem *>(item);
                bool isInput = _model->findInputPlug(plug->getPlug()) != NULL;
                bool isOutput = _model->findOutputPlug(plug->getPlug()) != NULL;

                _editedConnection.item = new ConnectionItem();
                addItem(_editedConnection.item);

                // When editing an input node, edit its actual connection if there is one
                if(isInput)
                {
                    foreach(ConnectionItem *connectionItem, _connections)
                    {
                        Connection *connection = connectionItem->getConnection();
                        if(connection->getInput() == plug->getPlug())
                        {
                            // We have found an existing connection, edit it
                            _editedConnection.item->setOutput(connectionItem->getOutput());
                            _editedConnection.item->setInput(event->scenePos());
                            _editedConnection.plugOutput = connection->getOutput();
                            _editedConnection.fromOutput = true;
                            _model->removeConnection(connectionItem->getConnection());
                            return;
                        }
                    }
                }

                // We are not editing an existing connection
                if(isInput)
                {
                    _editedConnection.item->setOutput(event->scenePos());
                    _editedConnection.item->setInput(plug->mapToScene(QPointF(0, 0)));
                    _editedConnection.plugInput = plug->getPlug();
                    _editedConnection.fromOutput = false;
                }
                else if(isOutput)
                {
                    _editedConnection.item->setOutput(plug->mapToScene(QPointF(0, 0)));
                    _editedConnection.item->setInput(event->scenePos());
                    _editedConnection.plugOutput = plug->getPlug();
                    _editedConnection.fromOutput = true;
                }
                else
                {
                    qCritical("Selected plug not found in node inputs/outputs");
                }
            }
            else if(item->type() == CustomItems::Node)
            {
                event->widget()->setCursor(Qt::ClosedHandCursor);
                _editedNode.item = static_cast<GenericNodeItem *>(item);
                _editedNode.initClickPos = event->scenePos();
                _editedNode.initNodePose = _editedNode.item->pos();
            }
        }
    }
}

void ComposerScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QCursor cursor = Qt::ArrowCursor;

    #warning It is possible to connect an output to an input of the same node

    if(_editedConnection.item)
    {
        bool plugFound = false;
        PlugType::Enum baseType;
        if(_editedConnection.fromOutput)
        {
            baseType = _editedConnection.plugOutput->getDefinition().type;
        }
        else
        {
            baseType = _editedConnection.plugInput->getDefinition().type;
        }

        foreach(GenericNodeItem *nodeItem, _nodes)
        {
            const QList<PlugItem *> &plugItems = _editedConnection.fromOutput ? nodeItem->getInputs() : nodeItem->getOutputs();
            foreach(PlugItem *plugItem, plugItems)
            {
                PlugType::Enum plugType = plugItem->getPlug()->getDefinition().type;
                PlugType::Enum compatible = PlugType::getCompatibility(plugType);
                if(compatible == baseType)
                {
                    QPointF itemPos = plugItem->mapToScene(QPointF(0, 0));
                    qreal distance = (event->scenePos() - itemPos).manhattanLength();
                    if(distance < PlugItem::magnetRadius)
                    {
                        if(_editedConnection.fromOutput)
                        {
                            _editedConnection.item->setInput(plugItem->mapToScene(QPointF(0, 0)));
                            _editedConnection.plugInput = plugItem->getPlug();
                        }
                        else
                        {
                            _editedConnection.item->setOutput(plugItem->mapToScene(QPointF(0, 0)));
                            _editedConnection.plugOutput = plugItem->getPlug();
                        }

                        cursor = Qt::PointingHandCursor;
                        plugFound = true;
                    }
                }
            }
        }

        if(not plugFound)
        {
            if(_editedConnection.fromOutput)
            {
                _editedConnection.item->setInput(event->scenePos());
                _editedConnection.plugInput = NULL;
            }
            else
            {
                _editedConnection.item->setOutput(event->scenePos());
                _editedConnection.plugOutput = NULL;
            }
        }
    }
    else if(_editedNode.item)
    {
        cursor = Qt::ClosedHandCursor;

        _editedNode.item->setPos(_editedNode.initNodePose + (event->scenePos() - _editedNode.initClickPos));
    }
    else
    {
        // We are not editing anything
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        if(item)
        {
            if(item->type() == QGraphicsProxyWidget::Type)
            {
                // Let the parent do its job fully
                return QGraphicsScene::mouseMoveEvent(event);
            }
            else if(item->type() == CustomItems::Plug)
            {
                cursor = Qt::PointingHandCursor;
            }
            else if(item->type() == CustomItems::Node)
            {
                cursor = Qt::OpenHandCursor;
            }
        }
    }

    event->widget()->setCursor(cursor);
}

void ComposerScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if(_editedConnection.item)
    {
        if(_editedConnection.plugInput && _editedConnection.plugOutput)
        {
            _model->addConnection(_editedConnection.plugOutput, _editedConnection.plugInput);
        }

        removeItem(_editedConnection.item);
        delete _editedConnection.item;

        _editedConnection.item = NULL;
        _editedConnection.plugInput = NULL;
        _editedConnection.plugOutput = NULL;
    }
    else if(_editedNode.item)
    {
        _editedNode.item = NULL;
        event->widget()->setCursor(Qt::OpenHandCursor);
    }
    else
    {
        // Let the parent do its job fully
        return QGraphicsScene::mouseReleaseEvent(event);
    }
}

void ComposerScene::onConnectionAdded(Connection *connection)
{
    ConnectionItem *connectionItem = new ConnectionItem();
    connectionItem->setConnection(connection);

    foreach(const GenericNodeItem *nodeView, _nodes)
    {
        foreach(const PlugItem *plugItem, nodeView->getInputs())
        {
            if(plugItem->getPlug() == connection->getInput())
            {
                connectionItem->setInput(plugItem->mapToScene(QPointF(0, 0)));
                break;
            }
        }
        foreach(const PlugItem *plugItem, nodeView->getOutputs())
        {
            if(plugItem->getPlug() == connection->getOutput())
            {
                connectionItem->setOutput(plugItem->mapToScene(QPointF(0, 0)));
                break;
            }
        }
    }

    addItem(connectionItem);
    _connections << connectionItem;
}

void ComposerScene::onConnectionRemoved(Connection *connection)
{
    foreach(ConnectionItem *connectionItem, _connections)
    {
        if(connectionItem->getConnection() == connection)
        {
            removeItem(connectionItem);
            delete connectionItem;
            _connections.removeAll(connectionItem);
            break;
        }
    }
}

void ComposerScene::onPlugItemPositionChanged()
{
    // A plug item has moved, move the associated connections
    PlugItem *plugItem = qobject_cast<PlugItem *>(sender());
    if(plugItem)
    {
        const Plug *plug = plugItem->getPlug();
        foreach(ConnectionItem *connectionItem, _connections)
        {
            Connection *connection = connectionItem->getConnection();

            if(connection->getInput() == plug)
            {
                connectionItem->setInput(plugItem->mapToScene(QPointF(0, 0)));
            }
            else if(connection->getOutput() == plug)
            {
                connectionItem->setOutput(plugItem->mapToScene(QPointF(0, 0)));
            }
        }
    }
    else
    {
        qCritical() << "ComposerScene::onPlugItemPositionChanged" << "Sender is not a PlugItem";
    }
}
