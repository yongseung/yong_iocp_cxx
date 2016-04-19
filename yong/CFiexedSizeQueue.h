#ifndef _CFIEXEDSIZEQUEUE_H_
#define _CFIEXEDSIZEQUEUE_H_

#include "CSocket.h"
#include <windows.h>            
#include <process.h>     


#define FIXEDQUEUEEMPTY			NULL
#define FIXEDSIZEQUEUEMAX		8192

namespace Ryan{

	class smartCriticalSection {
	private:
		CRITICAL_SECTION*	_pCS;

	public:
		smartCriticalSection(CRITICAL_SECTION* pCS) :_pCS(pCS){ EnterCriticalSection(_pCS);	}
		~smartCriticalSection()	{ LeaveCriticalSection(_pCS);}
	};


	class CFiexedSizeQueue
	{
	public:
		CFiexedSizeQueue(){};
		~CFiexedSizeQueue(){
			DeleteCriticalSection(&_cs);
		}

		void InitProcess(int idx);
		void SocketEnqueue(CSocket* );
		void SocketDequeue(CSocket **);

		void setIndex(int id){ _iIndex = id; }
		int getIndex()	const	{ return _iIndex; }


	private:
		CRITICAL_SECTION		_cs;
		CSocket*			_pCSocketQueue[FIXEDSIZEQUEUEMAX];
		int		_iIndex;

		int	_iBegin,
			_iEnd,
			_iRestCount;
	};

}
#endif
