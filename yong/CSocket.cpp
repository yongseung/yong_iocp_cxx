#include "stdafx.h"
#include "CSocket.h"


using namespace std;

namespace Ryan{
	bool CSocket::initSocket(int index, SOCKET sockListener){

		int dummy;
		DWORD	dwReceived;

		ZeroMemory(this, sizeof(CSocket)); //zeromemory 안하면 overlapped 구조체 에러

		_eovSendTcp.mode = SENDEOVTCP;
		_eovRecvTcp.mode = RECVEOVTCP;
		_index = index;
		_iSTRestCnt = 0;


		_iFixedQ = DEFAULTQUEUE;       //처음 로직큐
		_iChannel = DEFAULTCHANNEL;		//처음 채널

		//링버퍼 초기화
		_cpSTBegin = _cpSTEnd = _cSendTcpRingBuf;
		_cpRTMark = _cpRTBegin = _cpRTEnd = _cRecvTcpRingBuf;

		_sockTcp = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (_sockTcp == INVALID_SOCKET){ printf("invlaid_socket/n"); return 0; }


		//1새로운 연결 받기 2 지역 주소 원격 주소 반환 3 첫 데이터 블럭 수신 
		dummy = AcceptEx(sockListener,
			_sockTcp,                            //소켓, 버퍼, 오버랩 구조체, 니주소, 내주소
			_cpRTEnd, //ipOutputBuffer
			MAXRECVPACKETSIZE, //실제 데이터 수신 바이트 수 최대 크기 처음에 안받으면 0
			sizeof(sockaddr_in) + 16, //localaddress length tcp is 16 + 원격 까지 16
			sizeof(sockaddr_in) + 16, //remoteaddress length
			&dwReceived,              // 연결되자마자 받은 데이터의 크기이다.
			(OVERLAPPED *)&_eovRecvTcp); //plOverlapped       os가 생성해서 리턴해줌 이놈만 쓰라고 특별히 할당해줌


		if (dummy == FALSE && GetLastError() != ERROR_IO_PENDING) { 
			printf("duummy is %d accept fail is %d/n",dummy,GetLastError()); return 0; 
		}

		InitializeCriticalSection(&_csSTcp);
		return true;

	}
}
