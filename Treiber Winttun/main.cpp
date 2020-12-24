#include "Application.h"

#include "CLI11.hpp"

int main() {

    Application* instance = Application::Get();

    instance->init("conf.conf");
}
