#include "qefientrydpdetailview.h"
#include "helpers.h"


QEFIEntryDPDetailView::QEFIEntryDPDetailView(QEFIDevicePath *dp, QWidget *parent)
    : QWidget(parent)
{
    m_topLevelLayout = new QFormLayout(this);

    m_topLevelLayout->addRow("Device Path type:", new QLabel(
        convert_device_path_type_to_name(dp->type()) + " " +
        convert_device_path_subtype_to_name(dp->type(), dp->subType()))
    );
    m_topLevelLayout->addRow(QStringLiteral(""),
        new QLabel(QStringLiteral("%1 %1")
	    .arg(dp->type(), 2, 16, QLatin1Char('0'))
	    .arg(dp->subType(), 2, 16, QLatin1Char('0')))
    );
    // Parse device path and add more properties
    QList<QPair<QString, QString>> attrs = convert_device_path_attrs(dp);
    for (auto &i : attrs) {
        m_topLevelLayout->addRow(i.first + ":", new QLabel(i.second));
    }
    setLayout(m_topLevelLayout);
}

QEFIEntryDPDetailView::~QEFIEntryDPDetailView()
{
    if (m_topLevelLayout != nullptr) {
        m_topLevelLayout->deleteLater();
        m_topLevelLayout = nullptr;
    }
}
