#include "qefidpeditorview.h"
#include "helpers.h"

#include <QVariant>

QEFIDPEditorView::QEFIDPEditorView(QEFIDevicePath *dp, QWidget *parent)
    : QWidget(parent)
{
    m_dpTypeSelected = -1;
    m_dpSubtypeSelected = -1;

    m_topLevelLayout = new QFormLayout(this);

    m_dpTypeSelector = new QComboBox(this);
    // m_dpTypeSelector->setPlaceholderText(tr("Device Path type"));
    // m_dpTypeSelector->addItem(
    //     convert_device_path_type_to_name(QEFIDevicePathType::DP_Hardware),
    //     QVariant(QEFIDevicePathType::DP_Hardware));
    // m_dpTypeSelector->addItem(
    //     convert_device_path_type_to_name(QEFIDevicePathType::DP_ACPI),
    //     QVariant(QEFIDevicePathType::DP_ACPI));
    // m_dpTypeSelector->addItem(
    //     convert_device_path_type_to_name(QEFIDevicePathType::DP_Message),
    //     QVariant(QEFIDevicePathType::DP_Message));
    m_dpTypeSelector->addItem(
        convert_device_path_type_to_name(QEFIDevicePathType::DP_Media),
        QVariant(QEFIDevicePathType::DP_Media));
    // m_dpTypeSelector->addItem(
    //     convert_device_path_type_to_name(QEFIDevicePathType::DP_BIOSBoot),
    //     QVariant(QEFIDevicePathType::DP_BIOSBoot));
    if (dp != nullptr) {
        m_dpTypeSelected = m_dpTypeSelector->findData(QVariant(dp->type()));
    }
    m_dpTypeSelector->setCurrentIndex(m_dpTypeSelected);
    connect(m_dpTypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QEFIDPEditorView::dpTypeComboBoxCurrentIndexChanged);

    m_dpSubtypeSelector = new QComboBox(this);
    // m_dpSubtypeSelector->setPlaceholderText(tr("Device Path subtype"));
    if (dp != nullptr) {
        QList<quint8> subtypes = enum_device_path_subtype(dp->type());
        for (const auto &subtype: std::as_const(subtypes)) {
            m_dpSubtypeSelector->addItem(convert_device_path_subtype_to_name(
                dp->type(), subtype),
                QVariant(subtype));
        }
        m_dpSubtypeSelected = m_dpSubtypeSelector->findData(QVariant(dp->subType()));
    }
    m_dpSubtypeSelector->setCurrentIndex(m_dpSubtypeSelected);
    connect(m_dpSubtypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QEFIDPEditorView::dpSubtypeComboBoxCurrentIndexChanged);

    m_topLevelLayout->addRow("Device Path type:", m_dpTypeSelector);
    m_topLevelLayout->addRow("Device Path subtype:", m_dpSubtypeSelector);

    // Add edit view
    m_currentWidgets = constructDPEditView(dp);
    for (const auto &w: std::as_const(m_currentWidgets)) {
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

QVariant retrieveDPEditComponent(enum QEFIDPEditType type, QWidget *widget) {
    QVariant v;
    switch (type)
    {
    case QEFIDPEditType::EditType_Text:
    case QEFIDPEditType::EditType_Path:
        {
            QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
            if (edit != nullptr) {
                v = QVariant(edit->text());
            }
        }
        break;
    case QEFIDPEditType::EditType_HexData:
        {
            QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
            if (edit != nullptr) {
                v = QVariant(QByteArray::fromHex(edit->text().toLatin1()));
            }
        }
        break;
    case QEFIDPEditType::EditType_UUID:
        {
            QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
            if (edit != nullptr) {
                v = QVariant(QUuid(edit->text()));
            }
        }
        break;
    case QEFIDPEditType::EditType_Number:
    case QEFIDPEditType::EditType_HexNumber:
        {
            QSpinBox *edit = dynamic_cast<QSpinBox *>(widget);
            if (edit != nullptr) {
                v = QVariant(edit->value());
            }
        }
        break;
    case QEFIDPEditType::EditType_Enum:
        {
            QComboBox *edit = dynamic_cast<QComboBox *>(widget);
            if (edit != nullptr) {
                v = edit->currentData();
            }
        }
        break;
    default:
        {
            QLineEdit *edit = dynamic_cast<QLineEdit *>(widget);
            if (edit != nullptr) {
                v = QVariant(edit->text());
            }
        }
        break;
    }
    qDebug() << v;

    return v;
}

QEFIDevicePath *QEFIDPEditorView::getDevicePath()
{
    // Instantiate the device path
    int type = (m_dpTypeSelector != nullptr ?
        m_dpTypeSelector->itemData(m_dpTypeSelected).toInt() : 0);
    int subtype = (m_dpTypeSelector != nullptr ?
        m_dpSubtypeSelector->itemData(m_dpSubtypeSelected).toInt() : 0);
    QEFIDevicePath *dp = nullptr;
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        break;
    case QEFIDevicePathType::DP_ACPI:
        break;
    case QEFIDevicePathType::DP_Message:
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subtype)
        {
        case QEFIDevicePathMediaSubType::MEDIA_HD:
            {
                quint32 partitionNumber;
                quint64 start;
                quint64 size;
                quint8 signature[16];
                quint8 format;
                quint8 signatureType;
                QString filepath;
                for (const auto &w: std::as_const(m_currentWidgets)) {
                    qDebug() << w.first;
                    // TODO: Int range
                    if (w.first == tr("Partition Num")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Number, w.second);
                        if (data.isNull()) return dp;
                        partitionNumber = data.toInt();
                    } else if (w.first == tr("Start")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Number, w.second);
                        if (data.isNull()) return dp;
                        start = data.toInt();
                    } else if (w.first == tr("Size")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Number, w.second);
                        if (data.isNull()) return dp;
                        size = data.toInt();
                    } else if (w.first == tr("Format")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Enum, w.second);
                        if (data.isNull()) return dp;
                        format = data.toInt();
                    } else if (w.first == tr("Signature Type")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Enum, w.second);
                        if (data.isNull()) return dp;
                        signatureType = data.toInt();
                    } else if (w.first == tr("Signature")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_HexData, w.second);
                        if (data.isNull() || data.type() != QVariant::Type::ByteArray) return dp;
                        QByteArray sig = data.toByteArray();
                        for (int i = 0; i < 16 && i < sig.size(); i++) {
                            signature[i] = sig[i];
                        }
                    }
                }
                dp = new QEFIDevicePathMediaHD(partitionNumber, start, size,
                    signature, format, signatureType);
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_CDROM:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Vendor:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_File:
            {
                QString filepath;
                for (const auto &w: std::as_const(m_currentWidgets)) {
                    if (w.first == tr("File")) {
                        QVariant data = retrieveDPEditComponent(
                            QEFIDPEditType::EditType_Path, w.second);
                        if (data.isNull() || data.type() != QVariant::Type::String) return dp;
                        filepath = data.toString();
                    }
                }
                dp = new QEFIDevicePathMediaFile(filepath);
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Protocol:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
            break;
        default:
            break;
        }
        break;
    case QEFIDevicePathType::DP_BIOSBoot:
        break;
    default:
        break;
    }
    return dp;
}

