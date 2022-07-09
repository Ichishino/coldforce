Coldforce
========

Coldforce is a framework written in C that supports various network protocols.  
With the asynchronous API of this framework
You can easily create event-driven network applications.  
The currently supported protocols are as follows.
All of these support clients and servers (multi-client, C10K).
* TCP/UDP
* TLS
* HTTP/1.1
* HTTP/2
* WebSocket

### Platforms
* Windows
* Linux
* macOS

### Requirements
* C99 or later
* OpenSSL (only when using TLS, https, wss)
* -pthread -lm

### Modules
* co_core - Application core
* co_net - TCP,UDP
* co_tls - TLS
* co_http - HTTP/1.1
* co_http2 - HTTP/2
* co_ws - WebSocket

### Builds
* Windows  
Visual Studio ([prj/vs19/coldforce.sln](https://github.com/Ichishino/coldforce/tree/master/prj/vs19/coldforce))
* Linux  
cmake
```shellsession
$ cd build
$ cmake ..
$ make
```
* macOS  
cmake (same way as Linux)
XCode (coming soon)

### Code Examples
* [examples here](https://github.com/Ichishino/coldforce/tree/master/examples) 
