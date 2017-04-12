#include <cstdio>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <cstdio>
#include <stdlib.h>  
#include <sys/types.h>  
#include <sys/utime.h>  
#include <time.h>  
#include "FileManager.h"
#include  "Util.h"


FileManager::FileManager(string rootDir, string deviceName)
{
	homeDir = combinePath(rootDir, deviceName);
}


vector<string> FileManager::checkFile(string folder, const vector<FileInfo> &files)
{
	string fullFolder = combinePath(homeDir, folder);

	vector<string> newFiles;
	if (_access(fullFolder.c_str(), 0) != 0) {
		for (FileInfo file : files) {
			newFiles.push_back(file.path);
		}
		return newFiles;
	}

	for (FileInfo file : files) {
		if (checkFile(fullFolder,file)) {
			newFiles.push_back(file.path);
		}
	}

	return newFiles;
}

string FileManager::getFullPath(string folder, string file)
{
	for (size_t i = 0; i < file.length(); i++) {
		if (file[i] == '/')
			file[i] = '\\';
	}

	string fullFolder = combinePath(homeDir, folder);
	return combinePath(fullFolder, file);
}

bool FileManager::setModifyTime(string file, int64_t modifyTime) {
	struct utimbuf t = { modifyTime ,modifyTime };
	return utime(file.c_str(),&t) == 0;
}

bool FileManager::createFileDir(string file) {
	if (file.empty())
		return true;

	size_t pos = file.rfind("\\");
	if (pos == string::npos) {
		return true;
	}

	return mkdirs(file.substr(0,pos));
}

bool FileManager::checkFile(string folder,FileInfo file)
{
	string fileTemp = file.path;
	for (size_t i = 0; i < fileTemp.length(); i ++) {
		if (fileTemp[i] == '/')
			fileTemp[i] = '\\';
	}
	string fullPath = combinePath(folder, fileTemp);

	struct stat st = { 0 };

	if (stat(fullPath.c_str(), &st) != 0)
		return true;

	return file.fileSize != st.st_size ||
		file.modifyTime != st.st_mtime;
}

string FileManager::combinePath(string left, string right)
{
	if (left.empty())
		return right;

	if (right.empty())
		return left;


	if (left[left.length() - 1] != '\\') {
		left += "\\";
	}

	if (right[0] == '\\')
		right = right.substr(1);

	return left + right;
}

bool FileManager::mkdirs(string dir)
{
	if (dir.empty())
		return true;

	if (*dir.rbegin() == '\\') {
		*dir.rbegin() = '\0';
	}

	if (_access(dir.c_str(), 0) == 0)
		return true;

	char* tmp = &dir[0];
	char *p;

	p = strrchr(tmp, '\\');

	if (p) {
		*p = '\0';
		if (!mkdirs(tmp))
			return false;
		*p = '\\';
	}

	return _mkdir(tmp) == 0;
}

