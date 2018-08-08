#include "../NetManager.h"
#include "source/global/ISL_Macro.h"



ISL_RESULT_CODE ISL_NET::ISL_NetManager::InitNetwork()
{
	
}

ISL_RESULT_CODE ISL_NET::ISL_NetManager::Tick()
{
	ISL_RESULT_CODE rst = ISL_OK;
	rst = _listener->Tick();
	//CHECKRST_FAILRETURN(rst)

	return ISL_OK;
}


ISL_RESULT_CODE ISL_NET::ISL_NetManager::Stop()
{

}

ISL_RESULT_CODE ISL_NET::ISL_NetManager::SendMSG(T_USER_ID userID, void* data)
{

}

ISL_NET::ISL_NetManager::~ISL_NetManager()
{
	WSACleanup();
}