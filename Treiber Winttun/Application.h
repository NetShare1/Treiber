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
#include "AppConfig.h"

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

	std::shared_ptr<Config> getConfig() {
		return conf;
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

	std::shared_ptr<Config> createConfigFromFile(std::string file);
	void addConfParameter(std::string keyValue, _Inout_ std::shared_ptr<Config> conf);

	void clearWorkers();

	std::shared_ptr<Config> conf;
	std::shared_ptr<Config> wintunReceiveConf;
	std::shared_ptr<Config> wintunSendConf;
	NetworkAdapterList* adapterList;

	std::unique_ptr<std::vector<std::string>> adapterNames;
	std::vector<std::shared_ptr<UDPWorkerConfig>>* udpconfigs;

	int numberOfWorkers;
	HANDLE* Workers;

	std::thread* configThread;

	std::unique_ptr<AppConfig> internalConfig = std::make_unique<AppConfig>();

	std::string confFilePath;

	bool exited = true;
	bool startingUp = false;

#ifdef NS_PERF_PROFILE
	std::thread* metricsThread;
#endif
};


void nsShutdown();
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
