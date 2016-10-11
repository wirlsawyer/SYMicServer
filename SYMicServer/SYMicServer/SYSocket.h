#ifndef _SYSOCKET_H_
#define _SYSOCKET_H_

#include <winsock2.h>
#pragma comment (lib, "Ws2_32.lib")
//[Thread lib]
#include <Windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <vector>
#include <queue>

#ifdef BCB
//#include <Windows.h>
#else
#endif

enum SYTCPSocketEventStatus
{
	SYTCPSOCKET_CLOSE = 0,
	SYTCPSOCKET_CONNECTFAULT,
	SYTCPSOCKET_CONNECTED,
	SYTCPSOCKET_DISCONNECT,
	SYTCPSOCKET_LISTENED,
	SYTCPSOCKET_RECVDATA
};

struct SYTCPEvent
{
	SYTCPSocketEventStatus	Status;
	wchar_t					wszMsg[256];
	char					szData[1024];
	int						iLen;
};



class SYTCPConnectParam
{
public:
	SYTCPConnectParam()
	{
		_hRecvProcThread = NULL;	
		Socket = NULL;
		IsClient = true;
	}
	~SYTCPConnectParam()
	{
		if (_hRecvProcThread)
		{
			TerminateThread(_hRecvProcThread, 0);
			_hRecvProcThread = NULL;
		}

		closesocket(Socket);
	}
	//Recv Data
	PVOID   *pThis;
	HANDLE	_hRecvProcThread;	
	SOCKET  Socket;
	bool   IsClient;
};

class SYTCPSocket
{
public:
	SYTCPSocket()
	{
		Socket    = -1;
		LocalPort = -1;

		_hListenProcThread  = NULL;
		_hConnectProcThread = NULL;
		_hRecvProcThread	= NULL;
		_hSendProcThread	= NULL;
		
		_pVecRecvThread = new std::vector<SYTCPConnectParam*>;
		_pVecSendData = new std::vector<char*>;
	}
	~SYTCPSocket()
	{
		this->Close();
		delete _pVecRecvThread;
		delete _pVecSendData;
	}

	SOCKET  Socket;

	//Server
	USHORT  LocalPort;
	//Client
	char*   RemoteHost;
	USHORT	RemotePort ;
	//Event
	void (*OnEvent)(SYTCPSocket *sender, SYTCPEvent e);
	

	void Close()
	{
		//wchar_t wszBuf[256];
		shutdown(Socket, SD_BOTH);		
		closesocket(Socket);
		WSACleanup();
		OutputDebugString(L"Close:BeginProcThread\n");
		if (_hRecvProcThread)
		{
			TerminateThread(_hRecvProcThread, 0);
			_hRecvProcThread = NULL;
			OutputDebugString(L"Close:_hRecvProcThread\n");
		}

		if (_hSendProcThread)
		{
			TerminateThread(_hSendProcThread, 0);
			_hSendProcThread = NULL;
			OutputDebugString(L"Close:_hSendProcThread\n");
		}

		if (_hListenProcThread)
		{
			TerminateThread(_hListenProcThread, 0);
			_hListenProcThread = NULL;
			OutputDebugString(L"Close:_hListenProcThread\n");
		}

		if (_hConnectProcThread)
		{
			TerminateThread(_hConnectProcThread, 0);
			_hConnectProcThread = NULL;
			OutputDebugString(L"Close:_hConnectProcThread\n");
		}		

		
		
		//clear
		while(!_pVecRecvThread->empty())
		{
			TerminateThread(_pVecRecvThread->back(), 0);
			delete _pVecRecvThread->back();	
			_pVecRecvThread->pop_back();
		}	
		

		while(!_pVecSendData->empty())
		{
			free(_pVecSendData->back());
			_pVecSendData->pop_back();
		}

		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CLOSE;
		this->OnEvent(this, e);
	}

