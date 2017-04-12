#include <iostream>
#include "FileManager.h"
#include "FileServer.h"
#include <fstream>
#include <sstream>
#include "Util.h"
using namespace std;


int startServer(int argc, const char *  argv[]);

int main(int argc, const char *  argv[]) {

	const char* dataDir = NULL;
	int port = 8888;

	if (argc < 2) {
		CERR << "arg error" << endl;
		return -1;
	}
	if (strcmp(argv[1], "server") == 0) {
		return startServer(argc, argv);
	}

	dataDir = argv[1];
	if (argc > 2) {
		istringstream is(argv[2]);
		is >> port;
	}

	COUT << "data dir :" << dataDir <<  ",port:" << port << endl;

	FileServer server(dataDir,port);
	if (server.start()) {
		COUT << "start success!" << endl;
	} else {
		COUT << "start fail!" << endl;
	}
	cin.get();
	server.stop();
}
