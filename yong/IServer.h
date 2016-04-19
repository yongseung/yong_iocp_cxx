#ifndef _CSERVER_H_
#define _CSERVER_H_


#include "CSocket.h"
#include "CFiexedSizeQueue.h"
#include "CRmi.h"
#include "CChannel.h"
#include "protocol.h"
#include <atomic>
#include <thread>
#include <vector>
#include <future>


namespace Ryan{

	// broadcast, WSARECV 
	void PostTcpSend(int iSockNum, CSocket* to, char *cpPacket, int iPacketSize);      
	void PostTcpSendRest(CSocket* to, int iTransferred);       // worker & logic 서로 보내는 경우를 위해
	void PostTcpSend(int iBegin, char *cpPacket, int iPacketSize,IServer* pServer);


	class IServer
	{
	public:

		IServer() : _iMaxUserNum(8000), _iMaxQueue(2), _iMaxChannelInProcess(2), _iMaxChannel(_iMaxQueue *_iMaxChannelInProcess),
			pCSocket(new CSocket[_iMaxUserNum]), pCChannel(new CChannel[_iMaxChannel]), pCQueue(new CFiexedSizeQueue[_iMaxQueue]),
			_done(false), _iWorkThreadNum(GetWorkTNUM())
		{
		
		};

		int GetWorkTNUM(){
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			return systemInfo.dwNumberOfProcessors * 2;
		}

		~IServer(){ 
			_done = true;
			//동적으로 생성한거 다 지우자 어차피 서버 종료면 끝이다 프로그램
			//몇몇 쓰레드 남아있네
			delete[] pCSocket;
			delete[] pCChannel;
			delete[] pCQueue;

			//_acceptThread[0].join();
			/*for (int i = 0; i < _iWorkThreadNum; i++)
				_workerThread[i].join();
*/
			_acceptThread[0].detach();
			_acceptThread[0].~thread();

			for (int i = 0; i < 2; i++)
				_logicThread[i].join();

		
			cout << "called detor" << endl;
		};


		void Stop(){
			this->~IServer();
		}
		bool init(int port);
		void PostTcpRecv(CSocket*); //WSARECV 
		

		void AcceptProc(int pram);
		void WorkerTcpProc(int param);
		void GameProc(int  pram);


		void RecvTcpBufEnqueue(CSocket* pCSocket, int iPacketSize);
		void RecvTcpBufDequeue(CSocket*, char**, int*);
		void EnqueueClose(CSocket* pCSocket);
		
	
		
		int  ReInitCSocket(CSocket* pCSocket);
		void SocketEnqueueTcp(CSocket* pCSocket);

		int getMaxQueue()	const	{ return _iMaxQueue; }
		int getMaxChannel()	const	{ return _iMaxChannel; }
		int getMaxChannelInProcess()	const	{ return _iMaxChannelInProcess; }
		

		CFiexedSizeQueue& getCQueue(int idx)	const{ return pCQueue[idx];	}
		CChannel& getChannel(int idx)			const{ return pCChannel[idx];}




	private:

		std::function<int(CSocket*, char*)> requestRMI[20];

		bool _done;
		std::thread* _logicThread;
		std::thread* _workerThread;
		std::thread* _acceptThread;

		SOCKET	_sockListener;

		const int _iMaxQueue;
		const int _iMaxChannelInProcess;
		const int _iMaxChannel;
		const int _iMaxUserNum;
		const int _iWorkThreadNum;
		
		CSocket *pCSocket;
		CChannel* pCChannel;
		CFiexedSizeQueue* pCQueue;

		HANDLE	_hIocpWorkTcp,
			_hIocpAcpt;


	};

	extern IServer	myServer;

}
#endif
