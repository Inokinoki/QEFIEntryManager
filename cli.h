#ifndef CLI_H
#define CLI_H

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QTextStream>

class CLI
{
public:
    CLI(int argc, char *argv[]);
    ~CLI();
    int execute();

private:
    enum class EfiMode {
        Boot,
        Driver,
        SysPrep
    };

    QCoreApplication *m_app;
    QCommandLineParser m_parser;
    EfiMode m_mode;

    void setupParser();
    void printVersion();
    void printEntries(bool verbose, bool quiet);
    void printMirror();
    bool handleModifications();
    bool handleCreate(QTextStream &out, QTextStream &err, bool quiet);
    bool handleMirror(QTextStream &out, QTextStream &err, bool quiet);
    bool createLoadOption(QByteArray *out, QTextStream &err);
    QByteArray collectOptionalData(QTextStream &err, bool *ok);
    QString prefix() const;
    QString orderName() const;

    int m_argc;
    char **m_argv;
};

#endif // CLI_H
