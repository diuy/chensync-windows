#pragma once
#include <string>
#include <cstdint>
using namespace std;

class FileInfo
{
public:
	FileInfo(string path, int64_t modifyTime, int64_t fileSize)
		: path(path)
		, modifyTime(modifyTime)
		, fileSize(fileSize)
	{
	}

	string path;
	int64_t modifyTime;
	int64_t fileSize;
};

