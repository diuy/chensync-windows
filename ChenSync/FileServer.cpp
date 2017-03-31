#include <iostream>
#include "FileServer.h"
#pragma comment(lib,"ws2_32.lib")



FileServer::FileServer(string rootDir, int port)
	:so(INVALID_SOCKET)
	, runFlag(false)
	, rootDir(rootDir)
	, port(port) {
}

FileServer::~FileServer() {
	stop();
}

bool FileServer::start() {
	if (runFlag)
		return true;

	if (rootDir.empty()) {
		cerr << "rootDir is empty" << endl;
		return false;
	}

	if (port <= 0 || port >= 65535) {
		cerr << "port is error :" << port << endl;
		return false;
	}


	//初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		cerr << "WSAStartup error" << endl;
		return false;
	}

	//创建套接字
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET) {
		cerr << "socket create fail" << endl;
		return false;
	}

	//绑定IP和端口
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
		closesocket(slisten);
		cout << ("bind error !") << endl;
		return false;
	}

	//开始监听
	if (listen(slisten, 20) == SOCKET_ERROR) {
		closesocket(slisten);
		cout << ("listen error !") << endl;
		return false;
	}
	runFlag = true;
	so = slisten;
	th.reset(new thread(mem_fn(&FileServer::listener), this));
	return true;
}

void FileServer::stop() {
	if (!runFlag)
		return;
	runFlag = false;
	//shutdown(so, SD_BOTH);
	closesocket(so);
	th->join();

	processes.clear();

	WSACleanup();
	th = nullptr;
	so = INVALID_SOCKET;
}

void FileServer::listener() {
	//循环接收数据
	SOCKET sClient;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	while (runFlag) {
		sClient = accept(so, (SOCKADDR *)&remoteAddr, &nAddrlen);

		if (sClient == INVALID_SOCKET) {
			if (runFlag) {
				cout << "accept error !" << endl;
			}
			continue;
		}
		int32_t sendSize = 1024*100;
		int32_t recvSize = 1024 * 1024;
		char nodelay = 1;
		::setsockopt(sClient, SOL_SOCKET, SO_RCVBUF, (char *)&sendSize, sizeof(sendSize));
		::setsockopt(sClient, SOL_SOCKET, SO_SNDBUF, (char *)&sendSize, sizeof(sendSize));
		::setsockopt(sClient, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(char));
		cout << "accept client" << inet_ntoa(remoteAddr.sin_addr) << endl;
		processes.push_back(unique_ptr<FileProcess>(new FileProcess(rootDir,sClient)));

		for (auto item = processes.begin(); item != processes.end();) {
			if ((*item)->finished()) {
				item = processes.erase(item);
			} else {
				item++;
			}
		}
	}
}
