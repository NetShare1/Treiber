#ifndef NDEBUG

#define NS_DEBUG

#endif

#ifndef NS_USE_CONSOLE

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#endif

#include "Application.h"

#include "CLI11.hpp"
#include "log.h"

int main() {
    NS_INIT_LOG(ns::log::trace);


    Application* instance = Application::Get();

        
    instance->init("conf.nsconf");
}
