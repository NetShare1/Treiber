#include "Application.h"

int main() {
    Application instance = Application::Get();

    if (instance.init()) {
        instance.run();
    }
}
