#include "qefidpfields.h"
#include "qefifileselectiondialog.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>

// ============================================================================
// Custom Widget Implementations
// ============================================================================

// Custom widget for UInt64 (since QSpinBox only supports int)
class QUInt64SpinBox : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;
    quint64 m_value;
    quint64 m_min;
    quint64 m_max;
    bool m_hexDisplay;

public:
    explicit QUInt64SpinBox(QWidget *parent = nullptr, bool hexDisplay = false)
        : QWidget(parent), m_value(0), m_min(0), m_max(UINT64_MAX), m_hexDisplay(hexDisplay)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        if (hexDisplay) {
            m_lineEdit->setInputMask("HHHHHHHHHHHHHHHH");
            m_lineEdit->setText("0");
        } else {
            QRegularExpressionValidator *validator = new QRegularExpressionValidator(
                QRegularExpression("[0-9]{1,20}"), this);
            m_lineEdit->setValidator(validator);
        }

        connect(m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
            if (m_hexDisplay) {
                bool ok;
                quint64 val = text.toULongLong(&ok, 16);
                if (ok && val >= m_min && val <= m_max) {
                    m_value = val;
                }
            } else {
                bool ok;
                quint64 val = text.toULongLong(&ok);
                if (ok && val >= m_min && val <= m_max) {
                    m_value = val;
                }
            }
        });

        layout->addWidget(m_lineEdit);
        setLayout(layout);
    }

    void setValue(quint64 value) {
        m_value = value;
        if (m_hexDisplay) {
            m_lineEdit->setText(QString::number(value, 16).toUpper());
        } else {
            m_lineEdit->setText(QString::number(value));
        }
    }

    quint64 value() const { return m_value; }

    void setRange(quint64 min, quint64 max) {
        m_min = min;
        m_max = max;
    }
};

// Custom widget for IPv4 address
class QIPv4AddressWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;

public:
    explicit QIPv4AddressWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // IPv4 regex validator
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(
            QRegularExpression("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"),
            this);
        m_lineEdit->setValidator(validator);
        m_lineEdit->setPlaceholderText("0.0.0.0");

        layout->addWidget(m_lineEdit);
        setLayout(layout);
    }

    void setAddress(const QString &address) {
        m_lineEdit->setText(address);
    }

    QString address() const {
        return m_lineEdit->text();
    }

    void setAddressBytes(const quint8 *bytes) {
        if (bytes) {
            QString addr = QString("%1.%2.%3.%4")
                .arg(bytes[0]).arg(bytes[1]).arg(bytes[2]).arg(bytes[3]);
            setAddress(addr);
        }
    }

    void getAddressBytes(quint8 *bytes) const {
        if (!bytes) return;
        QStringList parts = m_lineEdit->text().split('.');
        if (parts.size() == 4) {
            for (int i = 0; i < 4; i++) {
                bytes[i] = parts[i].toUInt();
            }
        }
    }
};

// Custom widget for IPv6 address
class QIPv6AddressWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;

public:
    explicit QIPv6AddressWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_lineEdit->setPlaceholderText("::");

        layout->addWidget(m_lineEdit);
        setLayout(layout);
    }

    void setAddress(const QString &address) {
        m_lineEdit->setText(address);
    }

    QString address() const {
        return m_lineEdit->text();
    }
};

// Custom widget for MAC address
class QMACAddressWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;

public:
    explicit QMACAddressWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // MAC address validator (format: XX:XX:XX:XX:XX:XX)
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(
            QRegularExpression("^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$"),
            this);
        m_lineEdit->setValidator(validator);
        m_lineEdit->setPlaceholderText("00:00:00:00:00:00");

        layout->addWidget(m_lineEdit);
        setLayout(layout);
    }

    void setAddress(const QString &address) {
        m_lineEdit->setText(address);
    }

    QString address() const {
        return m_lineEdit->text();
    }
};

