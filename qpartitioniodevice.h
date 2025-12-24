#ifndef QPARTITIONIODEVICE_H
#define QPARTITIONIODEVICE_H

#include <QIODevice>
#include <QFile>
#include <QSharedPointer>

// Custom QIODevice that wraps a partition at a specific offset
class QPartitionIODevice : public QIODevice
{
    Q_OBJECT

public:
    explicit QPartitionIODevice(const QString &devicePath, quint64 partitionOffset, quint64 partitionSize, QObject *parent = nullptr);
    ~QPartitionIODevice();

    bool open(OpenMode mode) override;
    void close() override;
    bool seek(qint64 pos) override;
    qint64 size() const override;
    qint64 pos() const override;
    bool atEnd() const override;
    bool isSequential() const override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    QSharedPointer<QFile> m_file;
    quint64 m_partitionOffset;
    quint64 m_partitionSize;
    qint64 m_currentPos;
};

#endif // QPARTITIONIODEVICE_H
