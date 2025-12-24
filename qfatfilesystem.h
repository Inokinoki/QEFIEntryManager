#ifndef QFATFILESYSTEM_H
#define QFATFILESYSTEM_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QFile>
#include <QMetaType>

// FAT32 Boot Sector structure
#pragma pack(push, 1)
struct FAT32BootSector {
    quint8  jumpBoot[3];
    char    oemName[8];
    quint16 bytesPerSector;
    quint8  sectorsPerCluster;
    quint16 reservedSectorCount;
    quint8  numFATs;
    quint16 rootEntryCount;
    quint16 totalSectors16;
    quint8  media;
    quint16 FATSize16;
    quint16 sectorsPerTrack;
    quint16 numberHeads;
    quint32 hiddenSectors;
    quint32 totalSectors32;
    // FAT32 specific
    quint32 FATSize32;
    quint16 extFlags;
    quint16 FSVersion;
    quint32 rootCluster;
    quint16 FSInfo;
    quint16 backupBootSector;
    quint8  reserved[12];
    quint8  driveNumber;
    quint8  reserved1;
    quint8  bootSignature;
    quint32 volumeID;
    char    volumeLabel[11];
    char    fileSystemType[8];
};

struct FAT32DirectoryEntry {
    char    name[11];
    quint8  attr;
    quint8  NTRes;
    quint8  createTimeTenth;
    quint16 createTime;
    quint16 createDate;
    quint16 lastAccessDate;
    quint16 firstClusterHigh;
    quint16 writeTime;
    quint16 writeDate;
    quint16 firstClusterLow;
    quint32 fileSize;
};
#pragma pack(pop)

// File attributes
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

struct QFATFileInfo {
    QString name;
    QString path;
    quint32 size;
    quint32 firstCluster;
    bool isDirectory;
    bool isHidden;
    bool isSystem;

    QFATFileInfo() : size(0), firstCluster(0), isDirectory(false),
                     isHidden(false), isSystem(false) {}
};

class QFATFilesystem : public QObject
{
    Q_OBJECT

public:
    explicit QFATFilesystem(QObject *parent = nullptr);
    ~QFATFilesystem();

    bool open(const QString &devicePath, quint64 partitionOffset = 0);
    void close();
    bool isOpen() const;

    QVector<QFATFileInfo> listDirectory(const QString &path = "/");
    QByteArray readFile(const QString &path);
    QByteArray readFile(const QFATFileInfo &fileInfo);

    QString volumeLabel() const;
    QString fileSystemType() const;
    quint64 totalSize() const;
    quint64 freeSpace() const;

private:
    bool readBootSector();
    quint32 getNextCluster(quint32 currentCluster);
    QByteArray readCluster(quint32 clusterNumber);
    QVector<QFATFileInfo> readDirectoryCluster(quint32 clusterNumber, const QString &parentPath);
    QString parseShortName(const char *name);
    QString parseLongName(const QVector<FAT32DirectoryEntry> &entries, int startIndex);
    quint32 clusterToSector(quint32 cluster);

    QFile *m_device;
    quint64 m_partitionOffset;
    FAT32BootSector m_bootSector;
    QVector<quint32> m_FAT;
    bool m_isOpen;
    quint32 m_rootDirFirstCluster;
    quint32 m_FATStartSector;
    quint32 m_dataStartSector;
    quint32 m_totalClusters;
};

Q_DECLARE_METATYPE(QFATFileInfo)

#endif // QFATFILESYSTEM_H