// Custom widget for file path with browse button
class QPathWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;
    QPushButton *m_browseButton;

public:
    explicit QPathWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_browseButton = new QPushButton("Browse...", this);
        m_browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(m_browseButton, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this, tr("Select File"), m_lineEdit->text());
            if (!path.isEmpty()) {
                m_lineEdit->setText(path);
            }
        });

        layout->addWidget(m_lineEdit, 1);  // Stretch factor 1 for line edit
        layout->addWidget(m_browseButton, 0);  // No stretch for button
        setLayout(layout);
    }

    void setPath(const QString &path) {
        m_lineEdit->setText(path);
    }

    QString path() const {
        return m_lineEdit->text();
    }
};

// Custom widget for file path with "Browse File" button
class QFilePathWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;
    QPushButton *m_browseButton;

public:
    explicit QFilePathWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_lineEdit->setPlaceholderText("Enter file path or click Browse File...");

        m_browseButton = new QPushButton("Browse File...", this);
        m_browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(m_browseButton, &QPushButton::clicked, this, [this]() {
            QEFIFileSelectionDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted) {
                QString path = dialog.selectedFilePath();
                if (!path.isEmpty()) {
                    m_lineEdit->setText(path);
                }
            }
        });

        layout->addWidget(m_lineEdit, 1);  // Stretch factor 1 for line edit
        layout->addWidget(m_browseButton, 0);  // No stretch for button
        setLayout(layout);
    }

    void setPath(const QString &path) {
        m_lineEdit->setText(path);
    }

    QString path() const {
        return m_lineEdit->text();
    }
};

// Custom widget for partition selection with browse button
class QPartitionSelectorWidget : public QWidget {
    Q_OBJECT
private:
    QLineEdit *m_lineEdit;
    QPushButton *m_browseButton;

public:
    explicit QPartitionSelectorWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_lineEdit->setPlaceholderText("Enter partition details or click Browse Partitions...");

        m_browseButton = new QPushButton("Browse Partitions...", this);
        m_browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(m_browseButton, &QPushButton::clicked, this, [this]() {
            // Placeholder: Partition selection dialog would go here
            // The dialog would enumerate available partitions and allow selection
        });

        layout->addWidget(m_lineEdit, 1);  // Stretch factor 1 for line edit
        layout->addWidget(m_browseButton, 0);  // No stretch for button
        setLayout(layout);
    }

    void setValue(const QString &value) {
        m_lineEdit->setText(value);
    }

    QString value() const {
        return m_lineEdit->text();
    }
};

// ============================================================================
// Widget Factory Implementation
// ============================================================================

