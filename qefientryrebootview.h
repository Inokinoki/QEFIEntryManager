#ifndef QEFIENTRYREBOOTVIEW_H
#define QEFIENTRYREBOOTVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QLabel>

class QEFIEntryRebootView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;
    QLabel *m_testLabel;
public:
    QEFIEntryRebootView(QWidget *parent = nullptr);
    ~QEFIEntryRebootView();
};

#endif // QEFIENTRYREBOOTVIEW_H
