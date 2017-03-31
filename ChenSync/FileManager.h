#pragma once
#include <string>
#include <vector>
#include "File.h"
using namespace std;

class FileManager
{
private:
	string homeDir;
public:
	FileManager(string rootDir, string deviceName);
	vector<string> checkFile(string folder,const vector<File> &file);
	string getFullPath(string folder, string file);
	bool setModifyTime(string file, int64_t modifyTime);
	bool createFileDir(string file);
private:
	bool checkFile(string folder,File file);
	string combinePath(string left, string right);
	bool mkdirs(string dir);
};

