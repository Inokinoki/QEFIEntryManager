#ifndef CLI_H
#define CLI_H

#include <QCoreApplication>
#include <QCommandLineParser>

class CLI
{
public:
    CLI(int argc, char *argv[]);
    ~CLI();
    int execute();

private:
    QCoreApplication *m_app;
    QCommandLineParser m_parser;

    void setupParser();
    void printBootEntries(bool verbose, bool quiet);
    void printVersion();
    bool handleModifications();

    int m_argc;
    char **m_argv;
};

#endif // CLI_H
