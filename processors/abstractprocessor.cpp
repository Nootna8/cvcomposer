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

#include "abstractprocessor.h"

#include <QDebug>


AbstractProcessor::AbstractProcessor() :
    _properties()
{

}

AbstractProcessor::~AbstractProcessor()
{

}

void AbstractProcessor::setProperties(const Properties &properties)
{
    _properties = properties;
}

quint8 AbstractProcessor::getNbInputs() const
{
    return _inputs.count();
}

const QList<PlugDefinition> &AbstractProcessor::getInputs()
{
    return _inputs;
}

quint8 AbstractProcessor::getNbOutputs() const
{
    return _outputs.count();
}

const QList<PlugDefinition> &AbstractProcessor::getOutputs()
{
    return _outputs;
}

Properties AbstractProcessor::process(const Properties &inputs)
{
    // Check that given inputs match the expected inputs
    QList<QString> inputNames = inputs.keys();
    qSort(inputNames);

    QList<QString> expectedInputNames;
    foreach(const PlugDefinition &plug, _inputs)
    {
        expectedInputNames << plug.name;
    }
    qSort(expectedInputNames);

    Q_ASSERT(inputNames == expectedInputNames);

    // Do the actual computing
    Properties outputs = processImpl(inputs);

    // Check that computed outputs match the expected ouputs
    QList<QString> outputNames = outputs.keys();
    qSort(outputNames);

    QList<QString> expectedOutputNames;
    foreach(const PlugDefinition &plug, _outputs)
    {
        expectedOutputNames << plug.name;
    }
    qSort(expectedOutputNames);

    Q_ASSERT(outputs.count() == getNbOutputs());

    // Everything is fine, give the outputs
    return outputs;
}

void AbstractProcessor::addInput(const PlugDefinition &definition)
{
    _inputs << definition;
}

void AbstractProcessor::addInput(const QString &userReadableName, PlugType::Enum type)
{
    addInput(makePlug(userReadableName, type));
}

void AbstractProcessor::addOutput(const PlugDefinition &definition)
{
    _outputs << definition;
}

void AbstractProcessor::addOutput(const QString &userReadableName, PlugType::Enum type)
{
    addOutput(makePlug(userReadableName, type));
}

QVariant AbstractProcessor::getProperty(const QString &name) const
{
    Properties::const_iterator iterator = _properties.find(name);
    if(iterator != _properties.end())
    {
        return iterator.value();
    }
    else
    {
        qCritical() << "AbstractProcessor::getProperty" << "No property named" << name;
        return QVariant();
    }
}

PlugDefinition AbstractProcessor::makePlug(const QString &userReadableName, PlugType::Enum type)
{
    PlugDefinition plug;
    plug.name = QString(userReadableName).remove(' ');
    plug.userReadableName = userReadableName;
    plug.type = type;

    return plug;
}
