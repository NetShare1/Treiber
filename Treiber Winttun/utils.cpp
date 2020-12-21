#include "utils.h"

NsIpAddress* parseIpString(std::string ipAdress) {
	try {
		NsIpAddress* IpAdress = new NsIpAddress();
		std::string s = ipAdress;
		IpAdress->ipp1 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp2 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp3 = atoi(s.substr(0, s.find(".")).c_str());
		s = s.substr(s.find(".") + 1, s.length());
		IpAdress->ipp4 = atoi(s.substr(0, s.length()).c_str());
	}
	catch (...) {
		Log(WINTUN_LOG_ERR, L"Error parsing Ip Adress %S", ipAdress);
		exit(EXIT_FAILURE);
	}
}