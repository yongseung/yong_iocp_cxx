#include "stdafx.h"
#include "CRmi.h"
#include "IServer.h"
#include "PacketStore.h"


using namespace std;

namespace Rmi{

	int RequestLogin(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
	
		cout << "\n\nrequest login" << endl;
		short bodySize = 0;
		PacketStore::request_login login;
		CopyMemory(&bodySize, cpPacket, sizeof(short));            // 패킷의 내용은 bodysize, typeid, text bc해줄때 유저 아이디를 추가해야되잖아
		Ryan::CPStreamReader myReader(cpPacket, bodySize+4); //시작점과 사이즈  더해줘서 끝점 가르키키.
		myReader >> login; //login 패킷에 써서 

		//if (login == success) send_chat        fail == logout!
		//// ---- DataBase 에서 리하도록 함 ----

		PacketStore::answer_login ans_login(pCSocket->getIndex(), " login success");// (pCSocket->getIndeㅇx(), "Enter");	

		//고정크기 1024; 버퍼 체크 해야 한다! overflow 인지 아닌지! max값 지정해주기 또는 가변 크기 만들어주기
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		myWriterBuffer << ans_login;

		//성공했다면 성공을 알리고 초기화 진행
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4);
		RequestInitialize(pCSocket, cpPacket, pServer);

