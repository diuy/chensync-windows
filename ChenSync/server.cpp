#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers  
#include "windows.h"    
#include <iostream>  
#include <memory>
#include <sstream>
#include <fstream>
#include "FileServer.h"

using namespace std;


SERVICE_STATUS          gSvcStatus;  //服务状态    
SERVICE_STATUS_HANDLE   gSvcStatusHandle; //服务状态句柄    
HANDLE                  ghSvcStopEvent = NULL;//服务停止句柄    
#define SERVER_NAME    TEXT("ChenSync") //服务名    
VOID WINAPI Server_main(DWORD, LPTSTR *); //服务入口点    
void ServerReportEvent(LPTSTR szName, LPTSTR szFunction); //写Windows日志    
VOID ReportSvcStatus(DWORD, DWORD, DWORD); //服务状态和SCM交互    
VOID WINAPI SvcCtrlHandler(DWORD);  //SCM回调函数 

static bool start(const char* command);
static void stop();


int startServer(int argc, const char *  argv[]) {

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SERVER_NAME, (LPSERVICE_MAIN_FUNCTION)Server_main },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		ServerReportEvent(SERVER_NAME, TEXT("StartServiceCtrlDispatcher"));
		return  -1;
	}
	return 0;
}

static VOID WINAPI Server_main(DWORD dwArgc, LPTSTR *lpszArgv) {

	//ofstream os("D:1.txt", ios::out | ios::trunc | ios::binary);
	//os << ::GetCommandLine() << endl;
	//os.close();

	// Register the handler function for the service    
	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SERVER_NAME,
		SvcCtrlHandler);

	if (!gSvcStatusHandle) {
		ServerReportEvent(SERVER_NAME, TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here    
	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; //只有一个单独的服务    
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM    
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.    
	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes    
		TRUE,    // manual reset event    
		FALSE,   // not signaled    
		NULL);   // no name    

	if (ghSvcStopEvent == NULL) {
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	if (!start(GetCommandLine())) {
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.    
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);


	while (1) {
		// Check whether to stop the service.    
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		stop();
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}
void ServerReportEvent(LPTSTR szName, LPTSTR szFunction) {
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	unsigned int len = sizeof(szFunction);
	TCHAR *Buffer = new TCHAR[len];

	hEventSource = RegisterEventSource(NULL, szName);

	if (NULL != hEventSource) {
		//StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());    
		strcpy((char*)Buffer, (char*)szFunction);
		lpszStrings[0] = szName;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle    
			EVENTLOG_ERROR_TYPE, // event type    
			0,                   // event category    
			0,           // event identifier    
			NULL,                // no security identifier    
			2,                   // size of lpszStrings array    
			0,                   // no binary data    
			lpszStrings,         // array of strings    
			NULL);               // no binary data        
		DeregisterEventSource(hEventSource);
	}
}
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint) {
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.    

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.    
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl) {
	// Handle the requested control code.     
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		// Signal the service to stop.    
		SetEvent(ghSvcStopEvent);
		return;

	case SERVICE_CONTROL_INTERROGATE:
		// Fall through to send current status.    
		break;

	default:
		break;
	}
	ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
}

static FILE *fout=NULL,*ferr=NULL;
static unique_ptr<FileServer> fileServer;

static string nowTimeStr() {
	char str[255];
	time_t t= time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y%m%d", t2);
	return string(str);
}

static void setupLog(string exe) {
	size_t pos = exe.rfind("\\");
	if (pos != string::npos) {
		string coutfile = exe.substr(0, pos);
		string cerrfile = coutfile;
		string str = nowTimeStr();
		coutfile.append("\\").append("cout_").append(str).append(".txt");
		cerrfile.append("\\").append("cerr_").append(str).append(".txt");
		fout = freopen(coutfile.c_str(), "a+", stdout);
		ferr = freopen(cerrfile.c_str(), "a+", stderr);
		setbuf(fout, NULL);
		setbuf(ferr, NULL);
	}
}

static bool start(const char* command) {
	
	int port = 8888;
	string exe, cmd, path;
	istringstream is(command);
	is >> exe >> cmd >> path >> port;
	setupLog(exe);

	if (path.empty()) {
		cerr << "no data dir" << endl;
		return false;
	}
	cout << "data dir :" << path << endl << "port:" << port << endl;
	fileServer.reset(new FileServer(path, port));

	if (fileServer->start()) {
		cout << "start success!" << endl;
		return true;
	} else {
		cerr << "start fail!" << endl;
		return false;
	}
}

static void stop() {
	fileServer->stop();
	if (fout) fclose(fout);
	if (ferr)  fclose(ferr);
}