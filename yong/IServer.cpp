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
		// ���� �ڷ� �� ������ �Ͷ� close 

		// ---- set restsize to 0 ----
		// ---- prevent to sending packet ----

		std::cout << "***********socket closee and ***************"<<endl;
	
		{
			smartCriticalSection myCS(&pCSocket->getCS());
			pCSocket->setRestCnt(0);      //�����°� 0���� �����  ���̻� ��������
		}


		// ---- close socket ----
		// ---- gracefully close ----

		li.l_onoff = 1;
		li.l_linger = 0; //�� ms��ٸ���

		// onoff 1 is �����ϰ� ���� send buffer data ������ ������ ����
		// onoff1 linger not 0 �̸� ���� ���� ���� gracefull shutdown ���� ���� �Ǳ������� closesocket�� ���� ���� �ʴ´� ��õ� �ð��� ����
		// onoff 0 �� ��� ��׶���� ���� �Ѵ�

		//���� �ɼ��� ���Ͽ� �����Ѵ�
		if (setsockopt(pCSocket->getSocket(), SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li)) == SOCKET_ERROR)
		{
			printf("SocketConextReInit(%d) Tcp : %d\n", pCSocket->getIndex(), WSAGetLastError());
		}


		//1. cli �� send�� ���� �޼��� ����
		//2. ������ ���� �϶�� ��� ����
		//3. Ŭ�� closesocket ������ ������ closesocket ���� ( ack , fin ) ��ٸ��� ���� �ٷ� ���� ��Ŵ linger �ɼ� ���ؼ�
		//������ ����

		//1 ���� ���� ���� �� �ڵ� Ŭ�ο��� 
		std::cout << "REINIT::" << pCSocket->getIndex() << endl;
		closesocket(pCSocket->getSocket());//���� ������
		


		// ---- tcp buf -----      ������ ���۰� �� ���� �ּ� ���� ó������ �����δ�
		pCSocket->reInitBuf();


		// ---- tcp socket ----   ���� ����� ������ �ڵ��� ����
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


		//index �� �״�δϱ� ������
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

		// aceept ���� ���� ������ ���
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
			pCChannel[iN].InitChannel(iN); // ä�� ��ȣ �����ֱ� Ŭ������ 0���� �̸��� 0 �̷� ���
		}

		///// init LOGIC QUEUE CLASS ( multi threads queue) count = core count usually

		
		//������ ���� ť�� �ʱ�ȭ �Ѵ� �׸��� �� ť�� ó���� ���� �����带 �����
		for (iN = 0; iN < _iMaxQueue; iN++){
			pCQueue[iN].InitProcess(iN);
		}

		_acceptThread = new std::thread(&IServer::AcceptProc, this, 999);

		//������ �ʱ�ȭ�� ���߿�
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


		// overlapped ����ü
		// internal [out] error code ex) status_pending
		// internalhigh [out] ���۵� ����Ʈ ��
		// offset [in] 32bit ���� ���� �ּ�
		// offset [in] 32bit ���� ���� ������
		// hEvetn [in] �̺�Ʈ �ڵ��̳� ������;    [in �� ��� �ʱ�ȭ �� �ƴϸ� ����]
		//

		while (!_done)
		{

			//������ �Ǵ��� �����Ͱ� �;� ù �Ϸᰡ �ȴ�.
			//1.�Ϸᰡ �Ǹ� �̰� cp = queue -> ������ ������  ���� ���� ����
			//������ �;� �����!
			bResult = GetQueuedCompletionStatus(_hIocpAcpt, &dwTransferred,  //�����Ʈ���� 
				(LPDWORD)&dummy,		//�������� ��
				(LPOVERLAPPED *)&lpEov,// overapped �� ������ ����
				INFINITE);

			// ó�� ���̸� ġ��  ���⼭���� ���� �ȴ�

			if (lpEov != NULL) //������ ���� ���� �ƴ���
			{
				// 1---- increase current user number ----
				//Server.iCurUserNum++;

				// overlapped ����ü�� �ּҸ� �������ý�Ʈ�� �����ּҷ� �ٲ��ش�
				// ������ ������ ���� ���ý�Ʈ ����ü�� ������ �����ϱ�


				pCSocket = (CSocket*)lpEov;

				cout << "pCSocket::" << pCSocket;
				cout << "index used::" << pCSocket->getIndex() << endl;

				//get 1 ������ ����ü 2 ���� 3 ���� �ּ� 4 ����
				GetAcceptExSockaddrs(pCSocket->getRecvEnd(),   // pointer to a buffer  that reveives the first block of data from acceptex call same with lpoutputbuffer param of acceptex
					MAXRECVPACKETSIZE,			   //acceptex function �� ���� ��
					sizeof(sockaddr_in) + 16,      //local address information
					sizeof(sockaddr_in) + 16,      //remote address

					(sockaddr **)&pLocal,          //a pointer reveives the local address of the connection //���� �ּҰ� ��� ����ü�� ���ϵȴ�.
					&localLen,
					(sockaddr **)&pRemote,		   //a pointer reveives the remote address of the connection
					&remoteLen);

				//ť�� ���� �ű�� �����ʹ� �̹� �����Ƿ�
				RecvTcpBufEnqueue(pCSocket, dwTransferred);


				// 4---- store remoteAddr ----
				// 1�ּ� 2 ��
				pCSocket->setRemoteAddr(pRemote);

				//������ �Ҵ�Ǹ� iocp ���� ����� ���� ��Ų��.
				CreateIoCompletionPort((HANDLE)pCSocket->getSocket(),
					_hIocpWorkTcp,
					(DWORD)pCSocket,      // � ����� ������ �Ϸ�Ǿ����� ���ø����̼��� ������ �� �ִ� ( ÷�εȴ�)
					0);					  // �̰��� �Ϸ� ��Ŷ(completion packet)�� ������ ��, GetQueuedCompletionStatus �Լ� ȣ��� ��ȯ�ȴ�.
				// CompletionKey �Ű������� Ư�� ������ �Ϸ� ��Ŷ���� ť�� ������ �� PostQueueCompletionStatus �Լ��� ���ؼ�
				// ���Ǳ⵵ �Ѵ�.

				//            if recv ���ٸ� �ν��Ͻ��� �� �����Ͱ� �״�� �־�!!! �״�� ������ �̺�Ʈ�� �ȿ´� �Ǵ� ��ٷ����� �׷��Ƿ� 
				//			  ��Ŀ �����彺 ���� getqueue �� ������ ���ϰԵ�!!!!!!!!!!! �ٵ� �������� �̺�Ʈ�� ������ �ְ� ��
		
				PostTcpRecv(pCSocket);

				// ������ �����͸� �ѱ�� (ó���� login ��Ŷ ����Ÿ)
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

			// ������ ���� bResult �� false dw 0����Ʈ�̴�
			// ����1 ����� ���Ͽ���;;; �����Ͱ� ��� ���� �ñ��ϳ�
			// ���� �������� 0 byte read �̴�. 1 closesocket �Ǵ� shutdown �Լ� ȣ�� ����
			// ȣ������ �ʰ� ����� 0byte read�� �߻����� �ʴ´� 0byte read�� �߻����� ���� ���¿���
			// read �Ǵ� write �� �õ��ϸ� ���� �̹� ���� �Ǿ����Ƿ� error-netname_deleted 64 �߻��Ѵ�
			// hardclose ��� �Ѵ�.  ���� acceptex ȣ�� �� backlog �� �ִ� ������ accept �Ǳ����� ���� ��� �� ���� �߻��Ѵ�
			// ������ ���ú꿡 ���ú� ����ü ���忡 ���� ����ü ��� �س���
			// �켱 � ����ü����



			bResult = GetQueuedCompletionStatus((HANDLE)_hIocpWorkTcp,		//work �ڵ鷯 (ť�̸�)
				&dwTransferred,										//������ ������
				(DWORD*)&pCSocket,									//Completion Key ��ڽ�����!~
				(LPOVERLAPPED *)&lpEov,								//WSAOVERLAPPED  ���ŵ� ������.hEvent OS�� �˾Ƽ� �� ���� �������ش�
				INFINITE);
			


			if (bResult == FALSE)
			{
				cout << "HARD CLOSE! client didn't reply on my closesocket ack " << endl;
				cout << WSAGetLastError() << endl;
				
			}

			if (lpEov != NULL)        //�� �޾Ҵٸ� OS�κ���
			{
				if (dwTransferred == 0) // ����޽������ shutdown();
				{
					// LOGOUT or TYPE ���� �ص� �ȴ�
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

						// ���� ť�� �����͸� �ѱ��
						SocketEnqueueTcp(pCSocket);

						// ---- wait tcp recv ----
						PostTcpRecv(pCSocket);
					}

					else if (lpEov->mode == SENDEOVTCP)
					{
						//log ���
						cout << "TO " << pCSocket->getIndex() << " data is " << " Byte is " << dwTransferred << " send compelte" << endl;
						//���� ���´µ� workerthread �� logic �����尣�� ������ ���� ���� ��� �������� �Ѵ� ���ٸ� �ٷ� ����
						PostTcpSendRest(pCSocket, dwTransferred);
					}
				}
			}

			//end of while
		}

		//end of Thread
	}

	//���� ������ ���⿡�� ����ť�� ���� �Ѵ�
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
			//����ȭ ���� �����Ӽ� ����
			this_thread::sleep_for(std::chrono::milliseconds(100));
		

			while (1)
			{
			
				// ���� �����ϴ� ����ť�� �����Ͱ� ������ �����´�
			//	me->pCQueue[id].SocketDequeue(&pCSocket);
				pCQueue[pram].SocketDequeue(&pCSocket);

				//������ �ּҰ� ���̶�� �� ���� ����̴� �ƿ�
				if (pCSocket == FIXEDQUEUEEMPTY) break;

				//�� �Լ��� lpsocket ���ؽ�Ʈ�� �ְ� cppacket�� size�� ���� ������ �����Ѵٴ� ���� ���ÿ� ��ť				
				//ó�� ������ ��Ŷ ������.
				
				//�ӽ� ������ ��Ŷ�� ���� �� �ִ´�.
				RecvTcpBufDequeue(pCSocket, &cpPacket, &iPacketSize);

				//  ��Ŷ ���� Ȯ��
				CopyMemory(&sType, cpPacket + sizeof(short), sizeof(short));
	
			

				requestRMI[sType](pCSocket, cpPacket);

					
			}
		}
		printf("PROCESS::gamethread:: �׾���\n");

		return;
	}


	void IServer::RecvTcpBufEnqueue(CSocket* pCSocket, int iPacketSize)
	{

		//���� ��Ŷ�� ���� end�� �ű�°� �P
		int						iExtra;

		/* packet is already stored */
		//end�� �׻� �����Ͱ� ���� ������� ����Ų��
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
		{   //�׷��� ������ end�� ����Ʈ�� ��Ŷ ������ ��ŭ ������ ����
			pCSocket->setRecvEnd(iPacketSize);

		}
	}

	//��ť�� ���� ����� ��ġ�� ������� �Ѵ� � ��Ŷ�� ���� ��� ����
	void IServer::RecvTcpBufDequeue(CSocket* pCSocket, char **cpPacket, int *iPacketSize){

		// ���� ������ �����ϱ� ���� �Լ�
		short	iBodySize;
		int		iExtra;

		/* packet doesn't disappear immediately, so cpPacket pointer is valid */
		*cpPacket = pCSocket->getRecvBegin();//


		//��Ŷ�� ���� ���ַ��� ó�� �������� ����Ų�� ��Ŷ��!
		iExtra = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pCSocket->getRecvBegin();


		// in order to get body size 
		if (iExtra < sizeof(short))
		{
			*(pCSocket->getRecvRingBuf() + RINGBUFSIZE) = *pCSocket->getRecvRingBuf();
		}

		/* get bodysize */
		CopyMemory(&iBodySize, pCSocket->getRecvBegin(), sizeof(short));


		//������ �κ��� ������ �����ʹ� ������ ���ʿ� �ִٴ� �̾߱�
		if (iExtra <= iBodySize + HEADERSIZE)
		{
			/* recursive at body */
			CopyMemory(pCSocket->getRecvRingBuf() + RINGBUFSIZE, pCSocket->getRecvRingBuf(),
				iBodySize + HEADERSIZE - iExtra);
			//������ �������ִ� ������ �� �����͸� ������ ����ϱ� ���ؼ�

			//begin �� ��ġ�� �� ��Ŷ�� ������ ���� ��ġ
			pCSocket->setRecvBeginFromZero(iBodySize + HEADERSIZE - iExtra);
		}

		else //����� 4�� �̵�
		{
			pCSocket->setRecvBeginAdd(iBodySize + HEADERSIZE);

		}//���� ���̸� ����Ų��!��������� ���ٶ�� ����

		/* bodysize + headersize */
		*iPacketSize = iBodySize + HEADERSIZE;
	//	cout <<endl<< "RecvTcpBufDequeue::END" << endl;
	}


	 //���������� recv��� �Ѵ�
	 void IServer::PostTcpRecv(CSocket* pCSocket)
	 {
		 WSABUF					wsaBuf; // 1. lengh 2 char* ����ִ�~
		 DWORD					dwReceived, dwFlags;
		 int					iResult;


		 dwFlags = 0; // ���� Ȯ���ϱ�!
		 wsaBuf.buf = pCSocket->getRecvEnd(); //������ ��ġ�� �־��
		 wsaBuf.len = MAXRECVPACKETSIZE; //�ִ�ũ��

		 // ���ÿ�  ������  �׾Ƶּ� �ѹ��� ������;;; ������ �ٵ� �ϳ��� �����´�;; ������ ������� ��� �ִ�.

		 iResult = WSARecv(pCSocket->getSocket(),//Ŭ�� ����
			 &wsaBuf, //���� �ּ� ����� �����ض� �̷���
			 1,// ���� ���� 
			 &dwReceived, // ���� ������ �����
			 &dwFlags,  // �÷��� �����ض� in out? ǥ�ð���
			 (OVERLAPPED *)&pCSocket->_eovRecvTcp, //�������� �����Ͱ� ����ɵ�!!
			 NULL // ������ �Է��� �Ϸ� �Ǿ����� ȣ���� �爡�� ������ ����? �ݹ� �Լ� ���?
			 );


		 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
		 {
			 // get error;
			 printf("***** PostTcpRecv error : %d, %d\n", pCSocket->getSocket(), WSAGetLastError());
		 }
	 }

	 //����ť�� ������. ���� ť�� ó���ϱ� ���ؼ�
	 void IServer::SocketEnqueueTcp(CSocket* pCSocket)
	 {
		 //�����͸� �Ľ��ؼ� �����°Ŵ�! � ������ ���� ������ �����Ѵ� (ä�θ�)
		 //��Ŷ�� ��ȿ�� �˻� ���� �Ѵ�.
		 //ť�� �ִ� �����͸� ó���ϱ� ���ؼ� ó�� �����帣�� ������ �Ѵ�
		 //��Ŷ�� ���� �ü��� �����Ƿ� �����͸� �켱 �޾Ƽ� ũ�� ��ŭ ������ ������ �ȴ�
		 // 10.5 ���� ������ �ް� 0.5���� ������ �޴� ��츦 ó�� �ϱ� ���� �Լ��̴�.
		 //�켱�� ��Ŷ�� ������ �ǹ� ������ ©�� �����͸� ��Ӻ�����. 0.5�� ������ ���߰� �ȴ�

		 short				iBodySize;
		 int				iRestSize, iExtra;
		 short test;


		 iRestSize = pCSocket->getRecvEnd() - pCSocket->getRecvMark();

		 //���� ������ �ʱ��� ���⼭ �װ��� �����ش�. �ֳĸ� ������ �׻� �����Ͱ� �����Ƿ� end�� �� ũ��
		 if (iRestSize < 0) iRestSize += RINGBUFSIZE;

		 //������ ó���ؾ��� ������ ũ��
		 //cout << "SOCKETENQUEUETCP::restsiz-------------------------------------------" << iRestSize <<endl;



		 /* packet is not valid */
		 if (iRestSize < HEADERSIZE) return;

		 /* receive operation will be wait */
		 /* check begin at cpRTMark pointer */

		 while (1)
		 {
			// cout << "SOCKETENQUEUETCP::restsize(in while) is " << iRestSize << endl;

			 //extra is �� ������ �ų� ��ü���� ��Ŷ ��ŭ ũ�⸦ �� ��ŭ�� ������
			 //extra �� ��Ȱ�� mark �� ���� ��ü �������� ũ��(��� 2����Ʈ)�� ���ϱ� ���ϴ� �뵵�̴�. ��Ŷ�� ©�� �ִٸ� �װ���(����κ���)
			 // �װŸ� �����ֱ� ���ؼ� ������
			 iExtra = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pCSocket->getRecvMark();

			 //0,1
			 //2����Ʈ ���� �۴ٸ� ũ�⸦ ���ϴ��� 2����Ʈ �̹Ƿ� ������ ���� �� ù��° ���� �������� �ȴ�.
			 if (iExtra < sizeof(short))
			 {
				 *(pCSocket->getRecvRingBuf() + RINGBUFSIZE) = *pCSocket->getRecvRingBuf();
			 }

			 /* get bodysize */
			 CopyMemory(&iBodySize, pCSocket->getRecvMark(), sizeof(short));
			// CopyMemory(&test, pCSocket->cpRTMark + 2, sizeof(short));

		//	 cout << "SOCKETENQUEUETCP::BODYSIZE " << iBodySize << " , total size is " << iBodySize + 4 << endl;
			 /* packet is invalid */
			 
			 // ��ó���ϰ� ��Ŷ�� 0.5�����Ҵٸ� ������. 
			 if (iRestSize < iBodySize + HEADERSIZE) return;


			 //��Ŷ ������ �״��� ��Ŷ�� ���������� �̵�
			 pCSocket->setRecvMark(iBodySize + HEADERSIZE);// += iBodySize + HEADERSIZE;
		
			 /* recursive case */
			 // ��ũ�� ���� �������� �ƽ� �� ������ �ִٸ�  ��ũ�� ������ ó������ �ű��. �����ʹ� �̹� ����Ǿ�����
			if (pCSocket->getRecvMark() >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
				 pCSocket->setRecvMark(-RINGBUFSIZE);

			

			// �ϳ��� ��Ŷ�� ó�������� ���� ũ��� �׸�ŭ �پ���. 2���� ���ÿ� �Դٸ� �ݺ��� ���� �ΰ��� ó���Ѵ�.
			 iRestSize -= iBodySize + HEADERSIZE;
		//	 cout << "packet is more than 1 is arrive and then one of them is removed and then. remain is" << endl;
		//	 cout << "SOCKETENQUEUETCP::iRestSize(already ---) " << iRestSize <<" continued in while "<< endl;

			 
			 //��Ŷ�� �ǹ� ���� �ϳ��� ��ť�� ���� �ȴ�.
			 //tcp���� ���� ������ ���ӹ��� ��ť�� �־��ش�.
			 this->pCQueue[pCSocket->getFiexedQ()].SocketEnqueue(pCSocket);

			 /* packet is invalid */
			 //�ϳ��� ��Ŷ���� �۴ٶ�� ����� 0.5�� ��������
			 if (iRestSize < HEADERSIZE) return;
		 }
	 }

	 //�����ϱ� ���� ���� rmi �� ȣ���Ѵ�  workerthread �� ȣ���Ѵ�!
	 void IServer::EnqueueClose(CSocket* pCSocket)
	 {
		 short					dummy;

		 //db ������ �ٷ� ����
		 //���� �Լ� ȣ�� �ű⿡�� ���� �ɼ� �ֵ���
		 //������ queue thread �� Ȯ���ؾ� �ȴ�


		 dummy = (short)0;
		 CopyMemory(pCSocket->getRecvEnd(), &dummy, sizeof(short));
		 dummy = REQUEST_LOGOUT_NEXT; //requestlogout
		 //db ���� �ϰ� ���� ��Ű�� ���ؼ� �̷��� �Ѵ�!

		 CopyMemory(pCSocket->getRecvEnd() + sizeof(short), &dummy, sizeof(short));

		 // �����ۺ��� ũ�ٸ� �ʱ� ��ġ�� �纹�� ���ش� ���κи�
		 dummy = pCSocket->getRecvEnd() + HEADERSIZE - pCSocket->getRecvRingBuf() - RINGBUFSIZE;
		 if (dummy >= 0)
		 {
			 CopyMemory(pCSocket->getRecvRingBuf(), pCSocket->getRecvRingBuf() + RINGBUFSIZE, dummy);
		 }

		 // ������ ��ȣ Ȯ���ؼ� �ش� �����忡 �ڽ��� ���Ͽ� �־ RMI �� ȣ���ϰ� �Ѵ�.
		 this->getCQueue(pCSocket->getFiexedQ()).SocketEnqueue(pCSocket);

		// Server.ps[lpSockContext->iProcess].GameBufEnqueue(lpSockContext);
		 
	 }


	 //list �� ����� �ֵ����� ������ ���� ä���� ��ε� ĳ��Ʈ
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

			 //ä�� �ο��� ��ŭ ������
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

				 //�ٸ� ó�� �����尡 ���� ����� ó������ �ʾҴٸ�
				 if (iSendNow)
				 {
					 wsaBuf.buf = (*it)->getSendBegin();
					 wsaBuf.len = iPacketSize;


					 iResult = WSASend((*it)->getSocket(),
						 &wsaBuf,
						 1, //���� ����
						 &dwSent,
						 0, //flag 
						 (OVERLAPPED *)&(*it)->_eovSendTcp, NULL); 
					 
					 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
					 {
						 printf("***** PostTcpSend error : %d, %d\n", (*it)->getIndex(), WSAGetLastError());
					 }

				 }		
				 //IO ������� ��ģ�ٸ� �״�� �����ϰ� IO�����尡 �������� ���ش�
				 else
				 {
					 if ((*it)->getRestCnt() > RINGBUFSIZE)
					 {
						 /* it's not disconnection case */
						 printf("*********** tcp sendbuf overflow!(%d) : %d ***********\n", (*it)->getIndex(), (*it)->getRestCnt());
					 }
				 }
				 
				 it++; //���� ���� ���� ��ε� ĳ��Ʈ �Ѵ�
			 }//�Ѹ��� ������ ��
		
		 }//end of while

		 // usercount == 0  return;
		 return;
	 }


	 //���� 1�� �����忡�� 4�� ������ó�� �����尡 �ٲ�� ����� ����� ���ο� �����忡 ���ο� 
	 //�̺�Ʈ�� �����ؼ� ť�� �־���� �Ѵ� ��(�ӼӸ�ó�� ���� ä�ο� ������)
	 //�� �Լ��� fixedqueue �����尡 ó���� �Ѵ�
	 void PostTcpSend(int iSockNum, CSocket* to, char *cpPacket, int iPacketSize)
	 {
		 CSocket*	pCSocket;
		 WSABUF		wsaBuf;
		 DWORD		dwSent;
		 int		iResult, iExtra, iSendNow;
		 int		iN;

		 iSockNum = 1; //���� Ȯ���� ���� ���� list �� ū ũ���� �����͸� ���� ���� �������� ������ ������

		 for (iN = 0; iN < iSockNum; iN++)
		 {
			 pCSocket = to;
			 /* packet copy  ���������� ��Ŷ�� ���� �Ѵ� ������ ������ ��ŭ*/
			 // ���� ó�� ������ �̴� ��ε� ����Ʈ ����!
			 CopyMemory(pCSocket->getSendEnd(), cpPacket, iPacketSize);


			 /* move ringbuf end position */
			 // ������� ������ + ���� ũ�Ⱑ 
			 iExtra = pCSocket->getSendEnd() + iPacketSize - pCSocket->getSendRingBuf() - RINGBUFSIZE;
			 if (iExtra >= 0)//����Ʈ�� �߻���! ���и�ŭ�� ó�� ���� ����� �����Ѵ�! 
			 {
				 CopyMemory(pCSocket->getSendRingBuf(), pCSocket->getSendRingBuf() + RINGBUFSIZE, iExtra);
				 // pCSocket->getSendEnd() = pCSocket->getSendRingBuf() + iExtra;    //�������� �׸��� extra�� ���������� �̵�
				 pCSocket->setSendEndFromZero(iExtra);

			 }

			 else
			 {  //������ ���ٸ� �������� �̵�
				 pCSocket->setSendEnd(iPacketSize);
			 }

			 // �� �����尡 ���ÿ� ���� ����� ��ĥ �� �����Ƿ� 
			 // �� �������� �Ѿ������ �����ϰԵȴ� ���� 2�� �����尡 ������� �������
				 {
					 smartCriticalSection myCS(&to->getCS());
					 to->setRestCnt(iPacketSize);       // ��Ŷ �����ŭ += ���ش�
					 iSendNow = (to->getRestCnt() == iPacketSize) ? TRUE : FALSE;
				 }

			 if (iSendNow)
			 {
				 wsaBuf.buf = pCSocket->getSendBegin();
				 wsaBuf.len = iPacketSize;

				 //��� �Ѵ� ����� device ���� �´�.
				 iResult = WSASend(pCSocket->getSocket(),
					 &wsaBuf,
					 1, //���� ����
					 &dwSent,
					 0, //flag 
					 (OVERLAPPED *)&pCSocket->_eovSendTcp, NULL);

				 if (iResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)

				 {
					 printf("***** PostTcpSend error : %d, %d\n", pCSocket->getIndex(), WSAGetLastError());
				 }
			 }
			
			 //ť ���� �Ǳ� ���� �ٸ� �����尡 �� ��û�� ���� �� ���
			 //������ �ʰ� �״�� ����
			 else
			 {
				 //���� �����ۺ��� ū���� �����÷ο�
				 if (pCSocket->getRestCnt() > RINGBUFSIZE)
				 {
					 //���� ������ ��������� �Ѵ� 
					 printf("*********** tcp sendbuf overflow!(%d) : %d ***********\n", pCSocket->getIndex(), pCSocket->getRestCnt());
				 }
			 }
		 }
	 }

	 //������ ���ĸ� cpSTPbegin �� ��������Ʈ�� �Ű��ָ� ���� �� �Ʊ� �������� ��Ŷ ó���ϱ�! rest�� �����ִ� �͵�
	 void PostTcpSendRest(CSocket* to, int iTransferred)
	 {
		 WSABUF					wsaBuf;
		 DWORD					dwSent;
		 int					iResult, iRestSize, iExtra;

		 //���� �ߴٸ� ��ť ����� �� begin-> end ������ �Ű��ֱ�
		 /* dequeue completion size */
		 iExtra = to->getSendBegin() + iTransferred - to->getSendRingBuf() - RINGBUFSIZE;
		 if (iExtra >= 0)
		 {
			// to->setSendEndFromZero(iExtra);
			 to->setSendBeginFromZero(iExtra);

		 }

		 else
		 {
			 to->setSendBegin(iTransferred); //__cSendTcpBegin �� ��ġ �� �Ű��ش�
		 }

		 //�������� ���ϰ� �����Ϸ��� ��! ���´µ� �����⸦ ���Ϸ��� ��
		 //������ �� �Ŀ� �Ǵϱ�

		 /* set rest size, and get "is rest exist" */
		 {
			 smartCriticalSection myCS(&to->getCS());
			 to->setRestCnt(-iTransferred);                // ���ֳ��� ī���� ���� ���δ�
			 iRestSize = to->getRestCnt();                 // �����Ǳ� ���� � �����尡 ���� �����⸦ �� ��û���� ��� ���� �����͸� �������Ѵ�.
		 }

		 if (iRestSize == 0)
		 {
			// cout << "postSEND::sequence compelete" << endl;
		 }

		 else if (iRestSize > 0) //������ ���� ����� �����⸦ ��� ���س��� ������ �з�����! �Ǵ� ��Ƽ� ������ ��쵵 ��� 
		 {
		//	 cout << "REST DATA is " << iRestSize << "try to send again" << endl;
			/*
			 *  if iRestSize < 0, socketcontext is reinitialized
			 */

			 //�̰� ������ ����� ���Ͽ��Ը� ���� ���� N���� �����忡�� ���ÿ� �뷮���� ������ �� ���!
			 //��ģ��ŭ �����ٸ� �������ض�

			 if (iRestSize > MAXSENDPACKETSIZE) iRestSize = MAXSENDPACKETSIZE;

			 //8 ���� ������ �ִ� ������
			 iExtra = to->getSendRingBuf() + RINGBUFSIZE - to->getSendBegin();

			 //���� �������� 8 < �������� ũ�Ⱑ ��ũ�ٸ� ���� ��������.
			 if (iExtra < iRestSize)
			 {
				 //send���� �߰��� �������� ó������ ���ư��Ƿ� �� ���� ���� ���� ���縦 �Ѵ�!
				 //�׸����� �׸��� ������ ���� �Ⱦ��� ó���κп� ������ ������ִµ� �װ� ���������� ������ �����ؼ� �ѹ��� ������!
				 CopyMemory(to->getSendRingBuf() + RINGBUFSIZE, to->getSendRingBuf(),
					 iRestSize - iExtra);
			 }

			 /* send packet */
			 wsaBuf.buf = to->getSendBegin();
			 wsaBuf.len = iRestSize;

			 iResult = WSASend(to->getSocket(),
				 &wsaBuf, 
				 1, //���� ����
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
