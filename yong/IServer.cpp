#include "stdafx.h"
#include "IServer.h"
#include <chrono>

using namespace std;

namespace Ryan{


	
	int IServer::ReInitCSocket(CSocket* pCSocket)
	{
		int						dummy;
		DWORD					dwReceived;
		struct linger			li;  
		// 남은 자료 다 보내고 와라 close 

		// ---- set restsize to 0 ----
		// ---- prevent to sending packet ----

		std::cout << "***********socket closee and ***************"<<endl;
	
		{
			smartCriticalSection myCS(&pCSocket->getCS());
			pCSocket->setRestCnt(0);      //보내는걸 0개로 만든다  더이상 보내지마
		}


		// ---- close socket ----
		// ---- gracefully close ----

		li.l_onoff = 1;
		li.l_linger = 0; //몇 ms기다릴지

		// onoff 1 is 리턴하고 남은 send buffer data 버린다 비정상 종료
		// onoff1 linger not 0 이면 정상 종료 과정 gracefull shutdown 정상 종료 되기전까지 closesocket은 리턴 하지 않는다 명시된 시간후 종료
		// onoff 0 인 경우 백그라운드로 동작 한다

		//링거 옵션을 소켓에 지정한다
		if (setsockopt(pCSocket->getSocket(), SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li)) == SOCKET_ERROR)
		{
			printf("SocketConextReInit(%d) Tcp : %d\n", pCSocket->getIndex(), WSAGetLastError());
		}


		//1. cli 가 send로 종료 메세지 보냄
		//2. 서버가 종료 하라고 명령 전송
		//3. 클라가 closesocket 보내고 서버도 closesocket 보냄 ( ack , fin ) 기다리지 말고 바로 종료 시킴 linger 옵션 통해서
		//적용후 제거

		//1 세션 종료 절차 및 핸들 클로우즈 
		std::cout << "REINIT::" << pCSocket->getIndex() << endl;
		closesocket(pCSocket->getSocket());//소켓 버린다
		


		// ---- tcp buf -----      버퍼의 시작과 끝 점을 주소 가장 처음으로 움직인다
		pCSocket->reInitBuf();


		// ---- tcp socket ----   새로 만든다 소켓을 핸들을 통해
		pCSocket->setSocket();

		// ---- tcp socket accept ----
		dummy = AcceptEx(_sockListener, pCSocket->getSocket(), 
			pCSocket->getRecvEnd(),
			MAXRECVPACKETSIZE,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			&dwReceived,
			(OVERLAPPED *)&pCSocket->_eovRecvTcp);

		if (dummy == FALSE && GetLastError() != ERROR_IO_PENDING){
			std::cout << "reinit soccket error!" << endl;
			return 0;
		}


		//index 는 그대로니까 놔두자
		pCSocket->setFixedQ(DEFAULTQUEUE);
		pCSocket->setChannel(DEFAULTCHANNEL);

