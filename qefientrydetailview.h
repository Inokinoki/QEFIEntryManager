#ifndef QEFIENTRYDETAILVIEW_H
#define QEFIENTRYDETAILVIEW_H

#include <QWidget>

#include <QTabWidget>
#include <QFormLayout>
#include <QLabel>

#include "qefientry.h"

#include "qefientrydpdetailview.h"

class QEFIEntryDetailBriefView: public QWidget
{
    Q_OBJECT

    QEFIEntry &m_entry;

    QFormLayout *m_briefLayout;
public:
    QEFIEntryDetailBriefView(QEFIEntry &entry, QWidget *parent = nullptr);
    ~QEFIEntryDetailBriefView();
};

class QEFIEntryDetailView: public QWidget
{
    Q_OBJECT

    QEFIEntry &m_entry;

    QTabWidget *m_tab;

    QBoxLayout *m_topLevelLayout;
public:
    QEFIEntryDetailView(QEFIEntry &entry, QWidget *parent = nullptr);
    ~QEFIEntryDetailView();
};

#endif // QEFIENTRYDETAILVIEW_H