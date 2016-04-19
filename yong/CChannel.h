#ifndef _CCHANNEL_H_
#define _CCHANNEL_H_

#include "CSocket.h"
#include <list>
#include <algorithm>

namespace Ryan{

	class CChannel
	{
	public:
		CChannel(){}
		~CChannel(){}

		void InitChannel(int idx);
		void SetUserLink(CSocket *pCSocket);
		void KillUserLink(CSocket *pCSocket);

		std::list<CSocket*>& getUser()	{ return _userList; };
		int getUserNum()	const		{ return _userList.size(); };

	private:
		std::list<CSocket*> _userList;
		std::list<CSocket*>::iterator _it;

		 int _iIndex;
	};

}
#endif
