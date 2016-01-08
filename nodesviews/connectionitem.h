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

#ifndef CONNECTIONITEM_H
#define CONNECTIONITEM_H

#include <QGraphicsLineItem>

#include <QUuid>

class ConnectionItem : public QGraphicsLineItem
{
    public:
        ConnectionItem(QGraphicsItem *parent = NULL);

        QPointF getOutput() const;

        void setOutput(const QPointF &output);

        void setInput(const QPointF &input);

        void setConnectionId(const QUuid &connectionId);

        const QUuid &getConnectionId() const;

    private:
        QUuid _connectionId;
};

#endif // CONNECTIONITEM_H
