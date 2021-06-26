#ifndef QEFIENTRYVIEW_H
#define QEFIENTRYVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include <QMap>
#include <QList>

#include "qefientry.h"

class QEFIEntryView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;

    QListWidget *m_entries;

    QBoxLayout *m_buttonLayout;
    QPushButton *m_moveUpEntryButton;
    QPushButton *m_moveDownEntryButton;
    QPushButton *m_setCurrentButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;

    QMap<quint16, QEFIEntry> m_entryItems;
    QList<quint16> m_order;
public:
    QEFIEntryView(QWidget *parent = nullptr);
    ~QEFIEntryView();
};

#endif // QEFIENTRYVIEW_H
