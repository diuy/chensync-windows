#pragma once
#include <winsock2.h>
#include <thread>
#include <memory>
#include <vector>
#include "FileProcess.h"
using namespace std;

class FileServer
{
private:
	bool runFlag;
	SOCKET so;
	unique_ptr<thread>	th;
	vector<unique_ptr<FileProcess>> processes;
	int port;
	string rootDir;
public:
	FileServer(string rootDir,int port);
	~FileServer();
	bool start();
	void stop();
private:
	void listener();
};

