// Yong.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "IServer.h"
#include <future>


#pragma comment(lib,"libmysql")

#define DB_HOST "localhost"
#define DB_USER "user_id"
#define DB_PASS "user_pw"
#define DB_NAME "db"

using namespace std;
class Cserver : public Ryan::IServer
{
public:
	//virtual bool init(int port) override;
};




int _tmain(int argc, _TCHAR* argv[])
{


	//MYSQL mysql;
	auto myServer = new Cserver;

//	printf("%s",mysql_get_client_info());
	//mysql_init(&mysql);
	//if (!mysql_real_connect(&mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3307, (char*)NULL, 0)){
	//	printf("%s\n", mysql_error(&mysql));
	//	return -1;
	//}

	//mysql_close(&mysql);
	myServer->init(5555);

	bool still_alive = true;

	auto command = [&still_alive]()
	{
		std::string str;
		char temp[1024];
		std::cin >> temp;
		str = temp;

		return str;
	};

	auto fut = std::async(std::launch::async, command);

	while (still_alive)
	{
		auto cmd = fut.get();
		if (cmd == "/request")
		{
			std::cout << "/request will be updated" << std::endl;
		}
		else if (cmd == "/q")
		{

			myServer->Stop();
			still_alive = false;
		}
		if (still_alive)
			fut = std::async(std::launch::async, command);
	}

	
	return 0;
}

