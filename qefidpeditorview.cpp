#include "qefidpeditorview.h"
#include "helpers.h"

#include <QMap>
#include <QVariant>

QEFIDPEditorView::QEFIDPEditorView(QEFIDevicePath *dp, QWidget *parent)
    : QWidget(parent)
{
    m_dpTypeSelected = -1;
    m_dpSubtypeSelected = -1;

    m_topLevelLayout = new QFormLayout(this);
    m_topLevelLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_topLevelLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_topLevelLayout->setHorizontalSpacing(10);
    m_topLevelLayout->setVerticalSpacing(8);

    m_dpTypeSelector = new QComboBox(this);
    m_dpTypeSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_dpTypeSelector->addItem(convert_device_path_type_to_name(QEFIDevicePathType::DP_Hardware), QVariant(QEFIDevicePathType::DP_Hardware));
    m_dpTypeSelector->addItem(convert_device_path_type_to_name(QEFIDevicePathType::DP_ACPI), QVariant(QEFIDevicePathType::DP_ACPI));
    m_dpTypeSelector->addItem(convert_device_path_type_to_name(QEFIDevicePathType::DP_Message), QVariant(QEFIDevicePathType::DP_Message));
    m_dpTypeSelector->addItem(convert_device_path_type_to_name(QEFIDevicePathType::DP_Media), QVariant(QEFIDevicePathType::DP_Media));
    m_dpTypeSelector->addItem(convert_device_path_type_to_name(QEFIDevicePathType::DP_BIOSBoot), QVariant(QEFIDevicePathType::DP_BIOSBoot));
    if (dp != nullptr) {
        m_dpTypeSelected = m_dpTypeSelector->findData(QVariant(dp->type()));
    }
    m_dpTypeSelector->setCurrentIndex(m_dpTypeSelected);
    connect(m_dpTypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QEFIDPEditorView::dpTypeComboBoxCurrentIndexChanged);

    m_dpSubtypeSelector = new QComboBox(this);
    m_dpSubtypeSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // m_dpSubtypeSelector->setPlaceholderText(tr("Device Path subtype"));
    if (dp != nullptr) {
        QList<quint8> subtypes = enum_device_path_subtype(dp->type());
        for (const auto &subtype : std::as_const(subtypes)) {
            m_dpSubtypeSelector->addItem(convert_device_path_subtype_to_name(dp->type(), subtype), QVariant(subtype));
        }
        m_dpSubtypeSelected = m_dpSubtypeSelector->findData(QVariant(dp->subType()));
    }
    m_dpSubtypeSelector->setCurrentIndex(m_dpSubtypeSelected);
    connect(m_dpSubtypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QEFIDPEditorView::dpSubtypeComboBoxCurrentIndexChanged);

    m_topLevelLayout->addRow("Device Path type:", m_dpTypeSelector);
    m_topLevelLayout->addRow("Device Path subtype:", m_dpSubtypeSelector);

    // Add edit view
    m_currentWidgets = constructDPEditView(dp);
    for (const auto &w : std::as_const(m_currentWidgets)) {
        m_topLevelLayout->addRow(w.first, w.second);
    }

    setLayout(m_topLevelLayout);
}

QEFIDPEditorView::~QEFIDPEditorView()
{
    if (m_topLevelLayout != nullptr) {
        m_topLevelLayout->deleteLater();
        m_topLevelLayout = nullptr;
    }
}

#include <QDebug>

