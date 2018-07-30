#pragma once

#include "source/helper/ISL_Helper.h"
#include "source/global/ISL_Def.h"
#include "source/network/NetManager.h"


using namespace ISL_HELPER;



//������ ��������
class ServerFlow : public ISL_Instance<ServerFlow>
{
public:
	ServerFlow();
	~ServerFlow();

	ISL_RESULT_CODE InitServer();

	ISL_RESULT_CODE Tick();

private:

};
