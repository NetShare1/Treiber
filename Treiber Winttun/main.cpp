#ifndef NDEBUG

#define NS_DEBUG

#endif

#include "Application.h"

#include "CLI11.hpp"
#include "log.h"

int main() {
    NS_INIT_LOG(ns::log::trace);


    Application* instance = Application::Get();

        
    instance->init("conf.nsconf");
}