QWidget* QEFIFieldWidgetFactory::createWidget(const QEFIFieldMeta &field, QWidget *parent)
{
    QWidget *widget = nullptr;

    switch (field.type) {
    case QEFIFieldType::UInt8:
    case QEFIFieldType::UInt16: {
        QSpinBox *spinBox = new QSpinBox(parent);
        spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        if (field.hexDisplay) {
            spinBox->setDisplayIntegerBase(16);
            spinBox->setPrefix("0x");
        }
        spinBox->setRange(field.minValue.toInt(), field.maxValue.toInt());
        widget = spinBox;
        break;
    }

    case QEFIFieldType::UInt32: {
        // Use QSpinBox with max safe int range or line edit for larger values
        if (field.maxValue.toUInt() <= INT_MAX) {
            QSpinBox *spinBox = new QSpinBox(parent);
            spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            if (field.hexDisplay) {
                spinBox->setDisplayIntegerBase(16);
                spinBox->setPrefix("0x");
            }
            spinBox->setRange(field.minValue.toInt(), field.maxValue.toInt());
            widget = spinBox;
        } else {
            QLineEdit *lineEdit = new QLineEdit(parent);
            lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            if (field.hexDisplay) {
                lineEdit->setInputMask("HHHHHHHH");
            } else {
                QRegularExpressionValidator *validator = new QRegularExpressionValidator(
                    QRegularExpression("[0-9]{1,10}"), lineEdit);
                lineEdit->setValidator(validator);
            }
            widget = lineEdit;
        }
        break;
    }

    case QEFIFieldType::UInt64: {
        QUInt64SpinBox *spinBox = new QUInt64SpinBox(parent, field.hexDisplay);
        spinBox->setRange(field.minValue.toULongLong(), field.maxValue.toULongLong());
        widget = spinBox;
        break;
    }

    case QEFIFieldType::Port: {
        QSpinBox *spinBox = new QSpinBox(parent);
        spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        spinBox->setRange(0, 65535);
        widget = spinBox;
        break;
    }

    case QEFIFieldType::String:
    case QEFIFieldType::URL: {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        widget = lineEdit;
        break;
    }

    case QEFIFieldType::Path: {
        QPathWidget *pathWidget = new QPathWidget(parent);
        widget = pathWidget;
        break;
    }

    case QEFIFieldType::FilePath: {
        QFilePathWidget *filePathWidget = new QFilePathWidget(parent);
        widget = filePathWidget;
        break;
    }

    case QEFIFieldType::PartitionSelector: {
        QPartitionSelectorWidget *partitionWidget = new QPartitionSelectorWidget(parent);
        widget = partitionWidget;
        break;
    }

    case QEFIFieldType::UUID: {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // UUID format validator
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(
            QRegularExpression("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"),
            lineEdit);
        lineEdit->setValidator(validator);
        lineEdit->setPlaceholderText("00000000-0000-0000-0000-000000000000");
        widget = lineEdit;
        break;
    }

    case QEFIFieldType::HexData:
    case QEFIFieldType::ByteArray: {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lineEdit->setPlaceholderText("Enter hex data (e.g., AABBCCDD)");
        widget = lineEdit;
        break;
    }

    case QEFIFieldType::IPv4Address: {
        QIPv4AddressWidget *ipWidget = new QIPv4AddressWidget(parent);
        widget = ipWidget;
        break;
    }

    case QEFIFieldType::IPv6Address: {
        QIPv6AddressWidget *ipWidget = new QIPv6AddressWidget(parent);
        widget = ipWidget;
        break;
    }

    case QEFIFieldType::MACAddress: {
        QMACAddressWidget *macWidget = new QMACAddressWidget(parent);
        widget = macWidget;
        break;
    }

    case QEFIFieldType::Enum: {
        QComboBox *comboBox = new QComboBox(parent);
        comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        for (const auto &enumVal : field.enumValues) {
            comboBox->addItem(enumVal.first, enumVal.second);
        }
        widget = comboBox;
        break;
    }

    default:
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        widget = lineEdit;
        break;
    }

    if (widget && !field.tooltip.isEmpty()) {
        widget->setToolTip(field.tooltip);
    }

    return widget;
}

