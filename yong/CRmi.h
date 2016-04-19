//#ifndef _CRMI_H_
//#define _CRMI_H_

#include "CSocket.h"

namespace Ryan{
	class IServer;
}


namespace Rmi{
	
	//0~4
	int RequestLogin(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestInitialize(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestInitializeNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int NotifyUserList(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestChangeChannel(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	
	//5~
	int RequestChangeChannelNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestChat(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestLogout(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);
	int RequestLogoutNext(Ryan::CSocket* pCSocket, char *cpPacket, Ryan::IServer* pServer);


}
//#endif
