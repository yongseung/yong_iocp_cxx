#include "stdafx.h"
#include "CSocket.h"


using namespace std;

namespace Ryan{
	bool CSocket::initSocket(int index, SOCKET sockListener){

		int dummy;
		DWORD	dwReceived;

		ZeroMemory(this, sizeof(CSocket)); //zeromemory ���ϸ� overlapped ����ü ����

		_eovSendTcp.mode = SENDEOVTCP;
		_eovRecvTcp.mode = RECVEOVTCP;
		_index = index;
		_iSTRestCnt = 0;


		_iFixedQ = DEFAULTQUEUE;       //ó�� ����ť
		_iChannel = DEFAULTCHANNEL;		//ó�� ä��

		//������ �ʱ�ȭ
		_cpSTBegin = _cpSTEnd = _cSendTcpRingBuf;
		_cpRTMark = _cpRTBegin = _cpRTEnd = _cRecvTcpRingBuf;

		_sockTcp = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (_sockTcp == INVALID_SOCKET){ printf("invlaid_socket/n"); return 0; }


		//1���ο� ���� �ޱ� 2 ���� �ּ� ���� �ּ� ��ȯ 3 ù ������ �� ���� 
		dummy = AcceptEx(sockListener,
			_sockTcp,                            //����, ����, ������ ����ü, ���ּ�, ���ּ�
			_cpRTEnd, //ipOutputBuffer
			MAXRECVPACKETSIZE, //���� ������ ���� ����Ʈ �� �ִ� ũ�� ó���� �ȹ����� 0
			sizeof(sockaddr_in) + 16, //localaddress length tcp is 16 + ���� ���� 16
			sizeof(sockaddr_in) + 16, //remoteaddress length
			&dwReceived,              // ������ڸ��� ���� �������� ũ���̴�.
			(OVERLAPPED *)&_eovRecvTcp); //plOverlapped       os�� �����ؼ� �������� �̳� ����� Ư���� �Ҵ�����


		if (dummy == FALSE && GetLastError() != ERROR_IO_PENDING) { 
			printf("duummy is %d accept fail is %d/n",dummy,GetLastError()); return 0; 
		}

		InitializeCriticalSection(&_csSTcp);
		return true;

	}
}
