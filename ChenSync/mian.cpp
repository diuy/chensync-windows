#include <iostream>
#include "FileManager.h"
#include "FileServer.h"
#include <fstream>
#include <sstream>
using namespace std;

int main(int argc, const char *  argv[]) {
	const char* dataDir = NULL;
	int port = 8888;

	if (argc < 2) {
		cout << "no data dir" << endl;
		return -1;
	}
	dataDir = argv[1];
	if (argc > 2) {
		istringstream is(argv[2]);
		is >> port;
	}


	cout << "data dir :" << dataDir << endl << "port:" << port << endl;

	FileServer server(dataDir,port);
	server.start();

	cin.get();
	server.stop();
}
