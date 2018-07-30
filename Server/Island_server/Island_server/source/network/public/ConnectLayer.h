#pragma once
#include "source/global/ISL_Def.h"
#include <WinSock2.h>

namespace ISL_NET
{

	class ISL_Listener
	{
	public:
		ISL_Listener() = default;
		~ISL_Listener();

		ISL_RESULT_CODE CreateListener(const char* ipAddr, USHORT pport);

		ISL_RESULT_CODE Tick();
	private:
		SOCKET _socket;
		char _revData[255];
	};


	class ISL_Connection 
	{
	public:

		
	private:
		

	};

}