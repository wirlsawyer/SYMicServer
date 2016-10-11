// SYMicServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SYSocket.h"
#include <conio.h> // for _getch()
#include <PlaySound.h>
#pragma comment(lib, "PlaySound.lib")
//#pragma comment(lib, "SDL.lib")

void OnSYTCPSocketEvent(SYTCPSocket *sender, SYTCPEvent e);


#include "SYSaveWave.h"
SYSaveWave *g_sywav = NULL;
////////////////////////////////////////////////////////////////
//	char *pAniFilename = NULL;
//	SYTool::Instance()->WCharTochar(appPath.c_str(), &pAniFilename);
//  delete pAniFilename;
void WCharTochar(const wchar_t *source, char **dest)
{
	*dest = (char *)new char[2 * wcslen(source)+1] ;
	memset(*dest , 0 , 2 * wcslen(source)+1 );

	int   nLen   =   (int)wcslen(source)+1;
	WideCharToMultiByte(CP_ACP,   0,   source,   nLen,   *dest,   2*nLen,   NULL,   NULL);
	//OutputDebugStringA((*dest));
}




int _tmain(int argc, _TCHAR* argv[])
{

	g_sywav = new SYSaveWave();

	if (!CPlaySound::Init(AUDIO_U8, 1, /*44100*/8000, 500)) return 0;
	if (!CPlaySound::Play()) return 0;

	



	SYTCPSocket *_pServerSocket = new SYTCPSocket();	//By Sawyer
	_pServerSocket->OnEvent  = &OnSYTCPSocketEvent;	//By Sawyer
	_pServerSocket->LocalPort = 5000;
	_pServerSocket->Listen();
	
	char c;
	while(c = _getch()) 
	{

		switch(c)
		{
		case 'q':
			 goto stop;
		    break;
		default:
			break;
		}
	}
	stop:

	g_sywav->Close();
	delete _pServerSocket;
	if (!CPlaySound::Release()) return 0;
	
	system("PAUSE");
	return 0;
}

void OnSYTCPSocketEvent(SYTCPSocket *sender, SYTCPEvent e)
{
	

	switch (e.Status)
	{
	case SYTCPSOCKET_CLOSE:
		printf("SYTCPSOCKET_CLOSE\n");
		break;

	case SYTCPSOCKET_RECVDATA:
		printf("SYTCPSOCKET_RECVDATA Len=%d\n", e.iLen);
		//printf(e.szData);
		//printf("\n");
		
		for (int i = 0; i < e.iLen; i++) e.szData[i]+=128; 
		g_sywav->AddData(e.szData, e.iLen);
		if (!CPlaySound::Push(e.szData, e.iLen)){

		}
		break;

	case SYTCPSOCKET_CONNECTED:
		printf("SYTCPSOCKET_CONNECTED\n");
		break;

	case SYTCPSOCKET_CONNECTFAULT:
		printf("SYTCPSOCKET_CONNECTFAULT\n");
		break;

	case SYTCPSOCKET_DISCONNECT:
		printf("SYTCPSOCKET_DISCONNECT\n");
		break;

	case SYTCPSOCKET_LISTENED:
		printf("SYTCPSOCKET_LISTENED\n");
		break;
	}

	char *pAni = NULL;
	WCharTochar(e.wszMsg, &pAni);
	printf(pAni);
	delete pAni;
	
}