void QEFIDPEditorView::dpTypeComboBoxCurrentIndexChanged(int index)
{
    if (m_dpSubtypeSelector != nullptr && m_dpTypeSelected != index) {
        // Change the subtype
        m_dpSubtypeSelector->clear();
        int type = (m_dpTypeSelector != nullptr ?
            m_dpTypeSelector->itemData(index).toInt() : 0);
        QList<quint8> subtypes = enum_device_path_subtype(
            (enum QEFIDevicePathType)type);
        for (const auto &subtype: std::as_const(subtypes)) {
            m_dpSubtypeSelector->addItem(convert_device_path_subtype_to_name(
                (enum QEFIDevicePathType)type, subtype),
                QVariant(subtype));
        }
    }
    m_dpTypeSelected = index;
}

void QEFIDPEditorView::dpSubtypeComboBoxCurrentIndexChanged(int index)
{
    if (m_dpTypeSelector != nullptr && m_dpSubtypeSelected != index) {
        // Change the page
        int type = (m_dpTypeSelector != nullptr ?
            m_dpTypeSelector->itemData(m_dpTypeSelected).toInt() : 0);
        int subtype = (m_dpTypeSelector != nullptr ?
            m_dpSubtypeSelector->itemData(index).toInt() : 0);
        QList<QPair<QString, QWidget *> > widgets = constructDPEditView((QEFIDevicePathType)type,
            (quint8)subtype & 0xFF);
        while (m_topLevelLayout->rowCount() > 2) m_topLevelLayout->removeRow(2);
        m_currentWidgets = widgets;
        for (const auto &w: std::as_const(m_currentWidgets)) {
            m_topLevelLayout->addRow(w.first, w.second);
        }
    }
    m_dpSubtypeSelected = index;
}

