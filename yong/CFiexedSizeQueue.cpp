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

	//ť�� �������� ���� ��Ŀ �����忡�� 1�� io �� 2�� �� ��ĥ�� �����Ƿ� 
	void CFiexedSizeQueue::SocketEnqueue(CSocket* pCSocket)
	{	
		
		smartCriticalSection myCS(&_cs);
		_pCSocketQueue[_iEnd++] = pCSocket;
		if (_iEnd >= FIXEDSIZEQUEUEMAX) _iEnd = 0;
		_iRestCount++;

		if (_iRestCount >= FIXEDSIZEQUEUEMAX)
			printf("---- GameRingBuf Overflow ----\n");
	}

	//ť���� �E���� �����Ͱ� �������� ���������� �A��.
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
		//ť���� ���� (ó���ؾ��� ������ �ּҸ� �ӽú����� �ּҸ� �ִ´�) ������ ����
	}
	
}
