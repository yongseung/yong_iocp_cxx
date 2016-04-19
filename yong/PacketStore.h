#ifndef _PACKETSTORE_H_
#define _PACKETSTORE_H_

#include <string>
#include <list>
#include <vector>
#include "CPStream.h"
#include "protocol.h"

namespace PacketStore{

	class header{
	public:
		short _size;
		short _type;

		header(){}
		~header(){}
	};

	class request_login : public header{
	public:
		int _id;
		int _pw;

		request_login(){
			_type = REQUEST_LOGIN;//request_login
		}

		request_login(int param1, int param2) : _id(param1), _pw(param2){
			_type = REQUEST_LOGIN;//request_login
		}
		~request_login(){}

		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _id;
			streamBuf << _pw;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
			//cout << "final _size of login class" << _size << endl;
		}

		void serialR(Ryan::CPStreamReader& streamBuf){
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _id;
			streamBuf >> _pw;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
		}

	};


	class answer_login : public header{
	public:
		int _id;
		std::string _msg;

		answer_login(int id, std::string val) : _id(id), _msg(val){
			_type = ANSWER_LOGIN;//request_login // 타입전해주기~
		}
		answer_login() : _id(0), _msg("Good morning"){
			_type = ANSWER_LOGIN;//request_login // 타입전해주기~
		}
		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _id;
			streamBuf << _msg;
			//	cout << "size of _msg  " << _msg.size() << endl;

			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
			//		cout << "request char WRITER****************** size is " << _size << endl;
		}

		void serialR(Ryan::CPStreamReader& streamBuf){
			//	cout << "rueqest char READER*****************" << endl;
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _id;
			streamBuf >> _msg;

		}

	};

	class request_change_channel : public header{
	public:
		int _id;
		int _channel;
		request_change_channel() : _id(0), _channel(0){
			_type = REQUEST_CHANGE_CHANNEL;//request_login // 타입전해주기~
		}
		request_change_channel(int id, short channel) : _id(id), _channel(channel) {
			_type = REQUEST_CHANGE_CHANNEL;//request_login // 타입전해주기~
		}
		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _id;
			streamBuf << _channel;

			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
			//		cout << "request char WRITER****************** size is " << _size << endl;
		}

		void serialR(Ryan::CPStreamReader& streamBuf){
			//	cout << "rueqest char READER*****************" << endl;
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _id;
			streamBuf >> _channel;

		}

	};


	class answer_init : public header{
	public:
		int	_myChannel;
		int _userNum;

		answer_init() : _myChannel(0), _userNum(0){
			_type = ANSWER_INIT;//request_login
		}
		answer_init(int param1, int param2) : _myChannel(param1), _userNum(param2){
			//what is _type? which number!
			_type = ANSWER_INIT;//request_login
		}
		~answer_init(){}

		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _myChannel;
			streamBuf << _userNum;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
		}
		void serialR(Ryan::CPStreamReader& streamBuf){
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _myChannel;
			streamBuf >> _userNum;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
		}

	};

	class request_chat : public header{
	public:
		int _id;
		std::string _msg;

		request_chat(int id, std::string val) : _id(id), _msg(val){
			_type = REQUEST_CHAT;//request_login // 타입전해주기~
		}
		request_chat() : _id(0), _msg(""){
			_type = REQUEST_CHAT;//request_login // 타입전해주기~
		}

		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _id;
			streamBuf << _msg;

			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
			//		cout << "request char WRITER****************** size is " << _size << endl;
		}

		void serialR(Ryan::CPStreamReader& streamBuf){
			//	cout << "rueqest char READER*****************" << endl;
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _id;
			streamBuf >> _msg;

		}

	};

	template <typename T>
	class notify_userlist_answer : public header{
	public:
		std::list<T> _userList;

		notify_userlist_answer() {
			_type = ANSWER_NOTIFY_USERLIST;
		}

		~notify_userlist_answer(){
			_userList.clear();
		}
		void serialW(Ryan::CPStreamWriter& streamBuf){
			streamBuf << _type;
			streamBuf << _userList;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
			streamBuf.setBodySize();
			_size = streamBuf.getBodySize();
		}

		void serialR(Ryan::CPStreamReader& streamBuf){
			streamBuf >> _size;
			streamBuf >> _type;
			streamBuf >> _userList;
			//데이터 입력이 끝나야 사이즈를 알 수 있다. 가장 맨 앞에 입력된다 
		}
	};
}
#endif
