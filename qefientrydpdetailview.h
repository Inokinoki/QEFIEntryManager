#ifndef QEFIENTRYDPDETAILVIEW_H
#define QEFIENTRYDPDETAILVIEW_H

#include <QWidget>

#include <QFormLayout>
#include <QLabel>

#include <qefi.h>

class QEFIEntryDPDetailView: public QWidget
{
    Q_OBJECT

    QFormLayout *m_topLevelLayout;
public:
    QEFIEntryDPDetailView(QEFIDevicePath *dp, QWidget *parent = nullptr);
    ~QEFIEntryDPDetailView();
};

#endif // QEFIENTRYDPDETAILVIEW_H