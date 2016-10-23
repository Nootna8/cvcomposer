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

#include "composerexecutor.h"

#include <QDebug>
#include <QElapsedTimer>

#include "model/node.h"
#include "processor/abstractprocessor.h"
#include "processor/processorsfactory.h"


ComposerExecutor::ComposerExecutor(QObject *parent) :
    QThread(parent),
    _node(NULL),
    _processor(NULL),
    _inputs(),
    _outputs(),
    _keepProcessing(false),
    _duration(0),
    _error()
{
    connect(this, SIGNAL(finished()), SLOT(onFinished()));
}

void ComposerExecutor::process(const Node *node, const Properties &inputs)
{
    _error.clear();
    _node = node;
    _processor = ProcessorsFactory::createProcessor(node->getName());
    _inputs = inputs;
    start();

    qDebug() << "start   " << this << _node->getUserReadableName();
}

const Node *ComposerExecutor::getNode()
{
    return _node;
}

const Properties &ComposerExecutor::getInputs() const
{
    return _inputs;
}

const Properties &ComposerExecutor::getOutputs() const
{
    return _outputs;
}

qint64 ComposerExecutor::getDuration() const
{
    return _duration;
}

const QString &ComposerExecutor::getError() const
{
    return _error;
}

bool ComposerExecutor::getKeepProcessing() const
{
    return _keepProcessing;
}

void ComposerExecutor::run()
{
    QElapsedTimer timer;
    timer.start();

    try
    {
        _outputs = _processor->process(_inputs);
    }
    catch(const std::exception &exception)
    {
        qDebug() << "Exception when executing node"
                 << _node->getUserReadableName() << " : " << exception.what();
        _error = exception.what();
    }

    _duration = timer.elapsed();
}

void ComposerExecutor::onFinished()
{
    qDebug() << "finished" << this;

    _keepProcessing = _processor->getRealTimeProcessing();

    delete _processor;

    emit nodeProcessed();
}
