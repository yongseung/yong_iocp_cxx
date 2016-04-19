#ifndef _CFIEXEDSIZEQUEUE_H_
#define _CFIEXEDSIZEQUEUE_H_
#include "stdafx.h"
#include <windows.h>            
#include <process.h>     
#include "CSocket.h"
//#include "coder.h"

#define GAMEBUFEMPTY		NULL
#define GAMERINGBUFSIZE		8192


class CFiexedSizeQueue
{
private:
	int	iIndex,
		iBaseChannelIndex;
	CRITICAL_SECTION		cs;
	CSocket			iGameRingBuf[GAMERINGBUFSIZE];

	int						iBegin,
		iEnd,
		iRestCount;


	HANDLE					hGameTEvent;


public:

	void InitProcess(int idx);
	void GameBufEnqueue(CSocket lpSockContext);
	void GameBufDequeue(LPSOCKETCONTEXT *lpSockContext);

	friend unsigned int __stdcall GameTProc(void *pParam);

	// ---- Handler ----
	//friend int OnRequestLogin(LPSOCKETCONTEXT lpSockContext, char *cpPacket);
	//friend int OnRequestLogout(LPSOCKETCONTEXT lpSockContext, char *cpPacket);
	//friend int OnRequestLogoutInt(LPSOCKETCONTEXT lpSockContext, char *cpPacket);

};


int InitProcessLayer();


#endif