void QEFIFieldWidgetFactory::setWidgetValue(QWidget *widget, const QEFIFieldMeta &field, const QVariant &value)
{
    if (!widget) return;

    switch (field.type) {
    case QEFIFieldType::UInt8:
    case QEFIFieldType::UInt16:
    case QEFIFieldType::Port: {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget);
        if (spinBox) {
            spinBox->setValue(value.toInt());
        }
        break;
    }

    case QEFIFieldType::UInt32: {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget);
        if (spinBox) {
            spinBox->setValue(value.toInt());
        } else {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
            if (lineEdit) {
                if (field.hexDisplay) {
                    lineEdit->setText(QString::number(value.toUInt(), 16).toUpper());
                } else {
                    lineEdit->setText(QString::number(value.toUInt()));
                }
            }
        }
        break;
    }

    case QEFIFieldType::UInt64: {
        QUInt64SpinBox *spinBox = qobject_cast<QUInt64SpinBox*>(widget);
        if (spinBox) {
            spinBox->setValue(value.toULongLong());
        }
        break;
    }

    case QEFIFieldType::String:
    case QEFIFieldType::URL: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            lineEdit->setText(value.toString());
        }
        break;
    }

    case QEFIFieldType::Path: {
        QPathWidget *pathWidget = qobject_cast<QPathWidget*>(widget);
        if (pathWidget) {
            pathWidget->setPath(value.toString());
        }
        break;
    }

    case QEFIFieldType::FilePath: {
        QFilePathWidget *filePathWidget = qobject_cast<QFilePathWidget*>(widget);
        if (filePathWidget) {
            filePathWidget->setPath(value.toString());
        }
        break;
    }

    case QEFIFieldType::PartitionSelector: {
        QPartitionSelectorWidget *partitionWidget = qobject_cast<QPartitionSelectorWidget*>(widget);
        if (partitionWidget) {
            partitionWidget->setValue(value.toString());
        }
        break;
    }

    case QEFIFieldType::UUID: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            lineEdit->setText(value.toUuid().toString());
        }
        break;
    }

    case QEFIFieldType::HexData:
    case QEFIFieldType::ByteArray: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            lineEdit->setText(value.toByteArray().toHex().toUpper());
        }
        break;
    }

    case QEFIFieldType::IPv4Address: {
        QIPv4AddressWidget *ipWidget = qobject_cast<QIPv4AddressWidget*>(widget);
        if (ipWidget) {
            ipWidget->setAddress(value.toString());
        }
        break;
    }

    case QEFIFieldType::IPv6Address: {
        QIPv6AddressWidget *ipWidget = qobject_cast<QIPv6AddressWidget*>(widget);
        if (ipWidget) {
            ipWidget->setAddress(value.toString());
        }
        break;
    }

    case QEFIFieldType::MACAddress: {
        QMACAddressWidget *macWidget = qobject_cast<QMACAddressWidget*>(widget);
        if (macWidget) {
            macWidget->setAddress(value.toString());
        }
        break;
    }

    case QEFIFieldType::Enum: {
        QComboBox *comboBox = qobject_cast<QComboBox*>(widget);
        if (comboBox) {
            int index = comboBox->findData(value);
            if (index >= 0) {
                comboBox->setCurrentIndex(index);
            }
        }
        break;
    }

    default:
        break;
    }
}

