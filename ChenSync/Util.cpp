#include "Util.h"
#include <time.h>
std::string nowTimeStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", t2);
	return std::string(str);
}

std::string nowDateStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y%m%d", t2);
	return std::string(str);
}
