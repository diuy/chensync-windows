#include <iostream>
#include "FileManager.h"
#include "FileServer.h"
#include <fstream>
#include <sstream>
using namespace std;


int startServer(int argc, const char *  argv[]);

int main(int argc, const char *  argv[]) {

	const char* dataDir = NULL;
	int port = 8888;

	if (argc < 2) {
		cerr << "arg error" << endl;
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

	cout << "data dir :" << dataDir << endl << "port:" << port << endl;

	FileServer server(dataDir,port);
	if (server.start()) {
		cout << "start success!" << endl;
	} else {
		cout << "start fail!" << endl;
	}
	cin.get();
	server.stop();
}
