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

#include "execution/composerscheduler.h"

#include <QDebug>

#include "model/composermodel.h"
#include "model/connection.h"
#include "model/node.h"
#include "composerexecutor.h"


ComposerScheduler::ComposerScheduler(const ComposerModel *model, QObject *parent) :
    QObject(parent),
    _executors(),
    _model(model),
    _executionList(),
    _connections(),
    _unreachableNodes(),
    _processedNodes(),
    _cancelled(false),
    _keepProcessing(false)
{
    _settings.cacheData = true;
    _settings.useMultiThreading = true;
    _settings.useOptimalThreadsCount = true;
    _settings.fixedThreadsCount = QThread::idealThreadCount();

    connect(_model, SIGNAL(connectionAdded(const Connection*)),
                    SLOT(onConnectionAdded(const Connection*)));
    connect(_model, SIGNAL(connectionRemoved(const Connection*)),
                    SLOT(onConnectionRemoved(const Connection*)));

    processNexts();
}

void ComposerScheduler::setSettings(const ExecutorSettings &settings)
{
    _settings = settings;

    #warning TODO recompute everything
}

const ExecutorSettings &ComposerScheduler::getSettings() const
{
    return _settings;
}

void ComposerScheduler::prepareExecution(const QList<Node *> &nodes,
                                         const QList<Connection *> &connections)
{
    _connections = connections;

    QList<Node *> nodesToProcess = nodes;

    do
    {
        QMutableListIterator<Node *> iterator(nodesToProcess);
        while(iterator.hasNext())
        {
            iterator.next();
            Node *nodeToProcess = iterator.value();

            // For each remaining node, check whether all its inputs are available, i.e. we can
            // process it now

            bool allInputsProcessed = true;
            foreach(Plug *input, nodeToProcess->getInputs())
            {
                if(PlugType::isInputPluggable(input->getDefinition().type) == PlugType::ManualOnly)
                {
                    // Plug can't be connected, so it is always valid
                    continue;
                }

                // Find the connection to this input
                const Connection *connection = _model->findConnectionToInput(input);

                // We have found the connection, now find the previous node
                Node *previousNode = NULL;
                if(connection)
                {
                    foreach(Node *node, nodes)
                    {
                        if(node->hasOutput(connection->getOutput()))
                        {
                            previousNode = node;
                            break;
                        }
                    }
                }

                if(_unreachableNodes.contains(previousNode) ||
                   (previousNode == NULL && PlugType::isInputPluggable(input->getDefinition().type) == PlugType::Mandatory))
                {
                    // Previous node is unreachable or mandatory input is not connected,
                    // there is no way we can process the node
                    _unreachableNodes << iterator.value();
                    iterator.remove();
                    allInputsProcessed = false;
                    break; // Don't bother checking other plugs
                }
                else if(previousNode != NULL && not _executionList.contains(previousNode))
                {
                    // Output of previous node has not been processed yet
                    allInputsProcessed = false;
                    break; // Don't bother checking other plugs
                }
            }

            if(allInputsProcessed)
            {
                // All inputs of node have been processed, we can process it now !
                _executionList.enqueue(nodeToProcess);
                iterator.remove();
            }
        }
    } while(not nodesToProcess.isEmpty());

    _initialExecutionList = _executionList;
}

void ComposerScheduler::onConnectionRemoved(const Connection *connection)
{

}

void ComposerScheduler::onConnectionAdded(const Connection *connection)
{
    processNexts();
}

#if 0
void ComposerScheduler::cancel()
{
    // Just tag the scheduler as cancelled, and wait for an execute() or onNodeProcessed() slot call
    _cancelled = true;
}

void ComposerScheduler::execute()
{
    if(_cancelled)
    {
        // Scheduler was cancelled even before it started :(
        deleteLater();
        return;
    }

    foreach(Node *node, _unreachableNodes)
    {
        node->signalProcessUnavailable();
    }

    processNextIfPossible();
}
#endif

void ComposerScheduler::onNodeProcessed(bool success,
                                        const Properties &outputs,
                                        bool keepProcessing)
{
    if(success)
    {
        ComposerExecutor *executor = qobject_cast<ComposerExecutor *>(sender());
        if(executor)
        {
            _executors.removeAll(executor);
            _processedNodes.insert(executor->getNode(), outputs);
            processNexts();
        }
        else
        {
            qCritical() << "Sender is not a ComposerExecutor instance";
        }
    }
    else
    {

    }

    /*if(_cancelled)
    {
        deleteLater();
        return;
    }

    _keepProcessing |= keepProcessing;

    Node *processedNode = _executionList.dequeue();
    if(success)
    {
        _processedNodes[processedNode] = outputs;
    }

    processNextIfPossible();*/
}

