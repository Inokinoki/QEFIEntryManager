#ifndef QEFIENTRYVIEW_H
#define QEFIENTRYVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QLabel>

class QEFIEntryView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;
    QLabel *m_testLabel;
public:
    QEFIEntryView(QWidget *parent = nullptr);
    ~QEFIEntryView();
};

#endif // QEFIENTRYVIEW_H
