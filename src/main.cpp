#include "app/App.h"

int main() {
    theword::app::App app;
    if (!app.Init("TheWord")) return 1;
    app.Run();
    return 0;
}
