
#include "source/framework/ServerFlow.h"




int main(int argc, char* argv[])
{
	ServerFlow* pServer = ServerFlow::GetInstance();

	pServer->InitServer();


	while (true)
	{
		//Todo���趨Tick���ʱ��


		//
		pServer->Tick();


	}

}