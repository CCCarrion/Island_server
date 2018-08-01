#pragma once

#include "source/helper/ISL_Helper.h"
#include "source/global/ISL_Def.h"
#include "public/ConnectLayer.h"

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

		ISL_RESULT_CODE Tick();

	private:
		ISL_Listener * _listener;
		std::thread _acceptThrd;
		std::thread _revThrd;
		std::thread _sendThrd;
	};





}