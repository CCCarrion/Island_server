#pragma once

#include "source/global/ISL_Def.h"
#include "source/tool/thread/ISL_MultiThread.h"
#include "../public/ISL_IO.h"
#include  <map>

using namespace ISL_TOOL;

namespace ISL_NET {



	class ISL_NET_Work_Indicator :public ISL_Worker_Indicator
	{
	public:

		ISL_NET_IOCP_Work	*pAttachedWork;

	};





	class ISL_NET_IOCP_Work {

	public:
		static void arsWork(ISL_Worker_Indicator& worker_info);

		ISL_RESULT_CODE InitWork();
		ISL_RESULT_CODE StartWork(USHORT port);


		void SendData(SOCKET skt, byte* datBufr,DWORD datlen);

		bool AssociateWithIOCP(ISL_PER_SOCKET_CONTEXT* pSocCTX);

		ISL_RESULT_CODE EndWork();
		ISL_RESULT_CODE	DeInitWork();
		HANDLE                      hIOCompletionPort;
		bool						bEndWork;
	private:
		bool _PostAccept(ISL_PER_IO_CONTEXT* pIoContext);
		bool _PostRecv(ISL_PER_IO_CONTEXT* pIoContext);
		bool _PostSend(ISL_PER_IO_CONTEXT* pIoContext);

		void _DoAccept(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext);
		void _DoSend(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext);
		void _DoRecv(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext);


		std::map<SOCKET, ISL_PER_SOCKET_CONTEXT*> _mapSocketCtx;
		ISL_PER_SOCKET_CONTEXT _listenerCtx;

	};
}
