# Treiber

**Der Code ist daweil noch nicht schön**

Here is the code of the "*driver*" of Netshare. It uses the wintun driver to capture Packets and sends them over the winsock2 api to the api server.

Currently there are two projects in this solution. The CLI and the driver itselfe.

## Install

1. Clone the git repository to your computer
  * ```git clone  https://github.com/NetShare1/Treiber.git```
2. Install the Windows Driver Development Kit
  * A toturial on how to do that can you find [here](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).
3. Build the CLI
  * Right click on CLI and click build. The CLI is self contained and does not need extra configuration to work.
  * Make sure you run all build comands in Release mode
4. Build the driver
  * Right click on the Treiber Wintun Projekt and select build.
  * The driver is dependent on the Windows Driver Development Kit. So if you dont have it installed it wont compile.
  * you can define NS_PERF_PROFILE to create a profilling file that you can view in the chrome profiler
  * you can define NS_USE_CONSOLE to start a console window when the driver starts. *Should only be used for development*.
  * you can define NS_ENABLE_STATISTICS to log statistics every couple of seconds. *Is defined by default.*
  * you can define NDEBUG to not use some debug stuff although you are in debug mode.

## CLI

The CLI is to configure, start and stop the driver.

It works by sending json files over a tcp connection to the driver.

To see the usage type: 
```ns -h```

## Driver

The driver itself does nothing but open a port and waiting for commands. So you can just start it. If you compiled it with NS_USE_CONSOLE a console will popup with all of the logs merged together for developing purposes. 

**Needs administrative privilages to start**

