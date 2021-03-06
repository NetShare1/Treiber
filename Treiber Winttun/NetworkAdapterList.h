#pragma once

#include <iphlpapi.h>

#include <vector>

#include "log.h"
#include "NetworkAdapter.h"
#include "UPDSocketWorker.h"

#pragma comment(lib, "IPHLPAPI.lib")



class NetworkAdapterList
{
public:

	int init() {
		MTR_SCOPE("Network_Adapter_List", __FUNCSIG__);
		PIP_ADAPTER_INFO pAdapterInfo;
		PIP_ADAPTER_INFO pAdapter = NULL;
		DWORD dwRetVal = 0;
		UINT i;

		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));

		if (pAdapterInfo == NULL) {
			NS_LOG_APP_ERROR("Error allocating memory needed to call GetAdaptersinfo");
			return 1;
		}

		// Make an initial call to GetAdaptersInfo to get
		// the necessary size into the ulOutBufLen variable
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
			if (pAdapterInfo == NULL) {
				NS_LOG_APP_ERROR("Error allocating memory needed to call GetAdaptersinfo");
				return 1;
			}
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
			pAdapter = pAdapterInfo;
			while (pAdapter) {
				std::pair<bool, NetworkAdapter*>* workingPair = new std::pair<bool, NetworkAdapter*>();
				workingPair->second = new NetworkAdapter(
					pAdapter->AdapterName,
					pAdapter->Description,
					pAdapter->IpAddressList.IpAddress.String
				);

				workingPair->first = isViable(workingPair->second);

				adapterList->push_back(workingPair);

				pAdapter = pAdapter->Next;
			}
			// activateViable();
		}
		else {
			NS_LOG_APP_ERROR("Error getting adapter infos");
			return 1;
		}
	}

	void deactiveEverythingBut(std::string names[], size_t numberOfNames) {
		MTR_SCOPE("Network_Adapter_List", __FUNCSIG__);
		bool hasName;
		for (
			std::vector<std::pair<bool, NetworkAdapter*>*>::iterator it = adapterList->begin();
			it != adapterList->end();
			++it)
		{
			hasName = false;
			for (int i = 0; i < numberOfNames; i++) {
				if (names[i].compare((*it)->second->Name) == 0) {
					hasName = true;
				}
			}
			(*it)->first = hasName;
			NS_LOG_APP_DEBUG(
				"Using adapter {} ? {}",
				(*it)->second->Desc,
				hasName ? "true" : "false"
			);
		}
	}

	std::vector<std::shared_ptr<UDPWorkerConfig>>* generateUDPConfigs(std::shared_ptr<Config> conf) {
		MTR_SCOPE("Network_Adapter_List", __FUNCSIG__);
		int port = 65530;
		int uid = 0;
		std::vector<std::shared_ptr<UDPWorkerConfig>>* configs = new std::vector<std::shared_ptr<UDPWorkerConfig>>();
		for (
			std::vector<std::pair<bool, NetworkAdapter*>*>::iterator it = adapterList->begin();
			it != adapterList->end();
			++it)
		{
			if ((*it)->first) {
				std::shared_ptr<UDPWorkerConfig> sconf{ new UDPWorkerConfig() };
				std::shared_ptr<UDPWorkerConfig> rconf{ new UDPWorkerConfig() };

				sconf->conf = conf;
				rconf->conf = conf;

				sconf->reciever = false;
				rconf->reciever = true;

				sconf->socketIpv4Adress = *((*it)->second->IpAdress);
				rconf->socketIpv4Adress = *((*it)->second->IpAdress);

				sconf->socketPort = port;
				rconf->socketPort = port;

				sconf->uid = uid;
				uid++;
				rconf->uid = uid;
				uid++;

				initSocket(sconf);
				rconf->socket = sconf->socket;

				configs->push_back(sconf);
				configs->push_back(rconf);

				port++;
			}
		}

		return configs;
	}

	bool hasActivatedNetworkAdapters() {
		for (
			std::vector<std::pair<bool, NetworkAdapter*>*>::iterator it = adapterList->begin();
			it != adapterList->end();
			++it
		) {
			// if is in use
			if ((*it)->first) {
				return true;
			}

		}
		return false;
	}

	std::vector<std::pair<bool, NetworkAdapter*>*>* getAdapterList() {
		return adapterList;
	}

private:

	void activateViable() {
		MTR_SCOPE("Network_Adapter_List", __FUNCSIG__);
		NsIpAddress zeroAdress{ 0, 0, 0, 0 };
		for (
			std::vector<std::pair<bool, NetworkAdapter*>*>::iterator it = adapterList->begin();
			it != adapterList->end();
			++it)
		{
			// holy s*** this is ugly
			if (*((*it)->second->IpAdress) != zeroAdress) {
				(*it)->first = true;
				NS_LOG_APP_INFO("Using Adapter as proxy: {}", (*it)->second->Desc);
			}
		}
	}

	bool isViable(NetworkAdapter* adapter) {
		MTR_SCOPE("Network_Adapter_List", __FUNCSIG__);
		NsIpAddress zeroAdress{ 0, 0, 0, 0 };

#ifndef NS_DEBUG
		return *(adapter->IpAdress) != zeroAdress;
#else
		bool viable = *(adapter->IpAdress) != zeroAdress;
		if (!viable) {
			NS_LOG_APP_DEBUG("Adapter {} is not viable because it has a zero adress", adapter->Desc);
		}
		return viable;
#endif // NS_DEBUG
	}

public: 

	std::vector<std::pair<bool, NetworkAdapter*>*>* adapterList = new std::vector<std::pair<bool, NetworkAdapter*>*>();

};