QVariant QEFIFieldWidgetFactory::getWidgetValue(QWidget *widget, const QEFIFieldMeta &field)
{
    if (!widget) return QVariant();

    switch (field.type) {
    case QEFIFieldType::UInt8:
    case QEFIFieldType::UInt16:
    case QEFIFieldType::Port: {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget);
        if (spinBox) {
            return QVariant(spinBox->value());
        }
        break;
    }

    case QEFIFieldType::UInt32: {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget);
        if (spinBox) {
            return QVariant(static_cast<quint32>(spinBox->value()));
        } else {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
            if (lineEdit) {
                bool ok;
                if (field.hexDisplay) {
                    quint32 val = lineEdit->text().toUInt(&ok, 16);
                    return ok ? QVariant(val) : QVariant();
                } else {
                    quint32 val = lineEdit->text().toUInt(&ok);
                    return ok ? QVariant(val) : QVariant();
                }
            }
        }
        break;
    }

    case QEFIFieldType::UInt64: {
        QUInt64SpinBox *spinBox = qobject_cast<QUInt64SpinBox*>(widget);
        if (spinBox) {
            return QVariant::fromValue(spinBox->value());
        }
        break;
    }

    case QEFIFieldType::String:
    case QEFIFieldType::URL: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            return QVariant(lineEdit->text());
        }
        break;
    }

    case QEFIFieldType::Path: {
        QPathWidget *pathWidget = qobject_cast<QPathWidget*>(widget);
        if (pathWidget) {
            return QVariant(pathWidget->path());
        }
        break;
    }

    case QEFIFieldType::FilePath: {
        QFilePathWidget *filePathWidget = qobject_cast<QFilePathWidget*>(widget);
        if (filePathWidget) {
            return QVariant(filePathWidget->path());
        }
        break;
    }

    case QEFIFieldType::PartitionSelector: {
        QPartitionSelectorWidget *partitionWidget = qobject_cast<QPartitionSelectorWidget*>(widget);
        if (partitionWidget) {
            return QVariant(partitionWidget->value());
        }
        break;
    }

    case QEFIFieldType::UUID: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            return QVariant(QUuid(lineEdit->text()));
        }
        break;
    }

    case QEFIFieldType::HexData:
    case QEFIFieldType::ByteArray: {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            return QVariant(QByteArray::fromHex(lineEdit->text().toLatin1()));
        }
        break;
    }

    case QEFIFieldType::IPv4Address: {
        QIPv4AddressWidget *ipWidget = qobject_cast<QIPv4AddressWidget*>(widget);
        if (ipWidget) {
            return QVariant(ipWidget->address());
        }
        break;
    }

    case QEFIFieldType::IPv6Address: {
        QIPv6AddressWidget *ipWidget = qobject_cast<QIPv6AddressWidget*>(widget);
        if (ipWidget) {
            return QVariant(ipWidget->address());
        }
        break;
    }

    case QEFIFieldType::MACAddress: {
        QMACAddressWidget *macWidget = qobject_cast<QMACAddressWidget*>(widget);
        if (macWidget) {
            return QVariant(macWidget->address());
        }
        break;
    }

    case QEFIFieldType::Enum: {
        QComboBox *comboBox = qobject_cast<QComboBox*>(widget);
        if (comboBox) {
            return comboBox->currentData();
        }
        break;
    }

    default:
        break;
    }

    return QVariant();
}

QString QEFIFieldWidgetFactory::getFieldLabel(const QEFIFieldMeta &field)
{
    return field.name;
}

// ============================================================================
// Helper Function to Get Fields for Any Device Path
// ============================================================================

