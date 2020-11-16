#include "init_wintun.h"
#include "wintun.h"

// WTF IS GOING ON WHY THE FCK DO I NEED TO DO THIS SINCE WHE 
// DO I NEED TO INCLUDE C FILES AND WHY CANT I DO THAT IN THE 
// FUCKING HEADER FILE LIKE A NORMAL FUCKING PERSON
#include "log.c"

#include <combaseapi.h>

int main() {
    HMODULE Wintun = InitializeWintun();
    if (!Wintun)
        return LogError(L"Failed to initialize Wintun", GetLastError());
        
    GUID guid;
    HRESULT hCreateGuid = CoCreateGuid(&guid);

    WINTUN_ADAPTER_HANDLE handle = WintunCreateAdapter(
        L"WireGuard",
        L"OfficeNet",
        &guid,
        NULL
    );

    return 0;
}