QVariant retrieveDPEditComponent(enum QEFIDPEditType type, QWidget *widget)
{
    QVariant v;
    switch (type) {
    case QEFIDPEditType::EditType_Text:
    case QEFIDPEditType::EditType_Path: {
        QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
        if (edit != nullptr) {
            v = QVariant(edit->text());
        }
    } break;
    case QEFIDPEditType::EditType_HexData: {
        QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
        if (edit != nullptr) {
            v = QVariant(QByteArray::fromHex(edit->text().toLatin1()));
        }
    } break;
    case QEFIDPEditType::EditType_UUID: {
        QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
        if (edit != nullptr) {
            v = QVariant(QUuid(edit->text()));
        }
    } break;
    case QEFIDPEditType::EditType_Number:
    case QEFIDPEditType::EditType_HexNumber: {
        QSpinBox *edit = dynamic_cast<QSpinBox *>(widget);
        if (edit != nullptr) {
            v = QVariant(edit->value());
        }
    } break;
    case QEFIDPEditType::EditType_Enum: {
        QComboBox *edit = dynamic_cast<QComboBox *>(widget);
        if (edit != nullptr) {
            v = edit->currentData();
        }
    } break;
    default: {
        QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
        if (edit != nullptr) {
            v = QVariant(edit->text());
        }
    } break;
    }
    qDebug() << v;

    return v;
}

QEFIDevicePath *QEFIDPEditorView::getDevicePath()
{
    // Get type and subtype
    int type = (m_dpTypeSelector != nullptr ? m_dpTypeSelector->itemData(m_dpTypeSelected).toInt() : 0);
    int subtype = (m_dpSubtypeSelector != nullptr ? m_dpSubtypeSelector->itemData(m_dpSubtypeSelected).toInt() : 0);

    if (type == 0 || subtype == 0)
        return nullptr;

    // Collect values from widgets
    QMap<QString, QVariant> values;
    for (const auto &w : std::as_const(m_currentWidgets)) {
        // Get the field type from the dummy device path
        QEFIDevicePath *dummy = create_dummy_device_path((QEFIDevicePathType)type, (quint8)subtype);
        if (dummy == nullptr)
            continue;

        QList<QPair<QString, enum QEFIDPEditType>> fieldTypes = convert_device_path_types(dummy);
        delete dummy;

        // Find the edit type for this field
        enum QEFIDPEditType editType = QEFIDPEditType::EditType_Text;
        for (const auto &ft : std::as_const(fieldTypes)) {
            if (ft.first == w.first) {
                editType = ft.second;
                break;
            }
        }

        QVariant data = retrieveDPEditComponent(editType, w.second);
        if (!data.isNull()) {
            values[w.first] = data;
        }
    }

    // Create device path from values
    return create_device_path_from_values((QEFIDevicePathType)type, (quint8)subtype, values);
}

void QEFIDPEditorView::dpTypeComboBoxCurrentIndexChanged(int index)
{
    if (m_dpSubtypeSelector != nullptr && m_dpTypeSelected != index) {
        // Change the subtype
        m_dpSubtypeSelector->clear();
        int type = (m_dpTypeSelector != nullptr ? m_dpTypeSelector->itemData(index).toInt() : 0);
        QList<quint8> subtypes = enum_device_path_subtype((enum QEFIDevicePathType)type);
        for (const auto &subtype : std::as_const(subtypes)) {
            m_dpSubtypeSelector->addItem(convert_device_path_subtype_to_name((enum QEFIDevicePathType)type, subtype), QVariant(subtype));
        }

        // If we have subtypes, select the first one and create fields
        if (!subtypes.isEmpty()) {
            m_dpSubtypeSelector->setCurrentIndex(0);
            // Manually trigger field creation since setCurrentIndex might not emit signal if already at 0
            int subtype = m_dpSubtypeSelector->itemData(0).toInt();
            QList<QPair<QString, QWidget *>> widgets = constructDPEditView((QEFIDevicePathType)type, (quint8)subtype & 0xFF);
            while (m_topLevelLayout->rowCount() > 2)
                m_topLevelLayout->removeRow(2);
            m_currentWidgets = widgets;
            for (const auto &w : std::as_const(m_currentWidgets)) {
                m_topLevelLayout->addRow(w.first, w.second);
            }
            m_dpSubtypeSelected = 0;
        }
    }
    m_dpTypeSelected = index;
}