bool ComposerScheduler::makeInputs(const Node *node, Properties &inputs)
{
    foreach(Plug *plug, node->getInputs())
    {
        QString plugName = plug->getDefinition().name;

        // Find whether this plug is connected
        const Connection *connection = _model->findConnectionToInput(plug);
        if(connection)
        {
            // This plug is connected, get the output value of the previous node, if available
            bool nodeProcessed = false;
            QMapIterator<const Node *, Properties> iterator(_processedNodes);
            while(iterator.hasNext())
            {
                iterator.next();

                if(iterator.key()->hasOutput(connection->getOutput()))
                {
                    // The node providing the output has been processed, great !
                    nodeProcessed = true;
                    QString outputName = connection->getOutput()->getDefinition().name;
                    inputs.insert(plugName, iterator.value()[outputName]);
                }
            }

            if(not nodeProcessed)
            {
                // The input node couldn't be processed :(
                return false;
            }
        }
        else
        {
            // This plus is not connected, use the user-defined value
            inputs.insert(plugName, node->getProperties()[plugName]);
        }
    }

    return true;
}
#if 0
void ComposerScheduler::processNextIfPossible()
{
    if(_executionList.isEmpty())
    {
        if(_keepProcessing && not _initialExecutionList.isEmpty())
        {
            _executionList = _initialExecutionList;
            processNextIfPossible();
        }
        else
        {
            deleteLater();
        }
    }
    else
    {
        Properties inputs;
        if(makeInputs(_executionList.head(), inputs))
        {
            _executor->process(_executionList.head(), inputs);
        }
        else
        {
            onNodeProcessed(false, Properties(), false);
        }
    }
}
#endif


quint16 ComposerScheduler::maxThreads() const
{
    if(_settings.useMultiThreading)
    {
        if(_settings.useOptimalThreadsCount)
        {
            return QThread::idealThreadCount();
        }
        else
        {
            return _settings.fixedThreadsCount;
        }
    }
    else
    {
        return 1;
    }
}

void ComposerScheduler::processNexts()
{
    while(_executors.count() < maxThreads())
    {
        bool nodeAddedForProcessing = false;

        QList<const Node *> potentialNodes = _model->getNodes();
        foreach(const Node *node, _processedNodes.keys())
        {
            // Remove nodes which have already been processed
            potentialNodes.removeAll(node);
        }
        foreach(ComposerExecutor *executor, _executors)
        {
            // Remove nodes which are being processed
            potentialNodes.removeAll(executor->getNode());
        }

        foreach(const Node *node, potentialNodes)
        {
            // node, check whether all its inputs are available, i.e. we can process it now

            bool allInputsProcessed = true;
            foreach(Plug *input, node->getInputs())
            {
                if(PlugType::isInputPluggable(input->getDefinition().type) == PlugType::ManualOnly)
                {
                    // Plug can't be connected, so it is always valid
                    continue;
                }

                // Find the connection to this input
                const Connection *connection = _model->findConnectionToInput(input);

                // We have found the connection, now find the previous node
                Node *previousNode = NULL;
                if(connection)
                {
                    previousNode = _model->findOutputPlug(connection->getOutput());
                }

                if(previousNode == NULL)
                {
                    if(PlugType::isInputPluggable(input->getDefinition().type) == PlugType::Mandatory)
                    {
                        // Plug is not connected and it should be
                        allInputsProcessed = false;
                        break;
                    }
                    else
                    {
                        // Plug is not connected, but this is authorized
                    }
                }
                else if(!_processedNodes.contains(previousNode))
                {
                    // Plug is connected, but its input node has not been processed yet
                    allInputsProcessed = false;
                    break;
                }
            }

            if(allInputsProcessed)
            {
                // All inputs of node have been processed, we can process it now !
                Properties inputs;
                if(makeInputs(node, inputs))
                {
                    ComposerExecutor *executor = new ComposerExecutor(this);
                    connect(executor, SIGNAL(nodeProcessed(bool,Properties,bool)),
                            executor, SLOT(deleteLater()));
                    connect(executor, SIGNAL(nodeProcessed(bool,Properties,bool)),
                                      SLOT(onNodeProcessed(bool,Properties,bool)));
                    executor->process(node, inputs);

                    nodeAddedForProcessing = true;

                    _executors << executor;

                    break;
                }
                else
                {
                    //onNodeProcessed(false, Properties(), false);
                    #warning TBD
                }
            }
        }

        if(!nodeAddedForProcessing)
        {
            // We have iterated through all the node, and there is nothing more to be done
            return;
        }
    }
}
