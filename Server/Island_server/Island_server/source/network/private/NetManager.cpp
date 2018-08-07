#include "../NetManager.h"
#include "source/global/ISL_Macro.h"



ISL_RESULT_CODE ISL_NET::ISL_NetManager::InitNetwork()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return ISL_NET_ERR;
	}



	_listener = new ISL_Listener();

	ISL_RESULT_CODE rst = ISL_OK;
	//½¨Á¢¼àÌý
	rst = _listener->CreateListener("127.0.0.1", 6666);


	return rst;
}

ISL_RESULT_CODE ISL_NET::ISL_NetManager::Tick()
{
	ISL_RESULT_CODE rst = ISL_OK;
	rst = _listener->Tick();
	//CHECKRST_FAILRETURN(rst)

	return ISL_OK;
}


ISL_RESULT_CODE ISL_NET::ISL_NetManager::SendMSG(T_USER_ID userID, void* data)
{

}

ISL_NET::ISL_NetManager::~ISL_NetManager()
{
	WSACleanup();
}