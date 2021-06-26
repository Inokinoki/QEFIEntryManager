#ifndef QEFIENTRYREBOOTVIEW_H
#define QEFIENTRYREBOOTVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

#include <QMap>
#include <QList>

#include "qefientrystaticlist.h"

class QEFIEntryRebootView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;

    QListWidget *m_entries;

    QBoxLayout *m_buttonLayout;
    QPushButton *m_rebootTargetButton;

    QLabel *m_bootTimeoutLabel;

    QMap<quint16, QEFIEntry> m_entryItems;
    quint16 m_timeout;

    QList<quint16> m_entryIds;
    int m_rebootItemIndex;
public:
    QEFIEntryRebootView(QWidget *parent = nullptr);
    ~QEFIEntryRebootView();

public slots:
    void entryChanged(int currentRow);
    void rebootClicked(bool checked);
};

#endif // QEFIENTRYREBOOTVIEW_H
