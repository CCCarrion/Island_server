
#include "source/framework/ServerFlow.h"




int main(int argc, char* argv[])
{
	ServerFlow* pServer = ServerFlow::GetInstance();

	pServer->InitServer();


	while (true)
	{
		//Todo：设定Tick间隔时间


		//
		pServer->Tick();


	}

}