QList<QPair<QString, QWidget *> > QEFIDPEditorView::constructDPEditView(QEFIDevicePath *dp)
{
    if (dp == nullptr) return {};

    enum QEFIDevicePathType type = dp->type();
    quint8 subType = dp->subType();
    QList<QPair<QString, QWidget *> > widgets = constructDPEditView(type, subType);
    // TODO: Update the fields

    return widgets;
}

QWidget *constructDPEditComponent(enum QEFIDPEditType type) {
    QWidget *w;
    switch (type)
    {
    case QEFIDPEditType::EditType_Text:
    case QEFIDPEditType::EditType_Path: // TODO: Add a Browse button
    case QEFIDPEditType::EditType_HexData:
    case QEFIDPEditType::EditType_UUID:
        {
            QLineEdit *edit = new QLineEdit;
            w = (QWidget *)edit;
        }
        break;
    case QEFIDPEditType::EditType_Number:
    case QEFIDPEditType::EditType_HexNumber:
        {
            QSpinBox *edit = new QSpinBox;
            edit->setRange(0, INT_MAX);
            w = (QWidget *)edit;
        }
        break;
    case QEFIDPEditType::EditType_Enum:
        {
            QComboBox *edit = new QComboBox;
            w = (QWidget *)edit;
        }
        break;
    default:
        w = (QWidget *)new QLineEdit;
        break;
    }
    return w;
}

QList<QPair<QString, QWidget *> > QEFIDPEditorView::constructDPEditView(
    enum QEFIDevicePathType type, quint8 subType)
{
    QList<QPair<QString, QWidget *> > widgets;
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        break;
    case QEFIDevicePathType::DP_ACPI:
        break;
    case QEFIDevicePathType::DP_Message:
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subType)
        {
        case QEFIDevicePathMediaSubType::MEDIA_HD:
            // TODO: Add a quick disk+volume chooser to import data
            {
                quint8 signature[16];
                QEFIDevicePathMediaHD hd(0, 0, 0, signature, 0, 0);
                QList<QPair<QString, enum QEFIDPEditType>> types =
                    convert_device_path_types((QEFIDevicePath *)&hd);
                for (const auto &t: std::as_const(types)) {
                    QWidget *w = constructDPEditComponent(t.second);
                    widgets << QPair<QString, QWidget *>(t.first, w);
                    if (t.first == "Format") {
                        QComboBox *comboBox = dynamic_cast<QComboBox *>(w);
                        if (comboBox != nullptr) {
                            comboBox->addItem(tr("GPT"),
                                QVariant(QEFIDevicePathMediaHD::
                                    QEFIDevicePathMediaHDFormat::GPT));
                            comboBox->addItem(tr("PCAT"),
                                QVariant(QEFIDevicePathMediaHD::
                                    QEFIDevicePathMediaHDFormat::PCAT));
                        }
                    }
                    if (t.first == "Signature Type") {
                        QComboBox *comboBox = dynamic_cast<QComboBox *>(w);
                        if (comboBox != nullptr) {
                            comboBox->addItem(tr("None"),
                                QVariant(QEFIDevicePathMediaHD::
                                    QEFIDevicePathMediaHDSignatureType::NONE));
                            comboBox->addItem(tr("MBR"),
                                QVariant(QEFIDevicePathMediaHD::
                                    QEFIDevicePathMediaHDSignatureType::MBR));
                            comboBox->addItem(tr("GUID"),
                                QVariant(QEFIDevicePathMediaHD::
                                    QEFIDevicePathMediaHDSignatureType::GUID));
                        }
                    }
                }
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_CDROM:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Vendor:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_File:
            {
                QEFIDevicePathMediaFile file(QStringLiteral(""));
                QList<QPair<QString, enum QEFIDPEditType>> types =
                    convert_device_path_types((QEFIDevicePath *)&file);
                for (const auto &t: std::as_const(types)) {
                    QWidget *w = constructDPEditComponent(t.second);
                    widgets << QPair<QString, QWidget *>(t.first, w);
                }
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Protocol:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
            break;
        default:
            break;
        }
        break;
    case QEFIDevicePathType::DP_BIOSBoot:
        break;
    default:
        break;
    }
    return widgets;
}
