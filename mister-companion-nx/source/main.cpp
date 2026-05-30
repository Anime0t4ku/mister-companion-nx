#include <switch.h>
#include <libssh2.h>

#include "app.hpp"

int main(int argc, char* argv[]) {
    socketInitializeDefault();
    libssh2_init(0);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    App app;
    app.run();

    libssh2_exit();
    socketExit();
    return 0;
}
