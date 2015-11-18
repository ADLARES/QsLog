// Copyright (c) 2015, Axel Gembe <axel@gembe.net>
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLogDestWindow.h"
#include "QsLog.h"

#include <QColor>

const char* const QsLogging::WindowDestination::Type = "window";

QsLogging::WindowDestination::WindowDestination(size_t max_items) :
    max_items_(max_items)
{
}

QsLogging::WindowDestination::~WindowDestination()
{
}

void QsLogging::WindowDestination::write(const LogMessage& message)
{
    addEntry(message);
}

bool QsLogging::WindowDestination::isValid()
{
    return true;
}

QString QsLogging::WindowDestination::type() const
{
    return QString::fromLatin1(Type);
}

void QsLogging::WindowDestination::addEntry(const LogMessage& message)
{
    const int next_idx = static_cast<int>(data_.size());
    beginInsertRows(QModelIndex(), next_idx, next_idx);
    {
        QWriteLocker lock(&data_lock_);
        data_.push_back(message);
    }
    endInsertRows();

    if (max_items_ < std::numeric_limits<size_t>::max() && data_.size() > max_items_) {
        {
            QWriteLocker lock(&data_lock_);
            data_.pop_front();
        }
        /* Every item changed */
        const QModelIndex idx1 = index(0, 0);
        const QModelIndex idx2 = index(static_cast<int>(data_.size()), rowCount());
        emit dataChanged(idx1, idx2);
    }
}

void QsLogging::WindowDestination::clear()
{
    beginResetModel();
    {
        QWriteLocker lock(&data_lock_);
        data_.clear();
    }
    endResetModel();
}

QsLogging::LogMessage QsLogging::WindowDestination::at(size_t index)
{
    return data_[index];
}

int QsLogging::WindowDestination::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 3;
}

int QsLogging::WindowDestination::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    QReadLocker lock(&data_lock_);

    return static_cast<int>(data_.size());
}

QVariant QsLogging::WindowDestination::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        QReadLocker lock(&data_lock_);

        const LogMessage& item = data_.at(index.row());

        switch (index.column()) {
        case 0:
            return item.time.toLocalTime().toString();
        case 1:
            return LevelName(item.level);
        case 2:
            return item.message;
        case 100:
            return item.formatted;
        default:
            return QVariant();
        }

        return QString();
    }

    if (role == Qt::BackgroundColorRole) {
        QReadLocker lock(&data_lock_);

        const LogMessage& item = data_.at(index.row());

        switch (item.level)
        {
        case QsLogging::WarnLevel:
            return QVariant(QColor(255, 255, 128));
        case QsLogging::ErrorLevel:
            return QVariant(QColor(255, 128, 128));
        case QsLogging::FatalLevel:
            return QVariant(QColor(255, 0, 0));
        default:
            break;
        }
    }

    return QVariant();
}

QVariant QsLogging::WindowDestination::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Time");
        case 1: return tr("Level");
        case 2: return tr("Message");
        default: return QVariant();
        }
    }

    return QVariant();
}
