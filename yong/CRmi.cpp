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
		CopyMemory(&bodySize, cpPacket, sizeof(short));            // ��Ŷ�� ������ bodysize, typeid, text bc���ٶ� ���� ���̵� �߰��ؾߵ��ݾ�
		Ryan::CPStreamReader myReader(cpPacket, bodySize+4); //�������� ������  �����༭ ���� ����ŰŰ.
		myReader >> login; //login ��Ŷ�� �Ἥ 

		//if (login == success) send_chat        fail == logout!
		//// ---- DataBase ���� ���ϵ��� �� ----

		PacketStore::answer_login ans_login(pCSocket->getIndex(), " login success");// (pCSocket->getInde��x(), "Enter");	

		//����ũ�� 1024; ���� üũ �ؾ� �Ѵ�! overflow ���� �ƴ���! max�� �������ֱ� �Ǵ� ���� ũ�� ������ֱ�
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		myWriterBuffer << ans_login;

		//�����ߴٸ� ������ �˸��� �ʱ�ȭ ����
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4);
		RequestInitialize(pCSocket, cpPacket, pServer);

		//���н� ���� ���� �����ش�
		//���� �κ� ������ֱ� ******************************************* 
		return 1;
	}

	//���� Ŭ������ ä�ΰ� ť���� �����ִ� ���� �� ���Ŀ��� ä��Ŭ������ ����ϱ�
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


		//newProcess 0 ,1,2,3 �߿� �ϳ��� ���� 3 new�� 3
		//ó�� �����尡 ���� ���
		if (pCSocket->getFiexedQ() == indexOfFixexdeQ)
		{
		//	cout << "same thread" << endl;
			// ---- ���� ������ �������� ���� ȣ�� ----
			RequestInitializeNext(pCSocket, cpPacket, pServer);
			//������ ������ �� �Դٰ�
		}


		else
		{
		//	cout << "diffrent thread" << endl;

			// �ٸ� ���μ����� ���
			// 1�� ���μ����� �Ӹ�
			pCSocket->setFixedQ(indexOfFixexdeQ);
			//�̰� ������ ���� ��Ŷ�� ����Ű�µ� ���� ���۵��� 0���� �ʱ�ȭ �Ǿ������� ���������������������� ��
			pCSocket->setRecvBegin(cpPacket);


			//��Ŷ �̸� ��ֱ�
			dummy = REQUEST_INITIALIZE_NEXT;// request init intd RequestInitializeInt
			//���� ����Ű�� ���� ��Ŷ Ÿ��
			pOffset = cpPacket + sizeof(short);


			// ���μ��� �����忡 �����Ͱ� 2�� �ִٴ°��� �ٵ� ���������� ������� ó���Ѵ�. 
			// ���� ��� ���� ����ť���� �����Ʈ�� �Ե��� �ƴ϶� �ϳ� ó���ؾߵɰ� �ϳ� �ִٴ� �ǹ���
			// �����尡 �ٲ� ó���Ѵٰ� �ص� �� �޶����°� ���� 2������ ���� ��û�� �ϴ°� �ƴ϶� �� �ϳ� ó���� �ǹ̴�����ŭ �̰� ��û
			// �׸��� 1�� �����尡 ���� �ٽ� ť�� ��Ƽ� ó�� �Ѵٸ� 2�������忡�� ���� ��ɵ� ���� ó���ϸ� �ȴ�! �� ������ safe!

			if (pOffset >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
			{
				pOffset -= RINGBUFSIZE;
				CopyMemory(pOffset, &dummy, sizeof(short));
			}

			// ������ ��Ŷ�� ��ġ�� ������ �ι�° �Ǵ� ù��° ��� 

			else
			{
				//cout << "dumy is" << dummy << endl;
				CopyMemory(pOffset, &dummy, sizeof(short));


				dummy = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pOffset; // ���������� 3��° ��� ���̴� 1 ���� 0�̶�� ���� if�� ����
				if (dummy < sizeof(short)) // ���̰� 1�ΰ��ۿ� ���� �̷� ��� �������� ���� ������Ѽ� �־��ش�! ���� ����� �����ϱ�
				{
					pCSocket->copyRecvRingBufOfZero(*(pCSocket->getRecvRingBuf() + RINGBUFSIZE));//_type���簡 ���� ������ �������̶�� ó������ �������ֱ�
				}
			}

			//������ �ٲܳ��� ť���� ó���Ұ� �ϳ� �Դٰ� �˸��� 
			pServer->getCQueue(indexOfFixexdeQ).SocketEnqueue(pCSocket);
		}
		return 1;
	}

	//ä�ο� ��� �ϰ� 
	int RequestInitializeNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer)
	{
		int		currentChannel;
		int		userCount;

		PacketStore::answer_init my_answer;// ���� ���⼭ ��� ���� ������ ������ �����;� �Ѵ� ������ ���, â�� �� ��� ������
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		currentChannel = pCSocket->getChannel();
		printf("RequestInitializeNext(%d) channel is (%d) user num(������) is (%d)\n", pCSocket->getIndex(), currentChannel, pServer->getChannel(currentChannel).getUserNum());


		my_answer._myChannel = currentChannel;
		my_answer._userNum = (pServer->getChannel(currentChannel).getUserNum()) + 1; //�� ����
		myWriterBuffer << my_answer; //��Ʈ������ �����!

		// ---- log ----
		std::cout << "initNext::packet currentChannel is " << currentChannel << " userNum is(with me) " << my_answer._userNum << endl;
		//����
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		// ä�� list�� ���� ���� ����� ������ ������
		NotifyUserList(pCSocket, NULL, pServer);
		//int list �� ���·� �����͸� ���´�

		//������ ������
		userCount = pServer->getChannel(currentChannel).getUserNum();

		PacketStore::request_chat bc_my_enter(pCSocket->getIndex(), "Enter");
		myWriterBuffer.init();//�����Ͱ� ���� �Ǿ� �ִ��� �����͸� �Ȱ��ְ� �� �ʱ�ȭ ���� �ٽ� ���� �ִ�. ������ �߿��ϴ�
		myWriterBuffer << bc_my_enter;

		//@@unit test
		//Ryan::CPStreamReader myReader(myWriterBuffer.getBuf(), myWriterBuffer.getBodySize());
		//PacketStore::request_chat test;
		//myReader >> test;


		//���� ä�ο� �ִ� ����鿡�� ��ε� ĳ��Ʈ �ϱ� ���� �Դٰ�! 
		//�������� ��ε� ĳ��Ʈ ���ش�~
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//������ ä�ο� ��� ����Ʈ�� set�Ѵ�
		pServer->getChannel(currentChannel).SetUserLink(pCSocket);
		return 1;
	}


	//ä�ο� �ִ� ���� ����� ��û�ڿ��� �����Ѵ� (��������ŭ �ݺ� ������)
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
			//����Ʈ�� ����� ���� �ݺ���
			while (it != pServer->getChannel(myChannel).getUser().end()){
				cout << "inwhile is " << (*it)->getIndex() << endl;
				my_notify_userlist._userList.push_back((*it)->getIndex());
				it++;
			}
			//index ���簡 ������ ��Ʈ�� ���ۿ� ��ִ´�!
			myWriterBuffer << my_notify_userlist;
			PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);
			cout << "******************************************************" << endl;
		}

		else {
			cout << "user is 0 in this channel" << endl;
		}
		return 1;
	}


	// ���� Ŭ������ ť���� ä�ΰ��� ����! �� ä�� ����Ʈ kill, set�� �������� ó�����ش�
	// ������ ��Ŷ�� ������ �Ӵ϶�� �׳� �ٷ� �Ľ��ϸ� �ȴ� Ư���� ���� �ʿ䰡 ���ٸ�!
	int RequestChangeChannel(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		//short					dummy = REQUEST_LOGIN_DB;

		cout << "Client request change channel" << endl;
		char*	pOffset;
		int		currentChannel;
		int		userCount;
		int		toChannel;
		int		whichOfQueue;
		short	dummy,	bodySize;

		CopyMemory(&bodySize, cpPacket, 2);            // ��Ŷ�� ������ bodysize, typeid, text bc���ٶ� ���� ���̵� �߰��ؾߵ��ݾ�

		Ryan::CPStreamReader myReader(cpPacket, bodySize + 4); //�������� ���� ����
		PacketStore::request_change_channel user_message;// (pCSocket->getInde��x(), "Enter");	
		myReader >> user_message; //��Ŷ ������ ���� �޼��� ��ü�� ���� ��ִ´� 
		PacketStore::request_chat bc_user_out(pCSocket->getIndex(), " is out");// (pCSocket->getInde��x(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_out;

		//Ŭ��� ����� �ؾߵ� ��Ʈ���� ���������� ���������� ���� ��Ʈ������ �´ٰ� ����
		toChannel = user_message._channel;

		whichOfQueue = toChannel / pServer->getMaxChannelInProcess();

		cout << "to channel is "<< toChannel << endl;

		//1.  kill user ���ְ�  ���� ���鿡�� ������ �����ش� 
		currentChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(currentChannel).getUserNum();

		// ä�� ����Ʈ���� ����  �����
		pServer->getChannel(currentChannel).KillUserLink(pCSocket);

		//�������� ��ε� ĳ��Ʈ ���ش�~
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4, pServer);
		}
		else {
			cout << "REQUEST CHANGE CHANNEL::user is 0 in this channel" << endl;
		}

		//2. ä�� �ε����� �ٲ۴�. set user ���ְ� Socketenqueue ���ش� if) diffrent thread
		pCSocket->setChannel(toChannel);


		//ó�� �����尡 ���� ��� ������ iQueue �� �״�� �д�
		if (pCSocket->getFiexedQ() == whichOfQueue)
		{
		//	cout << "change Channel::same thread" << endl;
			// ---- ���� ������ �������� ���� ȣ�� ---- type �ٲ� �ʿ䰡 ����. ������ ��� ä�������� �Ľ��ϹǷ�
			RequestChangeChannelNext(pCSocket, cpPacket, pServer);
			//������ ������ �� �Դٰ�
		}

		else
		{
		//	cout << "change Channel::diffrent thread" << endl;

			// �ٸ� ���μ����� ���
			pCSocket->setFixedQ(whichOfQueue);

			//�̰� ������ ���� ��Ŷ�� ����Ű�µ� ���� ���۵��� 0���� �ʱ�ȭ �Ǿ������� ���������������������� ��
			pCSocket->setRecvBegin(cpPacket);


			//��Ŷ �̸� ��ֱ�
			dummy = REQUEST_CHANGE_CHANNEL_NEXT;//request init intd RequestChangeChannelInt

			//���� ����Ű�� ���� ��Ŷ Ÿ�� 
			pOffset = cpPacket + sizeof(short);

			//Ÿ�� �������ֱ� ���� ��Ŷ�� ġȯ�ϴ°�
			if (pOffset >= pCSocket->getRecvRingBuf() + RINGBUFSIZE)
			{
				pOffset -= RINGBUFSIZE;
				CopyMemory(pOffset, &dummy, sizeof(short));
			}

			// ������ ��Ŷ�� ��ġ�� ������ �ι�° �Ǵ� ù��° ��� 
			else
			{
				//cout << "dumy is" << dummy << endl;
				CopyMemory(pOffset, &dummy, sizeof(short));
				dummy = pCSocket->getRecvRingBuf() + RINGBUFSIZE - pOffset; // ���������� 3��° ��� ���̴� 1 ���� 0�̶�� ���� if�� ����
				if (dummy < sizeof(short)) // ���̰� 1�ΰ��ۿ� ���� �̷� ��� �������� ���� ������Ѽ� �־��ش�! ���� ����� �����ϱ�
				{
					pCSocket->copyRecvRingBufOfZero(*(pCSocket->getRecvRingBuf() + RINGBUFSIZE));//_type���簡 ���� ������ �������̶�� ó������ �������ֱ�
				}
			}

			//������ �ٲܳ��� ����ť���� ó���Ұ� �ϳ� �Դٰ� �˸��� 
			pServer->getCQueue(whichOfQueue).SocketEnqueue(pCSocket);
		}
		return 1;
	}

	// ť�� ä���� �ٲ� �����̴� �ٲ���ٰ� ��ε�ĳ��Ʈ! �ٲ� �̿��� ���� ���
	int RequestChangeChannelNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){


		int		currentChannel;
		int		userCount;

		PacketStore::request_chat ans_change_channel(pCSocket->getIndex(), " : change channel success\n");// (pCSocket->getInde��x(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;

		myWriterBuffer << ans_change_channel;

		currentChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(currentChannel).getUserNum();

		printf("RequestChangeChannelNext(%d) channel is (%d) user num(������) is (%d)\n", pCSocket->getIndex(), currentChannel, pServer->getChannel(currentChannel).getUserNum());

		// �����ߴٰ� �˸���
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		// ä�� list�� ���� ���� ����� ������ ������
		NotifyUserList(pCSocket, NULL, pServer);

		//��Ʈ�� ���� �ʱ�ȭ �� �޼��� ���� �ٲٱ�
		myWriterBuffer.init();
		ans_change_channel._msg = " is enter(change channel)\n";
		myWriterBuffer << ans_change_channel;

		//���� ä�ο� �ִ� ����鿡�� ��ε� ĳ��Ʈ �ϱ� ���� �̻� �Դٰ�
		//����ī��Ʈ�� 0�� �ƴϸ�!
		if (userCount != 0){
			PostTcpSend(currentChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//������ ä�� ����Ʈ�� set�Ѵ�
		pServer->getChannel(currentChannel).SetUserLink(pCSocket);
		return 1;
	}

	int RequestChat(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){

		short bodySize;
		int		myChannel;
		int		userCount;

		CopyMemory(&bodySize, cpPacket, 2);            // ��Ŷ�� ������ bodysize, typeid, text bc���ٶ� ���� ���̵� �߰��ؾߵ��ݾ�
														//�ٵ� ��Ŷ ������ ���� ���̵� �Բ� �شٸ� �׳� �״�� �ּ� �Ѱ��ָ� �ȴ� 
		myChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(myChannel).getUserNum();
		
		Ryan::CPStreamReader myReader(cpPacket, bodySize+4); //�������� ���� ����
		PacketStore::request_chat bc_user_message;// (pCSocket->getInde��x(), "Enter");	
		myReader >> bc_user_message; //��Ŷ ������ ���� �޼��� ��ü�� ���� ��ִ´� 
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_message;

		//log
		cout << "CRMI::       RequestChat is " << bc_user_message._id << " packet is:"<< bc_user_message._msg;
		
		//���� ä�ο� �ִ� ����鿡�� ��ε� ĳ��Ʈ �ϱ� 
		PostTcpSend(myChannel, cpPacket, bodySize+4, pServer);

		return 1;
	}

	//list ���� �� -> �뺸 -> ���� �ʱ�ȭ
	int RequestLogoutNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		//short					dummy = REQUEST_LOGIN_DB;
		
		//�α׾ƿ� ����� ���� ä�� ����Ʈ���� ���Ÿ� �Ѵ�. ��ε� ĳ��Ʈ �Ѵ�. ���� �ݴ´�! 
		//Ŀ��Ʈ�� �ϰ� �����Ͱ� ���� ������ ���ڽĵ� ���δ�! �̰� ��ε�ĳ��Ʈ�� ����ؼ� Ȯ���ص� �Ǵ±��� ����
		cout << "client request logout next (inner request)" << endl;
		int		myChannel;
		int		userCount;

		PacketStore::request_chat bc_user_logout(pCSocket->getIndex(), " is out(logout)");// (pCSocket->getInde��x(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		myWriterBuffer << bc_user_logout;

		myChannel = pCSocket->getChannel();
		userCount = pServer->getChannel(myChannel).getUserNum();

		// ä�� ����Ʈ���� ���� �����ϱ�
		pServer->getChannel(myChannel).KillUserLink(pCSocket);

		//�������� ��ε� ĳ��Ʈ ���ش�~
		if (userCount != 0){
			PostTcpSend(myChannel, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize()+4, pServer);
		}
		else {
			cout << "user is 0 in this channel" << endl;
		}

		//���Ͽ��� �뺸������������ ���⼭ ����
		//CSocket class init
		pServer->ReInitCSocket(pCSocket);

	//db�ʿ��ϴٸ� �۾��ϱ�
		return 1;
	}

	//Ŭ���̾�Ʈ���� shutdown �Լ��� �����ϴ� RMI �� ȣ���ϵ��� ��Ŷ�� ������.
	int RequestLogout(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer){
		cout << "client request logout" << endl;
		PacketStore::request_chat answer_logout(pCSocket->getIndex(), "confirm logoun\n");// (pCSocket->getInde��x(), "Enter");	
		Ryan::CPStreamWriterBuffer myWriterBuffer;
		answer_logout._type = ANSWER_LOGOUT;
		myWriterBuffer << answer_logout;
		Ryan::PostTcpSend(1, pCSocket, myWriterBuffer.getBuf(), myWriterBuffer.getBodySize() + 4);

		//db�ʿ��ϴٸ� �۾��ϱ�
		return 1;
	}
}