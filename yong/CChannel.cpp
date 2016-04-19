#include "stdafx.h"
#include "CChannel.h"

namespace Ryan{

void CChannel::InitChannel(int idx)
{
	_iIndex = idx;
}
//idx √§≥Œ ¿Œµ¶Ω∫

void CChannel::SetUserLink(CSocket *pCSocket)
{
	_userList.push_back(pCSocket);
	
	//log
	std::cout << "set[" << _iIndex << "]::" << pCSocket <<" enrol compelete"<<endl;
	for (_it = _userList.begin(); _it != _userList.end(); _it++)
	{
		std::cout << "Now of set[" << _iIndex << "]::" << *_it << endl;
	}
}

void CChannel::KillUserLink(CSocket *pCSocket)
{
	_it = find(_userList.begin(), _userList.end(), pCSocket);
	_userList.erase(_it);
	cout << "kill userlink success remain user is " <<_userList.size() << endl;
}

}