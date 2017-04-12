#include "FileProcess.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <io.h>
#include "FileManager.h"
#include "Util.h"

constexpr int HEAD_SIZE = 8;
constexpr int TYPE_CHECK = 1;
constexpr int TYPE_UPLOAD = 2;

//请求
//SYNC=0xCC,0xBB
//TYPE=0x1|0x2
//RESV=0
//SIZE=4byte
//
//返回
//SYNC=0xCC,0xEE
//RESULT=0x0|0x1
//RESV=0
//SIZE=4byte
//
//判断
//设备 目录
//文件路径 大小 修改时间
//文件路径 大小 修改时间
//
//判断返回
//文件路径
//文件路径
//
//
//上传
//设备 目录 文件路径 大小 修改时间
//数据....
//
//上传返回
//收到文件大小
//
//
//




FileProcess::FileProcess(string rootDir, SOCKET so)
	:rootDir(rootDir), so(so)
	, th(mem_fn(&FileProcess::run), this)
	, finishedFlag(false)
	, runFlag(true) {
}

FileProcess::~FileProcess() {
	if (!finishedFlag) {
		runFlag = false;
		closesocket(so);
	}
	th.join();
}

bool FileProcess::finished() {
	return finishedFlag;
}

void FileProcess::run() {
	string data;
	int type = 0;
	if (parse(data, type)) {
		if (type == TYPE_CHECK) {
			processCheck(data);
		} else if (type == TYPE_UPLOAD) {
			processUpload(data);
		}
	}
	closesocket(so);

	finishedFlag = true;
}

bool FileProcess::parse(string & data, int & type) {
	uint8_t  bytes[HEAD_SIZE];

	//接收数据
	int ret = recv(so, (char*)bytes, HEAD_SIZE, 0);
	if (ret <= 0) {
		if (runFlag) CERR << "recv error :" << ret << endl;
		return false;
	}

	if (bytes[0] != 0xCC || bytes[1] != 0xBB) {
		COUT << "bad sync head" << endl;
		return false;
	}

	type = bytes[2];
	int32_t size = *((int32_t*)(bytes + 4));
	if (type != TYPE_CHECK && type != TYPE_UPLOAD) {
		COUT << "bad cmd type" << endl;
		return false;
	}

	if (size <= 0 || size > 1024 * 1024 * 10) {
		COUT << "body size :" << size << endl;
		return false;
	}
	data.resize(size);
	int recvSize = 0;

	while (recvSize < size) {
		ret = recv(so, &data[recvSize], size - recvSize, 0);
		if (ret <= 0) {
			if (runFlag)  CERR << "parse recv data error :" << ret <<",error:" << WSAGetLastError() << endl;
			break;
		}
		recvSize += ret;
	}

	if (recvSize < size)
		return false;

	return true;
}

void FileProcess::processCheck(const string & data) {
	istringstream is(data);
	string device, folder;
	is >> device >> folder;
	if (device.empty() || folder.empty()) {
		CERR << "device or folder is empty" << endl;
		sendError(1, "device or folder is empty!");
		return;
	}
	unwrapFileName(device);
	unwrapFileName(folder);

	vector<FileInfo> files;
	string path;
	int64_t fileSize;
	int64_t modifyTime;

	while (is >> path >> fileSize >> modifyTime) {
		unwrapFileName(path);
		files.push_back(FileInfo(path, modifyTime, fileSize));
	}
	if (files.empty()) {
		CERR << "no file to check" << endl;
		sendError(1, "no file to check!");
		return;
	}

	FileManager manager(rootDir, device);
	vector<string> newFiles = manager.checkFile(folder, files);
	COUT << "check folder device:" << device << ",folder:" << folder <<",new:"<< newFiles.size()<< endl;

	sendCheckResult(newFiles);
}