		std::cout << "REINIT::" << pCSocket->getIndex() <<"BACK To the SOCKET POOL"<< endl;
		return 1;
	}

	bool IServer::init(int port){
	
		WSADATA					wd = { 0 };
		SOCKADDR_IN				addr;
		HANDLE					hThread;
		int						dummy, iN;
		int						iChannelNum = 0;

		requestRMI[REQUEST_LOGIN]					 = [this](CSocket* a, char* b){return Rmi::RequestLogin(a, b, this); };
		requestRMI[REQUEST_INITIALIZE]				 = [this](CSocket* a, char* b){return Rmi::RequestInitialize(a, b, this); };
		requestRMI[REQUEST_INITIALIZE_NEXT]			 = [this](CSocket* a, char* b){return Rmi::RequestInitializeNext(a, b, this); };
		requestRMI[NOTIFY_USERLIST]					 = [this](CSocket* a, char* b){return Rmi::NotifyUserList(a, b, this); };
		requestRMI[REQUEST_CHANGE_CHANNEL]			 = [this](CSocket* a, char* b){return Rmi::RequestChangeChannel(a, b, this); };
		requestRMI[REQUEST_CHANGE_CHANNEL_NEXT]		 = [this](CSocket* a, char* b){return Rmi::RequestChangeChannelNext(a, b, this); };
		requestRMI[REQUEST_CHAT]					 = [this](CSocket* a, char* b){return Rmi::RequestChat(a, b, this); };
		requestRMI[REQUEST_LOGOUT_NEXT]				 = [this](CSocket* a, char* b){return Rmi::RequestLogoutNext(a, b, this); };
		requestRMI[REQUEST_LOGOUT]					 = [this](CSocket* a, char* b){return Rmi::RequestLogout(a, b, this); };



		dummy = WSAStartup(MAKEWORD(2, 2), &wd);
		if (dummy != 0) return 0;

		_sockListener = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (_sockListener == INVALID_SOCKET) {
			std::cout << "invalid _socketlistner error" << endl;
			return 0;
		}

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons((short)port);

		dummy = ::bind(_sockListener, (sockaddr *)&addr, sizeof(addr));

		if (dummy == SOCKET_ERROR) return 0;
		dummy = listen(_sockListener, 512);
		if (dummy == SOCKET_ERROR) return 0;


		_hIocpWorkTcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_hIocpWorkTcp == NULL) return 0;
		_hIocpAcpt = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_hIocpAcpt == NULL) return 0;

		// aceept 만을 위한 쓰레드 등록
		CreateIoCompletionPort((HANDLE)_sockListener, _hIocpAcpt, 0, 0);

		////            CSOCKET init              ////

		for (iN = 0; iN < _iMaxUserNum; iN++){
			if (!pCSocket[iN].initSocket(iN, _sockListener)){
				printf("SOCKET CLASS INIT is FAIL");
				return 0;
			}
		}
		if (pCSocket == NULL) { std::cout << "PCSOCKET IS NULL";  return 0; }



		/////////    CHANNEL init
	//	pCChannel = new CChannel[_iMaxChannel];
		
		if (pCChannel == NULL){
			std::cout << "channel ctor error"; return 0;
		}

		for (iN = 0; iN < _iMaxChannel; iN++) 
		{
			pCChannel[iN].InitChannel(iN); // 채널 번호 정해주기 클래스에 0번은 이름이 0 이런 방식
		}

		///// init LOGIC QUEUE CLASS ( multi threads queue) count = core count usually

		
		//각각의 로직 큐를 초기화 한다 그리고 이 큐를 처리를 위한 쓰레드를 만든다
		for (iN = 0; iN < _iMaxQueue; iN++){
			pCQueue[iN].InitProcess(iN);
		}

		_acceptThread = new std::thread(&IServer::AcceptProc, this, 999);

		//쓰레드 초기화는 나중에
		_workerThread = new std::thread[8];
		_logicThread = new std::thread[2];

		for (int i = 0; i < _iWorkThreadNum; i++)
			_workerThread[i] = std::thread(&IServer::WorkerTcpProc, this, i);

		for (int i = 0; i < 2; i++)
			_logicThread[i] = std::thread(&IServer::GameProc, this, i);

		
		return true;

	}
	
	
	void IServer::AcceptProc(int param){

	//	cout << "Accept param" << param << endl;
	
		DWORD					dummy, dwTransferred;
		LPEOVERLAPPED			lpEov;
		CSocket*				pCSocket;
		BOOL					bResult = true;
		sockaddr_in				*pLocal, *pRemote;
		int						localLen, remoteLen;


		// overlapped 구조체
		// internal [out] error code ex) status_pending
		// internalhigh [out] 전송된 바이트 수
		// offset [in] 32bit 하위 파일 주소
		// offset [in] 32bit 상위 파일 오프셋
		// hEvetn [in] 이벤트 핸들이나 데이터;    [in 의 경우 초기화 꼭 아니면 에러]
		//

		while (!_done)
		{

			//연결은 되더라도 데이터가 와야 첫 완료가 된다.
			//1.완료가 되면 이걸 cp = queue -> 쓰레드 깨워서  다음 동작 진행
			//데이터 와야 진행됨!
			bResult = GetQueuedCompletionStatus(_hIocpAcpt, &dwTransferred,  //몇바이트인지 
				(LPDWORD)&dummy,		//누구껀진 모름
				(LPOVERLAPPED *)&lpEov,// overapped 에 데이터 도착
				INFINITE);

			// 처음 하이를 치면  여기서부터 시작 된다

			if (lpEov != NULL) //데이터 오니 널이 아니지
			{
				// 1---- increase current user number ----
				//Server.iCurUserNum++;

				// overlapped 구조체의 주소를 소켓컨택스트의 시작주소로 바꿔준다
				// 가능한 이유는 소켓 컨택스트 구조체를 가지고 있으니까


				pCSocket = (CSocket*)lpEov;

				cout << "pCSocket::" << pCSocket;
				cout << "index used::" << pCSocket->getIndex() << endl;

				//get 1 오버랩 구조체 2 버퍼 3 원격 주소 4 소켓
				GetAcceptExSockaddrs(pCSocket->getRecvEnd(),   // pointer to a buffer  that reveives the first block of data from acceptex call same with lpoutputbuffer param of acceptex
					MAXRECVPACKETSIZE,			   //acceptex function 과 같은 값
					sizeof(sockaddr_in) + 16,      //local address information
					sizeof(sockaddr_in) + 16,      //remote address

					(sockaddr **)&pLocal,          //a pointer reveives the local address of the connection //소켓 주소가 담긴 구조체가 리턴된다.
					&localLen,
					(sockaddr **)&pRemote,		   //a pointer reveives the remote address of the connection
					&remoteLen);

				//큐를 먼저 옮긴다 데이터는 이미 왔으므로
				RecvTcpBufEnqueue(pCSocket, dwTransferred);


				// 4---- store remoteAddr ----
				// 1주소 2 값
				pCSocket->setRemoteAddr(pRemote);

				//소켓이 할당되면 iocp 새로 만들어 연결 시킨다.
				CreateIoCompletionPort((HANDLE)pCSocket->getSocket(),
					_hIocpWorkTcp,
					(DWORD)pCSocket,      // 어떤 입출력 연산이 완료되었는지 애플리케이션이 추적할 수 있다 ( 첨부된다)
					0);					  // 이것은 완료 패킷(completion packet)이 도착할 때, GetQueuedCompletionStatus 함수 호출시 반환된다.
				// CompletionKey 매개변수는 특수 목적의 완료 패킷들을 큐에 저장할 때 PostQueueCompletionStatus 함수에 의해서
				// 사용되기도 한다.

				//            if recv 없다면 인스턴스에 그 데이터가 그대로 있어!!! 그대로 있으면 이벤트가 안온다 또는 기다려진다 그러므로 
				//			  워커 쓰레드스 에서 getqueue 가 동작을 안하게되!!!!!!!!!!! 근데 꺼냈으면 이벤트를 받을수 있게 됨
		
				PostTcpRecv(pCSocket);

				// 로직에 데이터를 넘긴다 (처음은 login 패킷 데이타)
				SocketEnqueueTcp(pCSocket);
			}
		}

		printf("accept[thread]::abnormal out please check the error %d???\n", GetLastError());
		return;
	}

	void IServer::WorkerTcpProc(int param){


		DWORD					dwTransferred, dummy;
		LPEOVERLAPPED			lpEov;
		CSocket*				pCSocket, *tCSocket;
		BOOL					bResult;

		while (!_done)
		{

			// 비정상 종료 bResult 가 false dw 0바이트이다
			// 가정1 연결된 소켓에는;;; 데이터가 어떻게 될지 궁금하네
			// 소켓 끊어짐은 0 byte read 이다. 1 closesocket 또는 shutdown 함수 호출 시점
			// 호출하지 않고 종료시 0byte read는 발생하지 않는다 0byte read가 발생하지 않은 상태에서
			// read 또는 write 를 시도하면 상대는 이미 종료 되었으므로 error-netname_deleted 64 발생한다
			// hardclose 라고 한다.  또한 acceptex 호출 시 backlog 에 있던 소켓이 accept 되기전에 접속 끊어도 이 에러 발생한다
			// 소켓은 리시브에 리시브 구조체 샌드에 샌드 구조체 등록 해놓고
			// 우선 어떤 구조체가옴



			bResult = GetQueuedCompletionStatus((HANDLE)_hIocpWorkTcp,		//work 핸들러 (큐이름)
				&dwTransferred,										//보내온 데이터
				(DWORD*)&pCSocket,									//Completion Key 어떤자식인지!~
				(LPOVERLAPPED *)&lpEov,								//WSAOVERLAPPED  수신된 데이터.hEvent OS가 알아서 값 오면 갱신해준다
				INFINITE);
			


			if (bResult == FALSE)
			{
				cout << "HARD CLOSE! client didn't reply on my closesocket ack " << endl;
				cout << WSAGetLastError() << endl;
				
			}

			if (lpEov != NULL)        //잘 받았다면 OS로부터
			{
				if (dwTransferred == 0) // 종료메시지라면 shutdown();
				{
					// LOGOUT or TYPE 으로 해도 된다
					EnqueueClose(pCSocket);
					cout << "log out ***" << endl;

				}
				else
				{
					//lpEov == pCSocket->_eovRecvTcp
					if (lpEov->mode == RECVEOVTCP)
					{
						//size check_packet_valid socekt



						// ---- setting end pointer ----
						RecvTcpBufEnqueue(pCSocket, dwTransferred);

						// 로직 큐에 데이터를 넘긴다
						SocketEnqueueTcp(pCSocket);

						// ---- wait tcp recv ----
						PostTcpRecv(pCSocket);
					}

					else if (lpEov->mode == SENDEOVTCP)
					{
						//log 찍기
						cout << "TO " << pCSocket->getIndex() << " data is " << " Byte is " << dwTransferred << " send compelte" << endl;
						//만약 보냈는데 workerthread 와 logic 쓰레드간의 경쟁이 있을 경을 경우 재전송을 한다 없다면 바로 리턴
						PostTcpSendRest(pCSocket, dwTransferred);
					}
				}
			}

			//end of while
		}

		//end of Thread
	}

	//로직 쓰레드 여기에서 로직큐를 관리 한다
	void IServer::GameProc(int pram){
		
		DWORD			dwResult = 0;;
		CSocket*		pCSocket;

		char			*cpPacket;
		short			sType;
		int				iPacketSize;
		int				 id;

		cout << "logicThread::pram" << pram << endl;
		//IServer* me = (IServer*)pram;

		pCQueue[pram].setIndex(pram);

		while (!_done)
		{
			//동기화 유지 프레임수 조절
			this_thread::sleep_for(std::chrono::milliseconds(100));
		

			while (1)
			{
			
				// 내가 관리하는 로직큐에 데이터가 왔으면 꺼내온다
			//	me->pCQueue[id].SocketDequeue(&pCSocket);
				pCQueue[pram].SocketDequeue(&pCSocket);

				//가져온 주소가 널이라면 즉 버퍼 빈것이니 아웃
				if (pCSocket == FIXEDQUEUEEMPTY) break;

				//이 함수는 lpsocket 컨텍스트를 주고 cppacket과 size를 로컬 변수로 저장한다는 뜻임 동시에 디큐				
				//처리 단위로 패킷 꺼낸다.
				
				//임시 변수에 패킷을 복사 해 넣는다.
				RecvTcpBufDequeue(pCSocket, &cpPacket, &iPacketSize);

				//  패킷 종류 확인
				CopyMemory(&sType, cpPacket + sizeof(short), sizeof(short));
	
			

				requestRMI[sType](pCSocket, cpPacket);

					
			}
		}
		printf("PROCESS::gamethread:: 죽었다\n");

		return;
	}


	void IServer::RecvTcpBufEnqueue(CSocket* pCSocket, int iPacketSize)
	{

		//다음 패킷을 위해 end를 옮기는게 긑
		int						iExtra;

		/* packet is already stored */
		//end는 항상 데이터가 없는 빈공간을 가르킨다
		iExtra = pCSocket->getRecvEnd() + iPacketSize - pCSocket->getRecvRingBuf() - RINGBUFSIZE;

		//printf("RecvTcpBufEnqueue:: cpRTEnd is %d packetsize %d  cprtBegin is %d\n", pCSocket->getRecvEnd(), iPacketSize, pCSocket->getRecvRingBuf());
		//printf("RecvTcpBufEnqueue:: iExtra is %d\n", iExtra);


	//	cout << "R::pCSocket[" << pCSocket->getIndex() << "]::" << pCSocket << endl;
	//	cout << "R::AceptThread_data::" << pCSocket->getRecvEnd() << endl;
		int before = (int)pCSocket->getRecvEnd() - (int)pCSocket->getRecvRingBuf();

		if (iExtra >= 0)
		{
			CopyMemory(pCSocket->getRecvRingBuf(), pCSocket->getRecvRingBuf() + RINGBUFSIZE, iExtra);
			pCSocket->setRecvEndFromZero(iExtra); //= pCSocket->getRecvRingBuf() + iExtra;
		}
		else
		{   //그렇지 않으면 end의 포인트를 패킷 사이즈 만큼 움직여 놔라
			pCSocket->setRecvEnd(iPacketSize);

		}
	}

	//디큐는 현재 비긴의 위치를 기반으로 한다 어떤 패킷이 오던 상관 없음
	void IServer::RecvTcpBufDequeue(CSocket* pCSocket, char **cpPacket, int *iPacketSize){

		// 받은 내용을 추출하기 위한 함수
		short	iBodySize;
		int		iExtra;

		/* packet doesn't disappear immediately, so cpPacket pointer is valid */
		*cpPacket = pCSocket->getRecvBegin();//


		//패킷을 복사 해주려면 처음 시작점을 가르킨다 패킷이!
		iExtra = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pCSocket->getRecvBegin();


		// in order to get body size 
		if (iExtra < sizeof(short))
		{
			*(pCSocket->getRecvRingBuf() + RINGBUFSIZE) = *pCSocket->getRecvRingBuf();
		}

		/* get bodysize */
		CopyMemory(&iBodySize, pCSocket->getRecvBegin(), sizeof(short));


		//남겨진 부분이 작으면 데이터는 링버퍼 앞쪽에 있다는 이야기
		if (iExtra <= iBodySize + HEADERSIZE)
		{
			/* recursive at body */
			CopyMemory(pCSocket->getRecvRingBuf() + RINGBUFSIZE, pCSocket->getRecvRingBuf(),
				iBodySize + HEADERSIZE - iExtra);
			//데이터 복사해주는 이유는 이 데이터를 가지고 출력하기 위해서

			//begin 의 위치는 즉 패킷의 마지막 도착 위치
			pCSocket->setRecvBeginFromZero(iBodySize + HEADERSIZE - iExtra);
		}

		else //비긴은 4로 이동
		{
			pCSocket->setRecvBeginAdd(iBodySize + HEADERSIZE);

		}//다음 하이를 가르킨다!빈공간까지 간다라고 가정

		/* bodysize + headersize */
		*iPacketSize = iBodySize + HEADERSIZE;
	//	cout <<endl<< "RecvTcpBufDequeue::END" << endl;
	}


	 //실제적으로 recv등록 한다
	 void IServer::PostTcpRecv(CSocket* pCSocket)
	 {
		 WSABUF					wsaBuf; // 1. lengh 2 char* 어디있니~
		 DWORD					dwReceived, dwFlags;
		 int					iResult;


		 dwFlags = 0; // 차이 확인하기!
		 wsaBuf.buf = pCSocket->getRecvEnd(); //마지막 위치에 있어요
		 wsaBuf.len = MAXRECVPACKETSIZE; //최대크기

		 // 동시에  보내면  쌓아둬서 한번에 가져옴;;; 쓰레드 근데 하나가 가져온다;; 나머지 쓰레드는 놀고 있다.

		 iResult = WSARecv(pCSocket->getSocket(),//클라 소켓
			 &wsaBuf, //버퍼 주소 여기로 저장해라 이런거
			 1,// 버퍼 개수 
			 &dwReceived, // 받은 데이터 적어라
			 &dwFlags,  // 플래그 저장해라 in out? 표시겠지
			 (OVERLAPPED *)&pCSocket->_eovRecvTcp, //오버랩에 데이터가 저장될듯!!
			 NULL // 데이터 입력이 완료 되었을때 호출한 루닡의 포인터 뭐지? 콜백 함수 등록?
			 );


		 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
		 {
			 // get error;
			 printf("***** PostTcpRecv error : %d, %d\n", pCSocket->getSocket(), WSAGetLastError());
		 }
	 }

	 //소켓큐로 보낸다. 로직 큐가 처리하기 위해서
	 void IServer::SocketEnqueueTcp(CSocket* pCSocket)
	 {
		 //데이터를 파싱해서 보내는거다! 어떤 쓰레드 에게 보낼지 결정한다 (채널링)
		 //패킷의 유효성 검사 등을 한다.
		 //큐에 있는 데이터를 처리하기 위해서 처리 쓰레드르로 보내야 한다
		 //패킷은 끊어 올수도 있으므로 데이터를 우선 받아서 크기 만큼 나눠서 보내게 된다
		 // 10.5 개의 내용을 받고 0.5개의 내용을 받는 경우를 처리 하기 위한 함수이다.
		 //우선은 패킷을 각각의 의미 단위로 짤라서 데이터를 계속보낸다. 0.5가 남으면 멈추게 된다

		 short				iBodySize;
		 int				iRestSize, iExtra;
		 short test;


		 iRestSize = pCSocket->getRecvEnd() - pCSocket->getRecvMark();

		 //만약 링버퍼 초기라면 여기서 그값을 더해준다. 왜냐면 시작은 항상 데이터가 왔으므로 end가 더 크다
		 if (iRestSize < 0) iRestSize += RINGBUFSIZE;

		 //현재의 처리해야할 데이터 크기
		 //cout << "SOCKETENQUEUETCP::restsiz-------------------------------------------" << iRestSize <<endl;



		 /* packet is not valid */
		 if (iRestSize < HEADERSIZE) return;

		 /* receive operation will be wait */
		 /* check begin at cpRTMark pointer */

		 while (1)
		 {
			// cout << "SOCKETENQUEUETCP::restsize(in while) is " << iRestSize << endl;

			 //extra is 총 사이즈 거나 전체에서 패킷 만큼 크기를 뺀 만큼의 나머지
			 //extra 의 역활은 mark 를 통해 전체 데이터의 크기(헤더 2바이트)를 구하기 위하는 용도이다. 패킷이 짤려 있다면 그것을(헤더부분이)
			 // 그거를 도와주기 위해서 존재함
			 iExtra = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pCSocket->getRecvMark();

			 //0,1
			 //2바이트 보다 작다면 크기를 구하느건 2바이트 이므로 다음의 시작 즉 첫번째 값을 가져오게 된다.
			 if (iExtra < sizeof(short))
			 {
				 *(pCSocket->getRecvRingBuf() + RINGBUFSIZE) = *pCSocket->getRecvRingBuf();
			 }

			 /* get bodysize */
			 CopyMemory(&iBodySize, pCSocket->getRecvMark(), sizeof(short));
			// CopyMemory(&test, pCSocket->cpRTMark + 2, sizeof(short));

		//	 cout << "SOCKETENQUEUETCP::BODYSIZE " << iBodySize << " , total size is " << iBodySize + 4 << endl;
			 /* packet is invalid */
			 
			 // 다처리하고 패킷이 0.5개남았다면 끝난다. 
			 if (iRestSize < iBodySize + HEADERSIZE) return;


			 //패킷 단위로 그다음 패킷의 시작점으로 이동
			 pCSocket->setRecvMark(iBodySize + HEADERSIZE);// += iBodySize + HEADERSIZE;
		
			 /* recursive case */
			 // 마크가 만약 링버퍼의 맥스 그 다음에 있다면  마크를 링버퍼 처음으로 옮긴다. 데이터는 이미 복사되어있음
			if (pCSocket->getRecvMark() >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
				 pCSocket->setRecvMark(-RINGBUFSIZE);

			

			// 하나의 패킷을 처리했으니 남은 크기는 그만큼 줄어든다. 2개가 동시에 왔다면 반복문 통해 두개를 처리한다.
			 iRestSize -= iBodySize + HEADERSIZE;
		//	 cout << "packet is more than 1 is arrive and then one of them is removed and then. remain is" << endl;
		//	 cout << "SOCKETENQUEUETCP::iRestSize(already ---) " << iRestSize <<" continued in while "<< endl;

			 
			 //패킷은 의미 단위 하나로 인큐에 진행 된다.
			 //tcp에서 받은 내용을 게임버퍼 인큐로 넣어준다.
			 this->pCQueue[pCSocket->getFiexedQ()].SocketEnqueue(pCSocket);

			 /* packet is invalid */
			 //하나의 패킷보다 작다라는 경우임 0.5개 남았을때
			 if (iRestSize < HEADERSIZE) return;
		 }
	 }

	 //종료하기 위해 종료 rmi 를 호출한다  workerthread 가 호출한다!
	 void IServer::EnqueueClose(CSocket* pCSocket)
	 {
		 short					dummy;

		 //db 없으니 바로 종료
		 //종료 함수 호출 거기에서 종료 될수 있도록
		 //본인의 queue thread 를 확인해야 된다


		 dummy = (short)0;
		 CopyMemory(pCSocket->getRecvEnd(), &dummy, sizeof(short));
		 dummy = REQUEST_LOGOUT_NEXT; //requestlogout
		 //db 정리 하고 종료 시키기 위해서 이렇게 한다!

		 CopyMemory(pCSocket->getRecvEnd() + sizeof(short), &dummy, sizeof(short));

		 // 링버퍼보다 크다면 초기 위치에 재복사 해준다 끝부분만
		 dummy = pCSocket->getRecvEnd() + HEADERSIZE - pCSocket->getRecvRingBuf() - RINGBUFSIZE;
		 if (dummy >= 0)
		 {
			 CopyMemory(pCSocket->getRecvRingBuf(), pCSocket->getRecvRingBuf() + RINGBUFSIZE, dummy);
		 }

		 // 쓰레드 번호 확인해서 해당 쓰레드에 자신을 소켓에 넣어서 RMI 를 호출하게 한다.
		 this->getCQueue(pCSocket->getFiexedQ()).SocketEnqueue(pCSocket);

		// Server.ps[lpSockContext->iProcess].GameBufEnqueue(lpSockContext);
		 
	 }


	 //list 에 연결된 애들한테 보내기 같은 채널의 브로드 캐스트
	 void PostTcpSend(int iChannel, char *cpPacket, int iPacketSize, IServer* pServer)
	 {
		 CSocket*			pCSocket;
		 WSABUF				wsaBuf;
		 DWORD				dwSent;

		 int				iResult, iExtra, iSendNow;
		 int				userCount;

		 userCount = pServer->getChannel(iChannel).getUserNum();
		 auto it = pServer->getChannel(iChannel).getUser().begin();

		 //printf("BROADCAST::usercount is <%d> in same list\n",userCount);

		 if (userCount != 0){

			 //채널 인원수 만큼 보내기
			 while (it != pServer->getChannel(iChannel).getUser().end()){


				 int before = (int)(*it)->getSendEnd() - (int)(*it)->getSendRingBuf();

				 CopyMemory((*it)->getSendEnd(), cpPacket, iPacketSize);
				
				 iExtra = (*it)->getSendEnd() + iPacketSize - (*it)->getSendRingBuf() - RINGBUFSIZE;
				 if (iExtra >= 0)
				 {

					 CopyMemory((*it)->getSendRingBuf(), (*it)->getSendRingBuf() + RINGBUFSIZE, iExtra);
					 (*it)->setSendEndFromZero(iExtra);

				 }
				
				 else
				 {
					 (*it)->setSendEnd(iPacketSize);
				 }


				 {
					 smartCriticalSection myCS(&(*it)->getCS());
					 (*it)->setRestCnt(iPacketSize);
					 iSendNow = ((*it)->getRestCnt() == iPacketSize) ? TRUE : FALSE;
				}

				 //다른 처리 쓰레드가 같은 명령을 처리하지 않았다면
				 if (iSendNow)
				 {
					 wsaBuf.buf = (*it)->getSendBegin();
					 wsaBuf.len = iPacketSize;


					 iResult = WSASend((*it)->getSocket(),
						 &wsaBuf,
						 1, //버퍼 갯수
						 &dwSent,
						 0, //flag 
						 (OVERLAPPED *)&(*it)->_eovSendTcp, NULL); 
					 
					 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
					 {
						 printf("***** PostTcpSend error : %d, %d\n", (*it)->getIndex(), WSAGetLastError());
					 }

				 }		
				 //IO 쓰레드와 겹친다면 그대로 종료하고 IO쓰레드가 보내도록 해준다
				 else
				 {
					 if ((*it)->getRestCnt() > RINGBUFSIZE)
					 {
						 /* it's not disconnection case */
						 printf("*********** tcp sendbuf overflow!(%d) : %d ***********\n", (*it)->getIndex(), (*it)->getRestCnt());
					 }
				 }
				 
				 it++; //다음 유저 에게 브로드 캐스트 한다
			 }//한명에게 보내기 끝
		
		 }//end of while

		 // usercount == 0  return;
		 return;
	 }


	 //만약 1번 쓰레드에서 4번 쓰레드처럼 쓰레드가 바뀌는 경우의 명령은 새로운 쓰레드에 새로운 
	 //이벤트를 생성해서 큐에 넣어줘야 한다 예(귓속말처럼 같은 채널에 없을때)
	 //이 함수는 fixedqueue 쓰레드가 처리를 한다
	 void PostTcpSend(int iSockNum, CSocket* to, char *cpPacket, int iPacketSize)
	 {
		 CSocket*	pCSocket;
		 WSABUF		wsaBuf;
		 DWORD		dwSent;
		 int		iResult, iExtra, iSendNow;
		 int		iN;

		 iSockNum = 1; //추후 확장을 위해 만약 list 의 큰 크기의 데이터를 보낼 경우는 여러번에 나눠서 보낸다

		 for (iN = 0; iN < iSockNum; iN++)
		 {
			 pCSocket = to;
			 /* packet copy  끝지점부터 패킷을 복사 한다 인테져 사이즈 만큼*/
			 // 같은 처리 쓰레드 이다 브로드 퀘스트 더라도!
			 CopyMemory(pCSocket->getSendEnd(), cpPacket, iPacketSize);


			 /* move ringbuf end position */
			 // 샌드버퍼 끝지점 + 보낼 크기가 
			 iExtra = pCSocket->getSendEnd() + iPacketSize - pCSocket->getSendRingBuf() - RINGBUFSIZE;
			 if (iExtra >= 0)//엑스트라가 발생됨! 여분만큼만 처음 버퍼 사이즈에 복사한다! 
			 {
				 CopyMemory(pCSocket->getSendRingBuf(), pCSocket->getSendRingBuf() + RINGBUFSIZE, iExtra);
				 // pCSocket->getSendEnd() = pCSocket->getSendRingBuf() + iExtra;    //끝지점은 그리고 extra의 끝지점으로 이동
				 pCSocket->setSendEndFromZero(iExtra);

			 }

			 else
			 {  //여분이 없다면 끝지점만 이동
				 pCSocket->setSendEnd(iPacketSize);
			 }

			 // 두 쓰레드가 동시에 보낼 명령이 겹칠 수 있으므로 
			 // 두 쓰레드중 한쓰레드는 정지하게된다 만약 2번 쓰레드가 여기까지 왔을경우
				 {
					 smartCriticalSection myCS(&to->getCS());
					 to->setRestCnt(iPacketSize);       // 패킷 사이즈만큼 += 해준다
					 iSendNow = (to->getRestCnt() == iPacketSize) ? TRUE : FALSE;
				 }

			 if (iSendNow)
			 {
				 wsaBuf.buf = pCSocket->getSendBegin();
				 wsaBuf.len = iPacketSize;

				 //등록 한다 결과는 device 에서 온다.
				 iResult = WSASend(pCSocket->getSocket(),
					 &wsaBuf,
					 1, //버퍼 갯수
					 &dwSent,
					 0, //flag 
					 (OVERLAPPED *)&pCSocket->_eovSendTcp, NULL);

				 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)

				 {
					 printf("***** PostTcpSend error : %d, %d\n", pCSocket->getIndex(), WSAGetLastError());
				 }
			 }
			
			 //큐 정리 되기 전에 다른 쓰레드가 또 요청을 먼저 한 경우
			 //보내지 않고 그대로 종료
			 else
			 {
				 //만약 링버퍼보다 큰경우는 오버플로우
				 if (pCSocket->getRestCnt() > RINGBUFSIZE)
				 {
					 //버퍼 사이즈 조절해줘야 한다 
					 printf("*********** tcp sendbuf overflow!(%d) : %d ***********\n", pCSocket->getIndex(), pCSocket->getRestCnt());
				 }
			 }
		 }
	 }

	 //보내고 난후면 cpSTPbegin 을 엔드포인트로 옮겨주면 끄읕 과 아까 못보내준 패킷 처리하기! rest에 남아있는 것들
	 void PostTcpSendRest(CSocket* to, int iTransferred)
	 {
		 WSABUF					wsaBuf;
		 DWORD					dwSent;
		 int					iResult, iRestSize, iExtra;

		 //전송 했다면 디큐 해줘라 즉 begin-> end 점까지 옮겨주기
		 /* dequeue completion size */
		 iExtra = to->getSendBegin() + iTransferred - to->getSendRingBuf() - RINGBUFSIZE;
		 if (iExtra >= 0)
		 {
			// to->setSendEndFromZero(iExtra);
			 to->setSendBeginFromZero(iExtra);

		 }

		 else
		 {
			 to->setSendBegin(iTransferred); //__cSendTcpBegin 의 위치 를 옮겨준다
		 }

		 //보내려는 놈하고 정리하려는 놈! 보냈는데 보내기를 또하려고 함
		 //정리는 그 후에 되니까

		 /* set rest size, and get "is rest exist" */
		 {
			 smartCriticalSection myCS(&to->getCS());
			 to->setRestCnt(-iTransferred);                // 보애냐할 카운터 숫자 줄인다
			 iRestSize = to->getRestCnt();                 // 정리되기 전에 어떤 쓰레드가 먼저 보내기를 또 요청했을 경우 남은 데이터를 보내야한다.
		 }

		 if (iRestSize == 0)
		 {
			// cout << "postSEND::sequence compelete" << endl;
		 }

		 else if (iRestSize > 0) //보내고 난후 어떤놈이 보내기를 등록 또해놔서 순서가 밀렸을떄! 또는 모아서 보내질 경우도 고려 
		 {
		//	 cout << "REST DATA is " << iRestSize << "try to send again" << endl;
			/*
			 *  if iRestSize < 0, socketcontext is reinitialized
			 */

			 //이건 어차피 연결된 소켓에게만 간다 만약 N개의 쓰레드에서 동시에 대량으로 보내게 될 경우!
			 //미친만큼 보낸다면 조정을해라

			 if (iRestSize > MAXSENDPACKETSIZE) iRestSize = MAXSENDPACKETSIZE;

			 //8 남은 버퍼의 최대 사이즈
			 iExtra = to->getSendRingBuf() + RINGBUFSIZE - to->getSendBegin();

			 //남은 공간보다 8 < 보내야할 크기가 더크다면 뭔가 에러난다.
			 if (iExtra < iRestSize)
			 {
				 //send에서 추가는 링버퍼의 처음으로 돌아가므로 그 값을 버퍼 끝에 복사를 한다!
				 //그림으로 그리면 여분은 원래 안쓰고 처음부분에 데이터 저장되있는데 그걸 여분쪽으로 데이터 복사해서 한번에 보낸다!
				 CopyMemory(to->getSendRingBuf() + RINGBUFSIZE, to->getSendRingBuf(),
					 iRestSize - iExtra);
			 }

			 /* send packet */
			 wsaBuf.buf = to->getSendBegin();
			 wsaBuf.len = iRestSize;

			 iResult = WSASend(to->getSocket(),
				 &wsaBuf, 
				 1, //버퍼 갯수
				 &dwSent,
				 0, //flag
				 (OVERLAPPED *)&to->_eovSendTcp, NULL);

			 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
			 {
				 printf("***** PostTcpSendRest error : %d, %d\n", to->getIndex(), WSAGetLastError());
			 }
		 }
	 }




}