QList<QEFIFieldMeta> getDevicePathFields(QEFIDevicePath *dp)
{
    if (dp == nullptr) {
        return {};
    }

    QEFIDevicePathType type = dp->type();
    quint8 subtype = dp->subType();

    // Hardware Device Paths
    if (type == QEFIDevicePathType::DP_Hardware) {
        switch (subtype) {
        case QEFIDevicePathHardwareSubType::HW_PCI:
            return getQEFIDevicePathHardwarePCIFields();
        case QEFIDevicePathHardwareSubType::HW_PCCard:
            return getQEFIDevicePathHardwarePCCardFields();
        case QEFIDevicePathHardwareSubType::HW_MMIO:
            return getQEFIDevicePathHardwareMMIOFields();
        case QEFIDevicePathHardwareSubType::HW_Vendor:
            return getQEFIDevicePathHardwareVendorFields();
        case QEFIDevicePathHardwareSubType::HW_Controller:
            return getQEFIDevicePathHardwareControllerFields();
        case QEFIDevicePathHardwareSubType::HW_BMC:
            return getQEFIDevicePathHardwareBMCFields();
        }
    }

    // ACPI Device Paths
    if (type == QEFIDevicePathType::DP_ACPI) {
        switch (subtype) {
        case QEFIDevicePathACPISubType::ACPI_HID:
            return getQEFIDevicePathACPIHIDFields();
        case QEFIDevicePathACPISubType::ACPI_HIDEX:
            return getQEFIDevicePathACPIHIDEXFields();
        }
    }

    // Message Device Paths
    if (type == QEFIDevicePathType::DP_Message) {
        switch (subtype) {
        case QEFIDevicePathMessageSubType::MSG_ATAPI:
            return getQEFIDevicePathMessageATAPIFields();
        case QEFIDevicePathMessageSubType::MSG_SCSI:
            return getQEFIDevicePathMessageSCSIFields();
        case QEFIDevicePathMessageSubType::MSG_USB:
            return getQEFIDevicePathMessageUSBFields();
        case QEFIDevicePathMessageSubType::MSG_I2O:
            return getQEFIDevicePathMessageI2OFields();
        case QEFIDevicePathMessageSubType::MSG_MACAddr:
            return getQEFIDevicePathMessageMACAddrFields();
        case QEFIDevicePathMessageSubType::MSG_IPv4:
            return getQEFIDevicePathMessageIPv4AddrFields();
        case QEFIDevicePathMessageSubType::MSG_IPv6:
            return getQEFIDevicePathMessageIPv6AddrFields();
        case QEFIDevicePathMessageSubType::MSG_UART:
            return getQEFIDevicePathMessageUARTFields();
        case QEFIDevicePathMessageSubType::MSG_USBClass:
            return getQEFIDevicePathMessageUSBClassFields();
        case QEFIDevicePathMessageSubType::MSG_LUN:
            return getQEFIDevicePathMessageLUNFields();
        case QEFIDevicePathMessageSubType::MSG_SATA:
            return getQEFIDevicePathMessageSATAFields();
        case QEFIDevicePathMessageSubType::MSG_ISCSI:
            return getQEFIDevicePathMessageISCSIFields();
        case QEFIDevicePathMessageSubType::MSG_VLAN:
            return getQEFIDevicePathMessageVLANFields();
        case QEFIDevicePathMessageSubType::MSG_NVME:
            return getQEFIDevicePathMessageNVMEFields();
        case QEFIDevicePathMessageSubType::MSG_URI:
            return getQEFIDevicePathMessageURIFields();
        case QEFIDevicePathMessageSubType::MSG_UFS:
            return getQEFIDevicePathMessageUFSFields();
        case QEFIDevicePathMessageSubType::MSG_SD:
            return getQEFIDevicePathMessageSDFields();
        case QEFIDevicePathMessageSubType::MSG_BT:
            return getQEFIDevicePathMessageBTFields();
        case QEFIDevicePathMessageSubType::MSG_WiFi:
            return getQEFIDevicePathMessageWiFiFields();
        case QEFIDevicePathMessageSubType::MSG_EMMC:
            return getQEFIDevicePathMessageEMMCFields();
        case QEFIDevicePathMessageSubType::MSG_BTLE:
            return getQEFIDevicePathMessageBTLEFields();
        case QEFIDevicePathMessageSubType::MSG_NVDIMM:
            return getQEFIDevicePathMessageNVDIMMFields();
        }
    }

    // Media Device Paths
    if (type == QEFIDevicePathType::DP_Media) {
        switch (subtype) {
        case QEFIDevicePathMediaSubType::MEDIA_HD:
            return getQEFIDevicePathMediaHDFields();
        case QEFIDevicePathMediaSubType::MEDIA_CDROM:
            return getQEFIDevicePathMediaCDROMFields();
        case QEFIDevicePathMediaSubType::MEDIA_File:
            return getQEFIDevicePathMediaFileFields();
        case QEFIDevicePathMediaSubType::MEDIA_Vendor:
            return getQEFIDevicePathMediaVendorFields();
        case QEFIDevicePathMediaSubType::MEDIA_Protocol:
            return getQEFIDevicePathMediaProtocolFields();
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
            return getQEFIDevicePathMediaFirmwareFileFields();
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
            return getQEFIDevicePathMediaFirmwareVolumeFields();
        case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
            return getQEFIDevicePathMediaRelativeOffsetFields();
        case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
            return getQEFIDevicePathMediaRAMDiskFields();
        }
    }

    // BIOS Boot Device Path
    if (type == QEFIDevicePathType::DP_BIOSBoot) {
        return getQEFIDevicePathBIOSBootFields();
    }

    return {};
}

#include "qefidpfields.moc"
