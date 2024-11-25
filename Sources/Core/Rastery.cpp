#include "App.h"
#include "Utils/Logger.h"
using namespace Rastery;

int main(int argc, const char **argv) {
    App app;
    if (argc >= 2) {
        app.import(argv[1]);
    }
    app.run();

    return 0;
}
