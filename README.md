# Treiber

**Der Code ist daweil noch nicht schön**

Here is the code of the "*driver*" of Netshare. It uses the wintun driver to capture Packets and sends them over the winsock2 api to the api server.

Currently there are two projects in this solution. The CLI and the driver itselfe.

## CLI

The CLI is to configure, start and stop the driver.

It works by sending json files over a tcp connection to the driver.

To see the usage type: 
```ns -h```

## Driver

The driver itself does nothing but open a port and waiting for commands. So you can just start it. If you compiled it with NS_USE_CONSOLE a console will popup with all of the logs merged together for developing purposes. 

**Needs administrative privilages to start**

