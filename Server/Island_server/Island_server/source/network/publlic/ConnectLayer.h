#pragma once

namespace ISL_NET
{
	typedef unsigned short NET_ERR_CODE;

	class SocketListener
	{
	public:
		SocketListener(char* ippAddr, USHORT pport);
		~SocketListener();

		NET_ERR_CODE Listen();
	private:
		NET_ERR_CODE CreateSocket();
	};

}