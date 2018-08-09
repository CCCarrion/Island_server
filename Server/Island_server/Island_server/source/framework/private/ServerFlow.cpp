
#include "../ServerFlow.h"

using namespace ISL_NET;

ServerFlow::ServerFlow()
{

}

ServerFlow::~ServerFlow()
{

}

ISL_RESULT_CODE ServerFlow::InitServer()
{
	ISL_RESULT_CODE rst = ISL_OK;

	ISL_NetManager* netMnger = ISL_NetManager::GetInstance();
	netMnger->InitNetwork();


	//Todo ��ʱ
	netMnger->Start();


	return ISL_OK;
}

ISL_RESULT_CODE ServerFlow::Tick()
{

	ISL_NetManager::GetInstance()->Tick();



	return 0;
}
