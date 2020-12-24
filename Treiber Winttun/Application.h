#pragma once


#include <WinSock2.h>

#include "adapter.h"
#include "diagnostics.h"
#include "log.h"
#include "init_wintun.h"
#include "wintun.h"
#include "Config.h"
#include "UDPWorkerConfig.h"
#include "WinTunLogic.h"
#include "UPDSocketWorker.h"
#include "NetworkAdapterList.h"
#include "execute.h"
#include "minitrace.h"
#include "utils.h"
#include "ConfigurationWorker.h"

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
	bool initRun();
	void shutdown();

	void init(std::string confFilePath);

	bool isRunning() {
		return !exited;
	}

	bool isStartingUp() {
		return startingUp;
	}

	static Application* Get() {
		static std::mutex m;
		static Application* instance;
		if (NULL == instance)
		{
			std::lock_guard<std::mutex> lock{ m };

			if (NULL == instance)
			{
				instance = new Application();
			}
		}
		return instance;
	}

	BOOL handleCtrlSignal(DWORD signal);
private:
	void initProfiling();
	void shutdownProfiling();

	Config* createConfigFromFile(std::string file);
	void addConfParameter(std::string keyValue, _Inout_ Config* conf);

	void clearWorkers();

	Config* conf;
	NetworkAdapterList* adapterList;

	std::vector<std::string>* adapterNames = new std::vector<std::string>();
	std::vector<UDPWorkerConfig*>* udpconfigs;

	int numberOfWorkers;
	HANDLE* Workers;

	std::thread* configThread;

	std::string confFilePath;

	bool exited = true;
	bool startingUp = false;


	

#ifdef NS_PERF_PROFILE
	std::thread* metricsThread;
#endif
};


void nsShutdown();
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
