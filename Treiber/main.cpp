#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include "Cfgmgr32.h"
#include <locale>
#include <codecvt>
#include <string>
#include <setupapi.h>
#include <stdlib.h> 
#include <psapi.h>

#define ARRAY_SIZE 1024

std::string GetLastErrorAsString();
void listAllDrivers();

int main() {
	/*
		Hier will ich einen handle auf das TAN Gerät bekommen. Dafür brauch ich die
		Device Instance ID: 
		 https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-instance-ids
		
		Um diese Instance ID zu bekommen brauch ich zuerst ein Device Information Set diesr enthält
		informationen über diese Geräte: 
		  https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw
		
	*/


	PDEVINST deviceInstancHandle = new DEVINST();

	// get device instance handle
	CM_Locate_DevNodeA(deviceInstancHandle, (DEVINSTID_A)L"ROOT/NET/005", CM_LOCATE_DEVNODE_NORMAL);

	if (deviceInstancHandle == INVALID_HANDLE_VALUE) {
		std::cout << "error" << std::endl;
	}

	std::cout << GetLastErrorAsString() << std::endl;

	return 0;


	/*listAllDrivers();

	return 0;
	*/

	// returns device information set according to the provided data.
	HDEVINFO deviceInformationSet = SetupDiGetClassDevsW(
		nullptr, // pointer to device setup class, don't need that so it can be null
		(PCWSTR)L"{4d36e972-e325-11ce-bfc1-08002be10318", // takes in an id of an pnp of which we want the information
		// NULL, // NULL to get every device node of the device tree
		NULL, // a handle to the toplevel level window, we dont have one so null
		DIGCF_ALLCLASSES // access falgs this one makes it so we get a list of all device setup classes
		// other flags can be found here: https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw
	);

	PSP_DEVINFO_DATA deviceData = new SP_DEVINFO_DATA();
	deviceData->cbSize = sizeof SP_DEVINFO_DATA;

	// im only doing a maximum of 10 itaration because if there are more than 10
	// than the id of the pnp is probalbly wrong.
	for (int i{ 0 }; i <= 10; i++) {

		/*if (i == 10) {
			// display error message informing user of wrong id 
			std::cerr << "There are more than 10 devicesnodes in" <<
				" the device information sets. The id of the pnp is probably wrong!\n"
				<< GetLastErrorAsString() << std::endl;
			exit(EXIT_FAILURE);
		}*/

		// retruns data from the device information set
		// https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-information-sets
		if (!SetupDiEnumDeviceInfo(
			deviceInformationSet, // Information sets we want the data from
			0, // index of the information
			deviceData // the information will be saved here
		)) {
			if (i == 0) {
				// if we are here than there was no infromation found so we need to inform
				// the user that there is something wrong!
				std::cerr << "No information within this InformationSet," 
					<< " the id of the pnp is probably wrong\n" 
					<< GetLastErrorAsString() << std::endl;
				
				exit(EXIT_FAILURE);
			}
			std::cout << "No more information in the InformationSet!" << std::endl;
			//break;
		}

		std::cout << "Found Information with id " << i << std::endl;

		std::cout << "GUID from found device: " 
			<< deviceData->ClassGuid.Data1 << " "
			<< deviceData->ClassGuid.Data2 << " "
			<< deviceData->ClassGuid.Data3 << " "
			<< deviceData->ClassGuid.Data4 << " "
			<<  std::endl;

		char* buffer = new char(MAX_CLASS_NAME_LEN);

		SetupDiClassNameFromGuidA(
			&(deviceData->ClassGuid),
			buffer,
			MAX_CLASS_NAME_LEN,
			NULL
		);

		std::cout << "className: " << buffer << std::endl << std::endl;
	}

	/*if (!SetupDiGetDeviceInstanceIdA(
		deviceInformationSet,

	)) {
		std::cerr << "Error recieving device instance id";
	}*/

	//PDEVINST* deviceInstancHandle;

	// get device instance handle
	//CM_Locate_DevNodeA();


	exit(EXIT_SUCCESS);
}


// gets the error string from the last error code
std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}


void listAllDrivers() {
	LPVOID drivers[ARRAY_SIZE];
	DWORD cbNeeded;
	int cDrivers, i;

	if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
	{
		TCHAR szDriver[ARRAY_SIZE];

		cDrivers = cbNeeded / sizeof(drivers[0]);

		_tprintf(TEXT("There are %d drivers:\n"), cDrivers);

		for (i = 0; i < cDrivers; i++)
		{
			if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
			{
				if (!_tcscmp(szDriver, L"wintun.sys")) { // compares the two strings
					_tprintf(TEXT("%d: %s\n"), i + 1, szDriver);
				}
			}
		}
	}
	else
	{
		_tprintf(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), cbNeeded / sizeof(LPVOID));
	}

}

