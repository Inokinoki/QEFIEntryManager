#include "cli.h"

// Dedicated CLI-only executable for qefibootmgr
// This provides an efibootmgr-compatible command-line interface
int main(int argc, char *argv[])
{
    CLI cli(argc, argv);
    return cli.execute();
}
