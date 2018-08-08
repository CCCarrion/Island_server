#pragma once

#include "source/helper/ISL_Helper.h"
#include "source/global/ISL_Def.h"
#include "public/ConnectLayer.h"
#include "public/ISL_NetWorker.h"

#include <thread>

namespace ISL_NET
{

	using namespace ISL_HELPER;


	class ISL_NetManager : public ISL_Instance<ISL_NetManager>
	{
	public:
		ISL_NetManager() = default;
		~ISL_NetManager();

		ISL_RESULT_CODE InitNetwork();

		ISL_RESULT_CODE Start();


		ISL_RESULT_CODE Tick();

		ISL_RESULT_CODE Stop();


		ISL_RESULT_CODE SendMSG(T_USER_ID userID, void* data);

		ISL_RESULT_CODE RecvMSG(T_USER_ID userID, void* data,DWORD dataLen);

	private:
		ISL_Listener * _listener;

		int _maxWorkersNum;
		std::thread* _arsWorkers;
		std::thread* _sendWorker;

		ISL_NET_IOCP_Work _netWork;
		std::map<T_USER_ID, SOCKET> _mapUserSocket;
	};





}