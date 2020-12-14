#pragma once

#include <string>

#include "Config.h"
#include "diagnostics.h"

class NetworkAdapter
{
public:
	char* Name;
	char* Desc;
	NsIpAddress* IpAdress;

	NetworkAdapter(
		char name[256],
		char desc[132],
		char ipAdress[16]
	) : Name{ name }, Desc{ desc }
	{
		IpAdress = new NsIpAddress();


		std::string s = ipAdress;
		IpAdress->ipp1 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp2 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp3 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp4 = atoi(s.substr(0, s.length()).c_str());

		DLOG(
			WINTUN_LOG_INFO,
			L"Added Adapter:\n   name: %S\n   desc: %S\n   IP: %d.%d.%d.%d",
			name,
			desc,
			IpAdress->ipp1,
			IpAdress->ipp2,
			IpAdress->ipp3,
			IpAdress->ipp4
		);
	}

	NetworkAdapter() {}
};

