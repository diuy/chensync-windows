#pragma once
#include <winsock2.h>
#include <thread>
#include <memory>
#include <vector>
using namespace std;

class FileProcess
{
private:
	SOCKET so;
	thread th;
	bool finishedFlag;
	bool runFlag;
	string rootDir;
public:
	FileProcess(string rootDir,SOCKET so);
	~FileProcess();
	bool finished();
private:
	void run();
	bool parse(string &data, int &type);
	void processCheck(const string &data);
	void processUpload(const string &data);
	void sendCheckResult(vector<string>& files);
	void sendUploadResult(int64_t recvSize);
	void sendError(uint8_t result, string reason);
	void wrapFileName(string &name);
	void unwrapFileName(string &name);


};

