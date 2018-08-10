#pragma once

#include "source/helper/ISL_Helper.h"
#include "source/global/ISL_Def.h"
#include "public/ISL_NetWorker.h"

#include "source/tool/thread/ISL_MultiThread.h"
#include "source/network/ISL_NetDef.h"
#include "source/msg/ISL_MsgDef.h"

namespace ISL_NET
{

	using namespace ISL_HELPER;
	using namespace ISL_MSG;

	class ISL_NetManager : public ISL_Instance<ISL_NetManager>
	{
	public:
		ISL_NetManager() = default;
		~ISL_NetManager();

		ISL_RESULT_CODE InitNetwork();

		ISL_RESULT_CODE Start();


		ISL_RESULT_CODE Tick();

		ISL_RESULT_CODE Stop();

		ISL_RESULT_CODE SendMSG(T_CONN_ID connID, ISL_MsgBase* pMSG);

		ISL_RESULT_CODE RecvMSG(T_CONN_ID connID, char* data,DWORD dataLen);

		int GetNetWorkerNum() { return _maxWorkersNum; };

	private:

		int _maxWorkersNum;
		std::thread* _arsWorkers;
		ISL_NET_Work_Indicator* _workerIndicators;

		ISL_NET_IOCP_Work _netWork;
		
		
		
	};





}