#include "stdafx.h"
#include "CFiexedSizeQueue.h"


namespace Ryan{

	void CFiexedSizeQueue::InitProcess(int idx){
		int						dummy;
		_iIndex = 0;
		_iBegin = 0;
		_iEnd = 0;
		_iRestCount = 0;
		InitializeCriticalSection(&_cs);
	}

	//큐에 넣을때는 여러 워커 쓰레드에서 1번 io 와 2번 이 겹칠수 있으므로 
	void CFiexedSizeQueue::SocketEnqueue(CSocket* pCSocket)
	{	
		
		smartCriticalSection myCS(&_cs);
		_pCSocketQueue[_iEnd++] = pCSocket;
		if (_iEnd >= FIXEDSIZEQUEUEMAX) _iEnd = 0;
		_iRestCount++;

		if (_iRestCount >= FIXEDSIZEQUEUEMAX)
			printf("---- GameRingBuf Overflow ----\n");
	}

	//큐에서 뺼때는 데이터가 있을때만 순차적으로 뺸다.
	void CFiexedSizeQueue::SocketDequeue(CSocket** pCSocket)
	{

		if (_iRestCount <= 0)
		{
			*pCSocket = FIXEDQUEUEEMPTY;
			return;
		}
		*pCSocket = _pCSocketQueue[_iBegin++];
		if (_iBegin >= FIXEDSIZEQUEUEMAX) _iBegin = 0;
		_iRestCount--;
		//큐에서 빼서 (처리해야할 소켓의 주소를 임시변수에 주소를 넣는다) 데이터 복사
	}
	
}
