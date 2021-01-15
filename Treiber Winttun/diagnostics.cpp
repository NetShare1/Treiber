#include "diagnostics.h"

#ifdef n
// !!! THIS FUNCTIONS SHOULD NEVER BE CALLED DIRECTLY !!!
// !This function must be called before any other diagnostic function!
// Loads the necessary dlls for the diagnostics
// returns fals if the dlls could not be loaded. If this is returened the programm should be exited
// to not get a runtime error later.
static void initDiagnostics() {
    MTR_SCOPE("Diagnostics", __FUNCSIG__);
    ntdll = LoadLibrary(TEXT("ntdll.dll"));

    if (!ntdll) {
        NS_LOG_APP_ERROR("Error trying to load ntdll.dll", GetLastErrorAsString());
        exit(4300);
    }

    Ipv4PrintAsString =
        (RtlIpv4AddressToStringW)GetProcAddress(ntdll, "RtlIpv4AddressToStringW");

    if (!Ipv4PrintAsString) {
        NS_LOG_APP_ERROR("Error trying to load RtlIpv4AddressToStringW", GetLastErrorAsString());
        exit(4301);
    }

    Ipv6PrintAsString =
        (RtlIpv6AddressToStringW)GetProcAddress(ntdll, "RtlIpv6AddressToStringW");

    if (!Ipv6PrintAsString) {
        NS_LOG_APP_ERROR("Error trying to load RtlIpv6AddressToStringW", GetLastErrorAsString());
        exit(4302);
    }

    NS_LOG_APP_DEBUG("Sucessfully initialized diagnostics");
}


#endif // DEBUG
