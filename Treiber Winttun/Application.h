#pragma once

#include <WinSock2.h>

#include "diagnostics.h"
#include "log.h"
#include "init_wintun.h"
#include "wintun.h"
#include "adapter.h"
#include "Config.h"
#include "UDPWorkerConfig.h"
#include "WinTunLogic.h"
#include "UPDSocketWorker.h"
#include "NetworkAdapterList.h"
#include "execute.h"
#include "minitrace.h"
#include "utils.h"

#include <combaseapi.h>
#include <synchapi.h>

#include <chrono>
#include <thread>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <fstream>

class Application
{
public:

	void run();
	bool init();
	void shutdown();

	static Application& Get() { static Application instance; return instance; }

	BOOL handleCtrlSignal(DWORD signal);
private:
	void initProfiling();
	void shutdownProfiling();

	Config* createConfigFromFile(std::string file);
	void addConfParameter(std::string keyValue, _Inout_ Config* conf);
	void addAdapterNames(std::string);

	void clearWorkers();

	Config* conf;
	NetworkAdapterList* adapterList;

	std::vector<std::string>* adapterNames = new std::vector<std::string>();
	std::vector<UDPWorkerConfig*>* udpconfigs;

	int numberOfWorkers;
	HANDLE* Workers;

#ifdef NS_PERF_PROFILE
	std::thread* metricsThread;
#endif
};


void nsShutdown();
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
