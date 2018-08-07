
#include "../ISL_NetDef.h"
#include "../public/ISL_NetWorker.h"
#include "../public/ISL_IO.h"
#include <Mswsock.h>


void ISL_NET::ISL_NET_IOCP_Work::arsWork(ISL_Worker_Indicator& worker_info)
{
	OVERLAPPED				*pOverlapped = NULL;
	ISL_PER_SOCKET_CONTEXT  *pSocketCtx		 = NULL;
	DWORD					dwBytesTransfered = 0;

	ISL_NET_Work_Indicator *pWorkIndi = static_cast<ISL_NET::ISL_NET_Work_Indicator*>(&worker_info);
	while (!pWorkIndi->pAttachedWork->bEndWork)
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			pWorkIndi->pAttachedWork->hIOCompletionPort,
			&dwBytesTransfered,
			(PULONG_PTR)&pSocketCtx,
			&pOverlapped,
			INFINITE);

		// 如果收到的是退出标志，则直接退出
		if (ISL_NET_CODE_QUEUE_OUT == (int)pSocketCtx)
		{
			break;
		}

		// 判断是否出现了错误
		if (!bReturn)
		{
			DWORD dwErr = GetLastError();

			// 显示一下提示信息
			/*if (!pIOCPModel->HandleError(pIOCtx, dwErr))
			{
				break;
			}*/

			continue;
		}
		else
		{
			// 读取传入的参数
			ISL_PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, ISL_PER_IO_CONTEXT, _overlapped);
			// 判断是否有客户端断开了
			if ((0 == dwBytesTransfered) && (RECV_POSTED == pIoContext->_opType || SEND_POSTED == pIoContext->_opType))
			{
				//掉线了 Todo 掉线处理


				continue;
			}
			else
			{
				switch (pIoContext->_opType)
				{
				case ACCEPT_POSTED:
					pWorkIndi->pAttachedWork->_DoAccept(pSocketCtx, pIoContext);
					break;				
				case RECV_POSTED:
					pWorkIndi->pAttachedWork->_DoRecv(pSocketCtx, pIoContext);
					break;
				case SEND_POSTED:
					pWorkIndi->pAttachedWork->_DoSend(pSocketCtx, pIoContext);
					break;
				default:
					// 不应该执行到这里
					
					break;
				} 

			}




		}


	}


}

bool ISL_NET::ISL_NET_IOCP_Work::AssociateWithIOCP(ISL_PER_SOCKET_CONTEXT* pSocCTX)
{
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pSocCTX->sBindSocket, hIOCompletionPort, (DWORD)pSocCTX, 0);

	if (NULL == hTemp)
	{
		//this->_ShowMessage(("执行CreateIoCompletionPort()出现错误.错误代码：%d"), GetLastError());
		return false;
	}

	return true;
}

bool ISL_NET::ISL_NET_IOCP_Work::_PostAccept(ISL_PER_IO_CONTEXT* pIoContext)
{
	//预建立socket 加入map
	SOCKET preSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	ISL_PER_SOCKET_CONTEXT* pPreSocketCTX = new ISL_PER_SOCKET_CONTEXT();
	pPreSocketCTX->sBindSocket = preSocket;

	//Todo map访问互斥
	_mapSocketCtx.emplace(preSocket, pPreSocketCTX);

	DWORD dwBytes = 0;

	//异步接收
	AcceptEx(pIoContext->_socket, pPreSocketCTX->sBindSocket, &(pIoContext->_wsabuf.buf), pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &(pIoContext->_overlapped));
	

}


void ISL_NET::ISL_NET_IOCP_Work::_DoAccept(ISL_PER_SOCKET_CONTEXT* pSocketCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);

	GetAcceptExSockaddrs(pIoContext->_wsabuf.buf, pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), localLen + 16, remoteLen + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
		
	pSocketCtx->addr = *ClientAddr;
	
	//端口与socket绑定
	AssociateWithIOCP(pSocketCtx);

	//通知等待接受
	_PostRecv(&(pSocketCtx->recvData));


	pIoContext->ResetBuffer();
	//继续接受
	_PostAccept(pIoContext);
}


bool ISL_NET::ISL_NET_IOCP_Work::_PostSend(ISL_PER_IO_CONTEXT* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	// 初始化完成后，，投递WSARecv请求
	int nBytesRecv = WSASend(pIoContext->_socket,p_wbuf,1,&dwBytes,dwFlags,p_ol,NULL);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//this->_ShowMessage("投递第一个WSARecv失败！");
		return false;
	}
	return true;
}

void ISL_NET::ISL_NET_IOCP_Work::_DoSend(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext)
{

}


bool ISL_NET::ISL_NET_IOCP_Work::_PostRecv(ISL_PER_IO_CONTEXT* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	pIoContext->ResetBuffer();

	// 初始化完成后，，投递WSARecv请求
	int nBytesRecv = WSARecv(pIoContext->_socket, p_wbuf, 1, &(pIoContext->dwBytes), &dwFlags, p_ol, NULL);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//this->_ShowMessage("投递第一个WSARecv失败！");
		return false;
	}
	return true;
}



void ISL_NET::ISL_NET_IOCP_Work::_DoRecv(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	//处理消息




	//重新等待接受
	_PostRecv(pIoContext);
}

