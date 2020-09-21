#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include "Cfgmgr32.h"
#include <locale>
#include <codecvt>
#include <string>
#include <setupapi.h>
#include <stdlib.h>  

std::string GetLastErrorAsString();

int main() {
	/*
		Hier will ich einen handle auf das TAN Gerät bekommen. Dafür brauch ich die
		Device Instance ID: 
		 https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-instance-ids
		
		Um diese Instance ID zu bekommen brauch ich zuerst ein Device Information Set diesr enthält
		informationen über diese Geräte: 
		  https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw
		
	*/
	

	// returns device information set according to the provided data.
	HDEVINFO deviceInformationSet = SetupDiGetClassDevsW(
		nullptr, // pointer to device setup class, don't need that so it can be null
		(PCWSTR)L"wintun", // takes in an id of an pnp of which we want the information
		NULL, // a handle to the toplevel level window, we dont have one so null
		DIGCF_ALLCLASSES // access falgs this one makes it so we get a list of all device setup classes
		// other flags can be found here: https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw
	);

	// this will hold the data about the device
	PSP_DEVINFO_DATA deviceData{};

	// im only doing a maximum of 10 itaration because if there are more than 10
	// than the id of the pnp is probalbly wrong.
	for (int i{ 0 }; i <= 10; i++) {

		if (i == 10) {
			// display error message informing user of wrong id 
			std::cerr << "There are more than 10 devicesnodes in" <<
				" the device information sets. The id of the pnp is probably wrong!\n"
				<< GetLastErrorAsString() << std::endl;
			exit(EXIT_FAILURE);
		}

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
			break;
		}

		std::cout << "Found Information with id " << i << std::endl;
	}

	/*if (!SetupDiGetDeviceInstanceIdA(
		deviceInformationSet,

	)) {
		std::cerr << "Error recieving device instance id";
	}*/

	PDEVINST* deviceInstancHandle;

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