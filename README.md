# mprpc-study
A personal study repository on the [MPRCP] distributed communication framework. Includes notes, source code analysis, configuration examples, and hands-on demo projects, aiming to understand its core design and implementation principles.

## Features
* Protocol Buffers Integration - Define RPC services using .proto files
* Service Registration & Discovery - Automatic service registration with ZooKeeper
* Asynchronous Logging - High-performance logging with configurable levels
* Multi-threaded Server - Built on muduo's event-driven architecture
* Connection Management - Efficient TCP connection handling with reconnection support


## Requirements
### Operating System
* Linux (Ubuntu 20.04+ recommended)

### Dependencies
* muduo network library
* CMake 3.10+
* Protocol Buffers 3.0+
* ZooKeeper C API


## Quick Start

### Clone and Build

```bash
git clone https://github.com/ChiangChianan/mprpc-study.git
cd mprpc-study
sh build.sh
```


### Starting the Server

```bash
./build/bin/user_service -i config/test.conf
```

### Starting the Client

```bash
./build/bin/user_client -i config/test.conf
```