void QEFIDPEditorView::dpSubtypeComboBoxCurrentIndexChanged(int index)
{
    if (m_dpTypeSelector != nullptr && m_dpSubtypeSelected != index) {
        // Change the page
        int type = (m_dpTypeSelector != nullptr ? m_dpTypeSelector->itemData(m_dpTypeSelected).toInt() : 0);
        int subtype = (m_dpTypeSelector != nullptr ? m_dpSubtypeSelector->itemData(index).toInt() : 0);
        QList<QPair<QString, QWidget *>> widgets = constructDPEditView((QEFIDevicePathType)type, (quint8)subtype & 0xFF);
        while (m_topLevelLayout->rowCount() > 2)
            m_topLevelLayout->removeRow(2);
        m_currentWidgets = widgets;
        for (const auto &w : std::as_const(m_currentWidgets)) {
            m_topLevelLayout->addRow(w.first, w.second);
        }
    }
    m_dpSubtypeSelected = index;
}

QList<QPair<QString, QWidget *>> QEFIDPEditorView::constructDPEditView(QEFIDevicePath *dp)
{
    if (dp == nullptr)
        return {};

    enum QEFIDevicePathType type = dp->type();
    quint8 subType = dp->subType();
    QList<QPair<QString, QWidget *>> widgets = constructDPEditView(type, subType);

    // Populate fields with existing values
    QList<QPair<QString, enum QEFIDPEditType>> fieldTypes = convert_device_path_types(dp);
    QMap<QString, QVariant> fieldValues;

    // Extract values from device path (simplified - would need full implementation)
    // For now, we'll just create empty widgets and let user fill them

    return widgets;
}

QWidget *constructDPEditComponent(enum QEFIDPEditType type)
{
    QWidget *w;
    switch (type) {
    case QEFIDPEditType::EditType_Text:
    case QEFIDPEditType::EditType_Path: // TODO: Add a Browse button
    case QEFIDPEditType::EditType_HexData:
    case QEFIDPEditType::EditType_UUID: {
        QLineEdit *edit = new QLineEdit;
        w = (QWidget *)edit;
    } break;
    case QEFIDPEditType::EditType_Number:
    case QEFIDPEditType::EditType_HexNumber: {
        QSpinBox *edit = new QSpinBox;
        edit->setRange(0, INT_MAX);
        w = (QWidget *)edit;
    } break;
    case QEFIDPEditType::EditType_Enum: {
        QComboBox *edit = new QComboBox;
        w = (QWidget *)edit;
    } break;
    default:
        w = (QWidget *)new QLineEdit;
        break;
    }
    return w;
}

QList<QPair<QString, QWidget *>> QEFIDPEditorView::constructDPEditView(enum QEFIDevicePathType type, quint8 subType)
{
    QList<QPair<QString, QWidget *>> widgets;

    // Create a dummy device path to get field definitions
    QEFIDevicePath *dummy = create_dummy_device_path(type, subType);
    if (dummy == nullptr)
        return widgets;

    QList<QPair<QString, enum QEFIDPEditType>> fieldTypes = convert_device_path_types(dummy);

    for (const auto &t : std::as_const(fieldTypes)) {
        QWidget *w = constructDPEditComponent(t.second);

        // Apply size policy to ensure widgets fill the available width
        if (w != nullptr) {
            w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

        widgets << QPair<QString, QWidget *>(t.first, w);

        // Set up enum values for specific fields
        if (type == QEFIDevicePathType::DP_Media && subType == QEFIDevicePathMediaSubType::MEDIA_HD) {
            if (t.first == "Format") {
                QComboBox *comboBox = dynamic_cast<QComboBox *>(w);
                if (comboBox != nullptr) {
                    comboBox->addItem(tr("GPT"), QVariant(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT));
                    comboBox->addItem(tr("PCAT"), QVariant(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::PCAT));
                }
            }
            if (t.first == "Signature Type") {
                QComboBox *comboBox = dynamic_cast<QComboBox *>(w);
                if (comboBox != nullptr) {
                    comboBox->addItem(tr("None"), QVariant(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::NONE));
                    comboBox->addItem(tr("MBR"), QVariant(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::MBR));
                    comboBox->addItem(tr("GUID"), QVariant(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID));
                }
            }
        }
    }

    delete dummy;
    return widgets;
}