		//실패시 소켓 해제 시켜준다
		//여기 부분 만들어주기 ******************************************* 
		return 1;
	}

	//소켓 클래스의 채널과 큐값을 정해주는 역할 그 이후에는 채널클래스에 등록하기
	int RequestInitialize(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer)
	{
		cout << "Requestinitialize" << endl;

		char	*pOffset;
		short					dummy;
		int		indexOfFixexdeQ;
		int		channel;

		//0,1,2,3  // 4,5,6,7         threads is 2
		pCSocket->setChannel(channel = rand() % pServer->getMaxChannel());
		indexOfFixexdeQ = channel / pServer->getMaxChannelInProcess();


		//newProcess 0 ,1,2,3 중에 하나다 현재 3 new도 3
		//처리 쓰레드가 같은 경우
		if (pCSocket->getFiexedQ() == indexOfFixexdeQ)
		{
		//	cout << "same thread" << endl;
			// ---- 같은 쓰레드 내에서는 직접 호출 ----
			RequestInitializeNext(pCSocket, cpPacket, pServer);
			//데이터 보낸라 나 왔다고
		}


		else
		{
		//	cout << "diffrent thread" << endl;

			// 다른 프로세스인 경우
			// 1번 프로세스로 임명
			pCSocket->setFixedQ(indexOfFixexdeQ);
			//이게 없으니 다음 패킷을 가르키는데 다음 버퍼들은 0으로 초기화 되어있으니 ㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋ 니
			pCSocket->setRecvBegin(cpPacket);


			//패킷 이름 써넣기
			dummy = REQUEST_INITIALIZE_NEXT;// request init intd RequestInitializeInt
			//현재 가르키는 것은 패킷 타입
			pOffset = cpPacket + sizeof(short);


			// 프로세스 쓰레드에 데이터가 2개 있다는거지 근데 순차적으로 쓰레드는 처리한다. 
			// 전혀 상관 없다 게임큐에는 몇바이트가 왔따가 아니라 하나 처리해야될게 하나 있다는 의미임
			// 쓰레드가 바뀌어서 처리한다고 해도 뭐 달라지는건 없네 2번에게 변경 요청을 하는게 아니라 뭐 하나 처리해 의미단위만큼 이걸 요청
			// 그리고 1번 쓰레드가 먼저 다시 큐를 잡아서 처리 한다면 2번쓰레드에서 다음 명령들 에코 처리하면 된다! 즉 쓰레드 safe!

			if (pOffset >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
			{
				pOffset -= RINGBUFSIZE;
				CopyMemory(pOffset, &dummy, sizeof(short));
			}

			// 마지막 패킷의 위치가 끝에서 두번째 또는 첫번째 라면 

			else
			{
				//cout << "dumy is" << dummy << endl;
				CopyMemory(pOffset, &dummy, sizeof(short));


				dummy = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pOffset; // 마지막에서 3번째 라면 더미는 1 만약 0이라면 위의 if로 간다
				if (dummy < sizeof(short)) // 더미가 1인경우밖에 없다 이럴 경우 마지막에 값을 복사시켜서 넣어준다! 더미 제대로 복사하기
				{
					pCSocket->copyRecvRingBufOfZero(*(pCSocket->getRecvRingBuf() + RINGBUFSIZE));//_type복사가 만약 링버퍼 끝지점이라면 처음으로 복사해주기
				}
			}

			//서버가 바꿀놈의 큐에게 처리할게 하나 왔다고 알린다 
			pServer->getCQueue(indexOfFixexdeQ).SocketEnqueue(pCSocket);
		}
		return 1;
	}

	//채널에 등록 하고 
	int RequestInitializeNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer)
	{
		int		currentChannel;
		int		userCount;

		PacketStore::answer_init my_answer;// 원래 여기서 모든 나의 데이터 정보를 가져와야 한다 아이템 목록, 창고 내 모든 데이터
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		currentChannel = pCSocket->getChannel();
		printf("RequestInitializeNext(%d) channel is (%d) user num(나빼고) is (%d)\n", pCSocket->getIndex(), currentChannel, pServer->getChannel(currentChannel).getUserNum());


		my_answer._myChannel = currentChannel;
		my_answer._userNum = (pServer->getChannel(currentChannel).getUserNum()) + 1; //나 포함
		myWriterBuffer << my_answer; //스트림으로 만들기!

		// ---- log ----
		std::cout << "initNext::packet currentChannel is " << currentChannel << " userNum is(with me) " << my_answer._userNum << endl;
		//응답
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		// 채널 list의 접속 유저 목록을 나한테 보낸다
		NotifyUserList(pCSocket, NULL, pServer);
		//int list 의 형태로 데이터를 보냈다

		//유저수 얻어오기
		userCount = pServer->getChannel(currentChannel).getUserNum();

		PacketStore::request_chat bc_my_enter(pCSocket->getIndex(), "Enter");
		myWriterBuffer.init();//데이터가 저장 되어 있더라도 포인터만 옴겨주고 값 초기화 없이 다시 쓸수 있다. 사이즈 중요하다
		myWriterBuffer << bc_my_enter;

		//@@unit test
		//Ryan::CPStreamReader myReader(myWriterBuffer.getBuf(), myWriterBuffer.getBodySize());
		//PacketStore::request_chat test;
		//myReader >> test;


		//같은 채널에 있는 사람들에게 브로드 캐스트 하기 내가 왔다고! 
		//나간것을 브로드 캐스트 해준다~
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//유저를 채널에 등록 리스트에 set한다
		pServer->getChannel(currentChannel).SetUserLink(pCSocket);
		return 1;
	}


	//채널에 있는 유저 목록을 요청자에게 제공한다 (유저수만큼 반복 가져옴)
	int NotifyUserList(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer)
	{
		int		myChannel;
		int		userCount;

		PacketStore::notify_userlist_answer<int> my_notify_userlist;
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		myChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(myChannel).getUserNum();
		auto it = pServer->getChannel(myChannel).getUser().begin();
		//log
		printf("NotifyUserList(%d) mychannel is (%d) usercount is (%d) \n", pCSocket->getIndex(), myChannel, userCount);

		if (userCount != 0){
			//리스트를 만들기 위한 반복문
			while (it != pServer->getChannel(myChannel).getUser().end()){
				cout << "inwhile is " << (*it)->getIndex() << endl;
				my_notify_userlist._userList.push_back((*it)->getIndex());
				it++;
			}
			//index 복사가 끝나면 스트림 버퍼에 써넣는다!
			myWriterBuffer << my_notify_userlist;
			PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);
			cout << "******************************************************" << endl;
		}

		else {
			cout << "user is 0 in this channel" << endl;
		}
		return 1;
	}


	// 소켓 클래스의 큐값과 채널값만 변경! 및 채널 리스트 kill, set은 다음에서 처리해준다
	// 복잡한 패킷의 내용이 앙니라면 그냥 바로 파싱하면 된다 특별히 만들 필요가 없다면!
	int RequestChangeChannel(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		//short					dummy = REQUEST_LOGIN_DB;

		cout << "Client request change channel" << endl;
		char*	pOffset;
		int		currentChannel;
		int		userCount;
		int		toChannel;
		int		whichOfQueue;
		short	dummy,	bodySize;

		CopyMemory(&bodySize, cpPacket, 2);            // 패킷의 내용은 bodysize, typeid, text bc해줄때 유저 아이디를 추가해야되잖아

		Ryan::CPStreamReader myReader(cpPacket, bodySize + 4); //시작점과 끝점 지정
		PacketStore::request_change_channel user_message;// (pCSocket->getIndeㅇx(), "Enter");	
		myReader >> user_message; //패킷 내용을 유저 메세지 객체에 값을 써넣는다 
		PacketStore::request_chat bc_user_out(pCSocket->getIndex(), " is out");// (pCSocket->getIndeㅇx(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_out;

		//클라와 약속을 해야됨 인트인지 문자형인지 문자형으로 가정 스트림으로 온다고 하자
		toChannel = user_message._channel;

		whichOfQueue = toChannel / pServer->getMaxChannelInProcess();

		cout << "to channel is "<< toChannel << endl;

		//1.  kill user 해주고  기존 고객들에게 퇴장을 말해준다 
		currentChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(currentChannel).getUserNum();

		// 채널 리스트에서 나를  지우고
		pServer->getChannel(currentChannel).KillUserLink(pCSocket);

		//나간것을 브로드 캐스트 해준다~
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4, pServer);
		}
		else {
			cout << "REQUEST CHANGE CHANNEL::user is 0 in this channel" << endl;
		}

		//2. 채널 인덱스를 바꾼다. set user 해주고 Socketenqueue 해준다 if) diffrent thread
		pCSocket->setChannel(toChannel);


		//처리 쓰레드가 같은 경우 소켓의 iQueue 는 그대로 둔다
		if (pCSocket->getFiexedQ() == whichOfQueue)
		{
		//	cout << "change Channel::same thread" << endl;
			// ---- 같은 쓰레드 내에서는 직접 호출 ---- type 바꿀 필요가 없다. 어차피 몇번 채널인지만 파싱하므로
			RequestChangeChannelNext(pCSocket, cpPacket, pServer);
			//데이터 보낸라 나 왔다고
		}

		else
		{
		//	cout << "change Channel::diffrent thread" << endl;

			// 다른 프로세스인 경우
			pCSocket->setFixedQ(whichOfQueue);

			//이게 없으니 다음 패킷을 가르키는데 다음 버퍼들은 0으로 초기화 되어있으니 ㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋ 니
			pCSocket->setRecvBegin(cpPacket);


			//패킷 이름 써넣기
			dummy = REQUEST_CHANGE_CHANNEL_NEXT;//request init intd RequestChangeChannelInt

			//현재 가르키는 것은 패킷 타입 
			pOffset = cpPacket + sizeof(short);

			//타입 복사해주기 받은 패킷에 치환하는것
			if (pOffset >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
			{
				pOffset -= RINGBUFSIZE;
				CopyMemory(pOffset, &dummy, sizeof(short));
			}

			// 마지막 패킷의 위치가 끝에서 두번째 또는 첫번째 라면 
			else
			{
				//cout << "dumy is" << dummy << endl;
				CopyMemory(pOffset, &dummy, sizeof(short));
				dummy = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pOffset; // 마지막에서 3번째 라면 더미는 1 만약 0이라면 위의 if로 간다
				if (dummy < sizeof(short)) // 더미가 1인경우밖에 없다 이럴 경우 마지막에 값을 복사시켜서 넣어준다! 더미 제대로 복사하기
				{
					pCSocket->copyRecvRingBufOfZero(*(pCSocket->getRecvRingBuf() + RINGBUFSIZE));//_type복사가 만약 링버퍼 끝지점이라면 처음으로 복사해주기
				}
			}

			//서버가 바꿀놈의 로직큐에게 처리할게 하나 왔다고 알린다 
			pServer->getCQueue(whichOfQueue).SocketEnqueue(pCSocket);
		}
		return 1;
	}

	// 큐랑 채널이 바뀐 상태이다 바뀐었다고 브로드캐스트! 바뀐 이용자 정보 얻기
	int RequestChangeChannelNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){


		int		currentChannel;
		int		userCount;

		PacketStore::request_chat ans_change_channel(pCSocket->getIndex(), " : change channel success\n");// (pCSocket->getIndeㅇx(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		myWriterBuffer << ans_change_channel;

		currentChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(currentChannel).getUserNum();

		printf("RequestChangeChannelNext(%d) channel is (%d) user num(나빼고) is (%d)\n", pCSocket->getIndex(), currentChannel, pServer->getChannel(currentChannel).getUserNum());

		// 성공했다고 알리기
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		// 채널 list의 접속 유저 목록을 나한테 보낸다
		NotifyUserList(pCSocket, NULL, pServer);

		//스트림 버퍼 초기화 및 메세지 내용 바꾸기
		myWriterBuffer.init();
		ans_change_channel._msg = " is enter(change channel)\n";
		myWriterBuffer << ans_change_channel;

		//같은 채널에 있는 사람들에게 브로드 캐스트 하기 내가 이사 왔다고
		//유저카운트가 0이 아니면!
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//유저를 채널 리스트에 set한다
		pServer->getChannel(currentChannel).SetUserLink(pCSocket);
		return 1;
	}

	int RequestChat(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){

		short bodySize;
		int		myChannel;
		int		userCount;

		CopyMemory(&bodySize, cpPacket, 2);            // 패킷의 내용은 bodysize, typeid, text bc해줄때 유저 아이디를 추가해야되잖아
														//근데 패킷 구조상 유저 아이디도 함께 준다면 그냥 그대로 주소 넘겨주면 된다 
		myChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(myChannel).getUserNum();
		
		Ryan::CPStreamReader myReader(cpPacket, bodySize+4); //시작점과 끝점 지정
		PacketStore::request_chat bc_user_message;// (pCSocket->getIndeㅇx(), "Enter");	
		myReader >> bc_user_message; //패킷 내용을 유저 메세지 객체에 값을 써넣는다 
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_message;

		//log
		cout << "CRMI::       RequestChat is " << bc_user_message._id << " packet is:"<< bc_user_message._msg;
		
		//같은 채널에 있는 사람들에게 브로드 캐스트 하기 
		PostTcpSend(myChannel, cpPacket, bodySize+4, pServer);

		return 1;
	}

	//list 제거 후 -> 통보 -> 소켓 초기화
	int RequestLogoutNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		//short					dummy = REQUEST_LOGIN_DB;
		
		//로그아웃 명령이 오면 채널 리스트에서 제거를 한다. 브로드 캐스트 한다. 소켓 닫는다! 
		//커넥트만 하고 데이터가 오지 않으면 그자식도 죽인다! 이건 브로드캐스트를 계속해서 확인해도 되는구나 ㅎㅎ
		cout << "client request logout next (inner request)" << endl;
		int		myChannel;
		int		userCount;

		PacketStore::request_chat bc_user_logout(pCSocket->getIndex(), " is out(logout)");// (pCSocket->getIndeㅇx(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_logout;

		myChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(myChannel).getUserNum();

		// 채널 리스트에서 소켓 해제하기
		pServer->getChannel(myChannel).KillUserLink(pCSocket);

		//나간것을 브로드 캐스트 해준다~
		if (userCount != 0){
			PostTcpSend(myChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//소켓에게 통보할지안할지는 여기서 설정
		//CSocket class init
		pServer->ReInitCSocket(pCSocket);

	//db필요하다면 작업하기
		return 1;
	}

	//클라이언트에게 shutdown 함수를 생성하는 RMI 를 호출하도록 패킷을 보낸다.
	int RequestLogout(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		cout << "client request logout" << endl;
		PacketStore::request_chat answer_logout(pCSocket->getIndex(), "confirm logoun\n");// (pCSocket->getIndeㅇx(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		answer_logout._type = ANSWER_LOGOUT;
		myWriterBuffer << answer_logout;
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		//db필요하다면 작업하기
		return 1;
	}
}