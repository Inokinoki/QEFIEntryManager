#include "qpartitioniodevice.h"
#include <QDebug>

QPartitionIODevice::QPartitionIODevice(const QString &devicePath, quint64 partitionOffset, quint64 partitionSize, QObject *parent)
    : QIODevice(parent)
    , m_file(new QFile(devicePath))
    , m_partitionOffset(partitionOffset)
    , m_partitionSize(partitionSize)
    , m_currentPos(0)
{
}

QPartitionIODevice::~QPartitionIODevice()
{
    if (isOpen()) {
        close();
    }
}

bool QPartitionIODevice::open(OpenMode mode)
{
    if (!m_file->open(mode)) {
        qWarning() << "Failed to open device file:" << m_file->fileName() << m_file->errorString();
        return false;
    }

    setOpenMode(mode);
    m_currentPos = 0;
    return true;
}

void QPartitionIODevice::close()
{
    if (m_file->isOpen()) {
        m_file->close();
    }
    setOpenMode(NotOpen);
}

bool QPartitionIODevice::seek(qint64 pos)
{
    if (pos < 0 || pos > (qint64)m_partitionSize) {
        return false;
    }

    qint64 absolutePos = m_partitionOffset + pos;
    if (!m_file->seek(absolutePos)) {
        return false;
    }

    m_currentPos = pos;
    return QIODevice::seek(pos);
}

qint64 QPartitionIODevice::size() const
{
    return m_partitionSize;
}

qint64 QPartitionIODevice::pos() const
{
    return m_currentPos;
}

bool QPartitionIODevice::atEnd() const
{
    return m_currentPos >= (qint64)m_partitionSize;
}

bool QPartitionIODevice::isSequential() const
{
    return false;
}

qint64 QPartitionIODevice::readData(char *data, qint64 maxSize)
{
    if (!m_file->isOpen()) {
        return -1;
    }

    // Ensure we're at the correct position
    qint64 absolutePos = m_partitionOffset + m_currentPos;
    if (m_file->pos() != absolutePos) {
        if (!m_file->seek(absolutePos)) {
            return -1;
        }
    }

    // Don't read beyond partition boundary
    qint64 remainingSize = m_partitionSize - m_currentPos;
    qint64 bytesToRead = qMin(maxSize, remainingSize);

    if (bytesToRead <= 0) {
        return 0;
    }

    qint64 bytesRead = m_file->read(data, bytesToRead);
    if (bytesRead > 0) {
        m_currentPos += bytesRead;
    }

    return bytesRead;
}

qint64 QPartitionIODevice::writeData(const char *data, qint64 maxSize)
{
    if (!m_file->isOpen()) {
        return -1;
    }

    // Ensure we're at the correct position
    qint64 absolutePos = m_partitionOffset + m_currentPos;
    if (m_file->pos() != absolutePos) {
        if (!m_file->seek(absolutePos)) {
            return -1;
        }
    }

    // Don't write beyond partition boundary
    qint64 remainingSize = m_partitionSize - m_currentPos;
    qint64 bytesToWrite = qMin(maxSize, remainingSize);

    if (bytesToWrite <= 0) {
        return 0;
    }

    qint64 bytesWritten = m_file->write(data, bytesToWrite);
    if (bytesWritten > 0) {
        m_currentPos += bytesWritten;
    }

    return bytesWritten;
}