	void Listen(void)
	{
		if (_hListenProcThread) this->Close();
		//Listen
		_hListenProcThread = CreateThread(NULL, 0, ListenProcThread, this, 0, NULL);
		_hSendProcThread = CreateThread(NULL, 0, SendProcThread, this, 0, NULL);
	}

	static DWORD WINAPI ListenProcThread(void *pParam)
	{
		SYTCPSocket *pSYSocket = (SYTCPSocket*)pParam;	

		
		wchar_t wszBuf[256];

		//----------------------
		// Initialize Winsock
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (iResult != NO_ERROR) {
			//OutputDebugString(L"Error at WSAStartup()\n");
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", L"Error at WSAStartup()\n");
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}			

		//----------------------
		// Create a SOCKET for listening for
		// incoming connection requests.		
		pSYSocket->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (pSYSocket->Socket == INVALID_SOCKET) 
		{
			memset(wszBuf, 0, sizeof(wszBuf));
			wsprintf(wszBuf, L"Error at socket(): %ld\n", WSAGetLastError());
			//OutputDebugString(wszBuf);		
			WSACleanup();
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", wszBuf);
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}


		//----------------------
		// The sockaddr_in structure specifies the address family,
		// IP address, and port for the socket that is being bound.	
		sockaddr_in service_addr;
		service_addr.sin_family = AF_INET;
		service_addr.sin_addr.s_addr = INADDR_ANY;
		service_addr.sin_port = htons(pSYSocket->LocalPort);

		

		if (bind( pSYSocket->Socket, (SOCKADDR*) &service_addr, sizeof(service_addr)) == SOCKET_ERROR) 
		{				
			//OutputDebugString(L"bind() failed.\n");
			closesocket(pSYSocket->Socket);
			WSACleanup();
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", L"bind() failed.\n");
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}


		//----------------------
		// Listen for incoming connection requests 
		// on the created socket
		if (listen( pSYSocket->Socket, 1 ) == SOCKET_ERROR)
		{
			//OutputDebugString(L"Error listening on socket.\n");
			closesocket(pSYSocket->Socket);
			WSACleanup();
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", L"Error listening on socket.\n");
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}
			

		memset(wszBuf, 0, sizeof(wszBuf));
		wsprintf(wszBuf, L"Listening on socket....Port:%d\n", pSYSocket->LocalPort);
		//OutputDebugString(wszBuf);	

		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_LISTENED;
		wsprintf(e.wszMsg, L"%s", wszBuf);
		pSYSocket->OnEvent(pSYSocket, e);

		
		while(true)
		{  

			int sin_size = sizeof(struct sockaddr_in);

			sockaddr_in client_addr;
			SOCKET subSocket = accept(pSYSocket->Socket, (struct sockaddr *)&client_addr,&sin_size);

			memset(wszBuf, 0, sizeof(wszBuf));
			wsprintf(wszBuf, L"Server got a connection from (%S, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
			OutputDebugString(wszBuf);

			
			SYTCPConnectParam *pRecvParam = new SYTCPConnectParam();	
			pSYSocket->_pVecRecvThread->push_back(pRecvParam);
			pRecvParam->pThis = (PVOID *)pSYSocket;
			pRecvParam->Socket = subSocket;
			pRecvParam->IsClient = false;
			
			//Recv
			pRecvParam->_hRecvProcThread = CreateThread(NULL, 0, ServerRecvProcThread, pRecvParam, 0, NULL);

			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTED;
			wsprintf(e.wszMsg, L"Server got a connection from (%S, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
			pSYSocket->OnEvent(pSYSocket, e);

			//delete pRecvParam;
			//closesocket(subSocket);
		}

		pSYSocket->Close();
		WSACleanup();	
	}

	void Connect(void)
	{
		
		//Connect
		_hConnectProcThread = CreateThread(NULL, 0, ConnectProcThread, this, 0, NULL);

		
		/*
		SYTCPConnectParam *pRecvParam = new SYTCPConnectParam();
		pRecvParam->pThis = (PVOID *)this;
		pRecvParam->Socket = Socket;
		pRecvParam->IsClient = true;
		*/
		//Recv
		_hRecvProcThread = CreateThread(NULL, 0, ClientRecvProcThread, this/*pRecvParam*/, 0, NULL);
		_hSendProcThread = CreateThread(NULL, 0, SendProcThread, this, 0, NULL);
		//delete pRecvParam;
	}

	static DWORD WINAPI ConnectProcThread(void *pParam)
	{
		SYTCPSocket *pSYSocket = (SYTCPSocket*)pParam;

		wchar_t wszBuf[256];
		//----------------------
		// Initialize Winsock
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (iResult != NO_ERROR){
			//OutputDebugString(L"Error at WSAStartup()\n");
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", L"Error at WSAStartup()\n");
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}			


		//----------------------
		// Create a SOCKET for connecting to server		
		pSYSocket->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (pSYSocket->Socket == INVALID_SOCKET) 
		{			
			memset(wszBuf, 0, sizeof(wszBuf));
			wsprintf(wszBuf, L"Error at socket(): %ld\n", WSAGetLastError());
			//OutputDebugString(wszBuf);	
			WSACleanup();
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", wszBuf);
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}

		//----------------------
		// The sockaddr_in structure specifies the address family,
		// IP address, and port of the server to be connected to.		
		sockaddr_in client_addr;
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = inet_addr( pSYSocket->RemoteHost );
		client_addr.sin_port = htons( pSYSocket->RemotePort );

		//----------------------
		// Connect to server.
		if ( connect( pSYSocket->Socket, (SOCKADDR*) &client_addr, sizeof(client_addr) ) == SOCKET_ERROR) {
			//OutputDebugString(L"Failed to connect.\n");
			WSACleanup();
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_CONNECTFAULT;
			wsprintf(e.wszMsg, L"%s", L"Failed to connect.\n");
			pSYSocket->OnEvent(pSYSocket, e);
			return false;
		}

		//OutputDebugString(L"Client connected to server.\n");
		
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTED;
		wsprintf(e.wszMsg, L"%s", L"Client connected to server.\n");
		pSYSocket->OnEvent(pSYSocket, e);
		
		return true;
		//WSACleanup();
	}

	void Send(const char* pData)
	{

		if (_hSendProcThread == NULL) return; //還未連線
		
		int iLen = (int)strlen(pData)+1;
		char *pBuffer = (char *)malloc(sizeof(char)*(iLen));
		memcpy(pBuffer, pData, sizeof(char)*(iLen));
		_pVecSendData->push_back(pBuffer);
	}


	static DWORD WINAPI ClientRecvProcThread(void *pParam)
	{
		//SYTCPConnectParam *pRecvParm = (SYTCPConnectParam *)pParam;
		SYTCPSocket *pThis = (SYTCPSocket *)pParam; //(SYTCPSocket *)pRecvParm->pThis;
		SOCKET Socket = pThis->Socket;
		
		while (true)
		{
			char recv_data[1024];       
			int bytes_recieved = recv(Socket, recv_data, 1024, 0);
			recv_data[bytes_recieved] = '\0';
			if (bytes_recieved >0)
			{
				/*
				if (pRecvParm->IsClient) {
					OutputDebugString(L"Client Recv Data:");
				}else{
					OutputDebugString(L"Server Recv Data:");
				}
				*/

				//OutputDebugStringA(recv_data);
				//OutputDebugString(L"\n");

				//Event
				SYTCPEvent e;
				ZeroMemory(&e, sizeof(e));
				e.Status = SYTCPSOCKET_RECVDATA;
				memcpy(e.szData, recv_data, sizeof(recv_data) );				
				pThis->OnEvent(pThis, e);				
			}	

			if (bytes_recieved ==0)
			{
				//OutputDebugString(L"Socket Is Close\n");
				//Event
				SYTCPEvent e;
				ZeroMemory(&e, sizeof(e));
				e.Status = SYTCPSOCKET_DISCONNECT;
				memset(e.szData, 0, sizeof(e.szData));				
				pThis->OnEvent(pThis, e);
				break;
			}
		}
		//delete pRecvParm;
		return true;
	}

	static DWORD WINAPI ServerRecvProcThread(void *pParam)
	{
		SYTCPConnectParam *pRecvParm = (SYTCPConnectParam *)pParam;
		SYTCPSocket *pThis = (SYTCPSocket *)pRecvParm->pThis;
		SOCKET Socket = pRecvParm->Socket;//*((SOCKET *)pRecvParm->pSocket);

		while (true)
		{
			char recv_data[1024];       
			int bytes_recieved = recv(Socket, recv_data, 1024, 0);
			recv_data[bytes_recieved] = '\0';
			if (bytes_recieved >0)
			{
				/*
				if (pRecvParm->IsClient) {
				OutputDebugString(L"Client Recv Data:");
				}else{
				OutputDebugString(L"Server Recv Data:");
				}
				*/

				//OutputDebugStringA(recv_data);
				//OutputDebugString(L"\n");

				//Event
				SYTCPEvent e;
				ZeroMemory(&e, sizeof(e));
				e.Status = SYTCPSOCKET_RECVDATA;
				e.iLen = bytes_recieved; 
				memcpy(e.szData, recv_data, sizeof(recv_data) );				
				pThis->OnEvent(pThis, e);				
			}	

			if (bytes_recieved ==0)
			{
				//OutputDebugString(L"Socket Is Close\n");
				//Event
				SYTCPEvent e;
				ZeroMemory(&e, sizeof(e));
				e.Status = SYTCPSOCKET_DISCONNECT;
				memset(e.szData, 0, sizeof(e.szData));				
				pThis->OnEvent(pThis, e);
				break;
			}
		}
		delete pRecvParm;
		return true;
	}

	
	static DWORD WINAPI SendProcThread(void *pParam)
	{
		
		//SYTCPConnectParam *pRecvParm = (SYTCPConnectParam *)pParam;
		//SOCKET Socket = *((SOCKET *)pRecvParm->pSocket);
		SYTCPSocket *pThis = (SYTCPSocket *)pParam;

		while(true)
		{
			if (pThis->_pVecSendData->size() > 0)
			{
				
				if (pThis->_hListenProcThread)
				{
					char *pData = pThis->_pVecSendData->at(0);				

					for (unsigned int i=0; i<pThis->_pVecRecvThread->size(); i++)
					{
						SYTCPConnectParam *param = pThis->_pVecRecvThread->at(i);

						int iResult = send(param->Socket, (const char *)pData, (int)strlen(pData), 0); 
						OutputDebugString(L"Server has send data\n");
					}
				}

				if (pThis->_hConnectProcThread)
				{
					char *pData = pThis->_pVecSendData->at(0);
					int iResult = send(pThis->Socket, (const char *)pData, (int)strlen(pData), 0); 
					OutputDebugString(L"Client has send data\n");
					
				}

				std::vector<char *>::iterator iter = pThis->_pVecSendData->begin();
				free(*iter);
				pThis->_pVecSendData->erase(iter);
			}//end if (pThis->_pVecSendData->size())
			
		}//end while
	
		return true;
	}

protected:

private:
	//Server
	HANDLE	_hListenProcThread;
	std::vector<SYTCPConnectParam*>* _pVecRecvThread;
	std::vector<char*>* _pVecSendData;
	//Client
	HANDLE	_hConnectProcThread;
public:
	//Recv Data
	HANDLE	_hRecvProcThread;
	//Send Data
	HANDLE	_hSendProcThread;

};

#endif