void FileProcess::processUpload(const string & data) {
	istringstream is(data);
	string device, folder, path;
	int64_t fileSize;
	int64_t modifyTime;
	is >> device >> folder >> path >> fileSize >> modifyTime;
	if (device.empty() || folder.empty() || path.empty()) {
		CERR << "device or folder  path is empty" << endl;
		sendError(1, "device or folder or path is empty!");
		return;
	}
	unwrapFileName(device);
	unwrapFileName(folder);
	unwrapFileName(path);

	FileManager manager(rootDir, device);
	string fullPath = manager.getFullPath(folder, path);

	COUT << "upload file start:" << fullPath << ",size:" << fileSize << endl;

	if (fileSize < 0) {
		CERR << "file size error!" << endl;
		sendError(1, "file size error!");
		return;
	}

	manager.createFileDir(fullPath);
	string fullPathTemp = fullPath + ".temp";

	if (_access(fullPath.c_str(), 0) == 0) {
		if (remove(fullPath.c_str()) != 0) {
			sendError(2, "delete old file fail!");
			return;
		}
	}

	ofstream os(fullPathTemp.c_str(), ios::out | ios::trunc | ios::binary);
	if (!os.is_open()) {
		CERR << "create file fail :" << fullPathTemp << endl;
		sendError(2, "create file fail!");
		return;
	}

	int64_t recvFileSize = 0;

	string fileData(1024 * 10, 0);
	while (recvFileSize < fileSize) {
		int ret = recv(so, &fileData[0], (int)fileData.size(), 0);
		if (ret <= 0) {
			if (runFlag)  CERR << "recv file data fail:" << fullPath <<",error:"<< WSAGetLastError() <<endl;
			break;
		}
		os.write(fileData.data(), ret);
		if (!os.good()) {
			CERR << "write file fail :" << fullPath << endl;
			sendError(3, "write file fail!");
			break;
		}

		recvFileSize += ret;
		if(recvFileSize<fileSize)//最后一次等文件处理完成再发
			sendUploadResult(recvFileSize);
	}
	os.close();
	if (recvFileSize < fileSize)
		return;

	if (recvFileSize >= fileSize) {
		if (0 != rename(fullPathTemp.c_str(), fullPath.c_str())) {
			CERR << "rename file fail :" << fullPath << ",errno:"<< errno <<endl;
			sendError(3, "rename file fail!");
			return;
		}
	}
	if (!manager.setModifyTime(fullPath, modifyTime)) {
		CERR << "set file modify time fail :" << fullPath << endl;
		sendError(3, "set file modify time fail!");
		return;
	}
	sendUploadResult(recvFileSize);
	COUT << "upload file success:" << fullPath<<endl;
}

void FileProcess::sendCheckResult(vector<string>& files) {
	ostringstream os;
	for (string file : files) {
		wrapFileName(file);
		os << file << endl;
	}
	string data = os.str();
	int32_t size = (int32_t)data.size();
	uint8_t  bytes[HEAD_SIZE] = { 0xCC,0xEE,0,0 };
	*((int32_t*)(bytes + 4)) = size;

	int ret = send(so, (char*)bytes, HEAD_SIZE, 0);
	if (ret < HEAD_SIZE) {
		if (runFlag)  CERR << "sendCheckResult:send  head error :" << ret << endl;
		return;
	}

	int sendSize = 0;
	while (sendSize < size) {
		ret = send(so, &data[sendSize], size - sendSize, 0);
		if (ret <= 0) {
			if (runFlag)  CERR << "sendCheckResult:send data error :" << ret << endl;
			break;
		}
		sendSize += ret;
	}
}

void FileProcess::sendUploadResult(int64_t recvSize) {
	ostringstream os;
	os << recvSize;
	string data = os.str();
	int32_t size = (int32_t)data.size();
	uint8_t  bytes[HEAD_SIZE] = { 0xCC,0xEE,0,0 };
	*((int32_t*)(bytes + 4)) = size;

	int ret = send(so, (char*)bytes, HEAD_SIZE, 0);
	if (ret < HEAD_SIZE) {
		if (runFlag)  CERR << "sendUploadResult:send  head error :" << ret << endl;
		return;
	}

	int sendSize = 0;
	while (sendSize < size) {
		ret = send(so, &data[sendSize], size - sendSize, 0);
		if (ret <= 0) {
			if (runFlag)  CERR << "sendUploadResult:send data error :" << ret << endl;
			break;
		}
		sendSize += ret;
	}
}

void FileProcess::sendError(uint8_t result, string reason) {

	int32_t size = (int32_t)reason.size();
	uint8_t  bytes[HEAD_SIZE] = { 0xCC,0xEE,result,0 };
	*((int32_t*)(bytes + 4)) = size;

	int ret = send(so, (char*)bytes, HEAD_SIZE, 0);
	if (ret < HEAD_SIZE) {
		if (runFlag)  CERR << "send  head error :" << ret << endl;
		return;
	}

	int sendSize = 0;
	while (sendSize < size) {
		ret = send(so, &reason[sendSize], size - sendSize, 0);
		if (ret <= 0) {
			if (runFlag)  CERR << "send data error :" << ret << endl;
			break;
		}
		sendSize += ret;
	}
}

void FileProcess::wrapFileName(string & name) {
	for (size_t i = 0; i < name.size(); i++) {
		if (name[i] == ' ')
			name[i] = '*';
	}
}

void FileProcess::unwrapFileName(string & name) {
	for (size_t i = 0; i < name.size(); i++) {
		if (name[i] == '*')
			name[i] = ' ';
	}
}
