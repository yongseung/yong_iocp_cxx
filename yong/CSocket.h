#ifndef _CSOCKET_H_
#define _CSOCKET_H_

#include <winsock2.h>
#include <mswsock.h>            
#include <windows.h>            
#include <process.h>      
#include <WS2tcpip.h>

#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "mswsock.lib" )    

#define MAXSENDPACKETSIZE	512
#define MAXRECVPACKETSIZE	512

#define RINGBUFSIZE			2048 //32768
#define MAXTRANSFUNC		128
#define HEADERSIZE			4
#define DEFAULTQUEUE		0
#define DEFAULTCHANNEL		0

using namespace std;

namespace Ryan{
	enum
	{
		RECVEOVTCP = 0, SENDEOVTCP,
	};

	typedef struct
	{
		OVERLAPPED				ovl;
		int						mode;    //0 = recv tcp, 1 send tcp
	}EOVERLAPPED, *LPEOVERLAPPED;


	class CSocket{
	public:
		bool initSocket(int, SOCKET);

		CSocket(){};
		~CSocket(){
			DeleteCriticalSection(&_csSTcp);
		};


		void setSendBegin(int size){ _cpSTBegin += size;};
		void setSendBeginFromZero(int size){
			char* origin = _cSendTcpRingBuf;
			_cpSTBegin = origin + size;
		}
		void setSendEnd(int size){ _cpSTEnd += size; };
		void setSendEndFromZero(int size){
			char* origin = _cSendTcpRingBuf;
			_cpSTEnd = origin + size;
		}

		void setRecvBeginAdd(int size){ _cpRTBegin += size; };
		void setRecvBegin(char* position){ _cpRTBegin = position; };

		void setRecvBeginFromZero(int size){
			char* origin = _cRecvTcpRingBuf;
			_cpRTBegin = origin + size;
		}
		void setRecvMark(int size){ _cpRTMark += size; };
		void setRecvEnd(int size){ _cpRTEnd += size; };
		void setRecvEndFromZero(int size){
			char* origin = _cRecvTcpRingBuf;
			_cpRTEnd = origin + size;
		};

		void setRestCnt(int size){ 
			//size 0 이면 종료 요청
			if (size == 0){
				_iSTRestCnt = 0;
			}
			else{
			_iSTRestCnt += size;
			}
		};
		void copyRecvRingBufOfZero(char value){ _cRecvTcpRingBuf[0] = value; } //링버퍼 body size or type 구하기 위해서
		void setFixedQ(int size){ _iFixedQ = size; }// logic 쓰레드를 결정한다

		char* getAddr()	{ return _addr; };

		char* getSendRingBuf(){ return _cSendTcpRingBuf; };
		char* getSendBegin(){ return _cpSTBegin; };
		char* getSendEnd(){ return _cpSTEnd; };

		char* getRecvRingBuf(){ return _cRecvTcpRingBuf; };
		char* getRecvBegin(){ return _cpRTBegin; };
		char* getRecvEnd(){ return _cpRTEnd; };
		char* getRecvMark(){ return _cpRTMark; };

		int   getIndex()	const	{ return _index; }
		int	  getRestCnt()	const	{ return _iSTRestCnt; };
		int	  getFiexedQ()	const	{ return _iFixedQ; };

		//소켓이 초기화될때 링버퍼의 포인터 초기화
		void	reInitBuf(){
			_cpSTBegin = _cpSTEnd =_cSendTcpRingBuf;
			_cpRTMark = _cpRTBegin = _cpRTEnd = _cRecvTcpRingBuf;
			cout << _index << "buf init compelete";
		}
	
		//소켓 초기화될때 소켓 핸들을 재할당 한다 기존의 것은 파기
		void	setSocket(){ 
			_sockTcp = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (_sockTcp == INVALID_SOCKET) {
				cout << "setSocket fail"<<endl;
			}
		}
	
		SOCKET&	getSocket(){ return _sockTcp; };

		void  setRemoteAddr(SOCKADDR_IN* remote){ 
			CopyMemory(&_remoteAddr, remote, sizeof(sockaddr_in));
			//printf("CSOCKETHEADER::port is %d\n", ntohs(_remoteAddr.sin_port)); // 쓰레기 주소 나옴
			inet_ntop(AF_INET, &(_remoteAddr.sin_addr), _addr, sizeof _addr);
			printf("CSCOKETHEADER::%s:%d\n", _addr, ntohs(_remoteAddr.sin_port));
		};

		int getChannel()	const	{ return _iChannel; };
		void setChannel(int idx){ _iChannel = idx; };
	
		CRITICAL_SECTION& getCS(){ return _csSTcp; }      //worker 쓰레드가 send 할때와 logic의 send 시 충돌을 막기 위해

		//오버랙 변수는 전역으로 외부에서 이것을 통해 데이터를 가져오므로
		EOVERLAPPED		_eovRecvTcp, _eovSendTcp;		

	private:
			// 오버랩 구조체 0으로 초기화 않되면 ERROR_INVALID_PARAMETER 가 리턴된다

			SOCKET					_sockTcp; //overapped io 지원하는 소켓
			sockaddr_in				_remoteAddr;// 원격 주소 알아야 함 포트랑
			CRITICAL_SECTION		_csSTcp;				 //초기화

			char _cRecvTcpRingBuf[RINGBUFSIZE + MAXRECVPACKETSIZE],
				*_cpRTBegin,
				*_cpRTEnd,
				*_cpRTMark,

				 _cSendTcpRingBuf[RINGBUFSIZE + MAXSENDPACKETSIZE],
				*_cpSTBegin,
				*_cpSTEnd,
				_addr[33];

			int						_iSTRestCnt; //send tcp 에서 보내야할 데이터의 크기
			int						_index;
			int						_iChannel;//어느 채널인지
			int                     _iFixedQ;//어느 큐인지 

	};

}
#endif
