#ifndef QEFIENTRYDETAILVIEW_H
#define QEFIENTRYDETAILVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QFormLayout>
#include <QLabel>

#include "qefientry.h"

class QEFIEntryDetailView: public QWidget
{
    Q_OBJECT

    QEFIEntry &m_entry;

    QBoxLayout *m_topLevelLayout;
    QFormLayout *m_briefLayout;
public:
    QEFIEntryDetailView(QEFIEntry &entry, QWidget *parent = nullptr);
    ~QEFIEntryDetailView();
};

#endif // QEFIENTRYDETAILVIEW_H