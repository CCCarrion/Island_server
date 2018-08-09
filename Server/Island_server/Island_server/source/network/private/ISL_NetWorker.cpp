
#include "../ISL_NetDef.h"
#include "../public/ISL_NetWorker.h"
#include "../NetManager.h"
#include <Mswsock.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS


void ISL_NET::ISL_NET_IOCP_Work::arsWork(ISL_Worker_Indicator* worker_info)
{
	OVERLAPPED				*pOverlapped = NULL;
	ISL_PER_SOCKET_CONTEXT  *pSocketCtx		 = NULL;
	DWORD					dwBytesTransfered = 0;

	ISL_NET_Work_Indicator *pWorkIndi = static_cast<ISL_NET::ISL_NET_Work_Indicator*>(worker_info);
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

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::InitWork()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return ISL_NET_ERR;
	}

	hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);


	if (hIOCompletionPort == NULL)
	{
		//获取复用端口出错

		return ISL_NET_ERR;
	}



	return ISL_OK;

}

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::StartWork(USHORT port)
{

	_listenerCtx.sBindSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _listenerCtx.sBindSocket)
	{
		//创建端口失败
		return ISL_NET_ERR;
	}



	SOCKADDR_IN localAddr;

	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(port);
	//char ipAddr[255];
	//gethostname(ipAddr, 255);
	//struct hostent FAR* lpHostEnt = getaddrinfo(ipAddr,);
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	//{
	//	inet_pton(AF_INET, lpHostEnt->h_addr_list[0], &localAddr.sin_addr);
	//	//localAddr.sin_addr.s_addr = inet_addr(lpHostEnt->h_addr_list[0]);
	//}


	if (bind(_listenerCtx.sBindSocket, (LPSOCKADDR)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
	{
		//printf("bind error !");

		return ISL_NET_ERR;
	}

	// 开始进行监听
	if (SOCKET_ERROR == listen(_listenerCtx.sBindSocket, SOMAXCONN))
	{
		//
		return ISL_NET_ERR;
	}

	//初始化关键函数

	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
	// 所以需要额外获取一下函数的指针，
	// 获取AcceptEx函数指针
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		_listenerCtx.sBindSocket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&_lpfnAcceptEx,
		sizeof(_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		return ISL_NET_ERR;
	}

	// 获取GetAcceptExSockAddrs函数指针，也是同理
	if (SOCKET_ERROR == WSAIoctl(
		_listenerCtx.sBindSocket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs),
		&_lpfnGetAcceptExSockAddrs,
		sizeof(_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{

		return ISL_NET_ERR;
	}




	_listenerCtx.ioCtxReuse._opType = ACCEPT_POSTED;

	_curMaxCONN = 0;
	//Todo  多个Accept
	_PostAccept(&_listenerCtx.ioCtxReuse);


	return ISL_OK;

}


void ISL_NET::ISL_NET_IOCP_Work::SendData(SOCKET skt, byte* datBufr, DWORD datlen)
{

	auto itKV = _mapSocketCtx.find(skt);

	if (itKV->first != skt)
	{
		//没有找到 

		return;
	}

	ISL_PER_SOCKET_CONTEXT* pSocCtx = itKV->second;


	ISL_PER_IO_CONTEXT*  pIoCtx = pSocCtx->GetNewIoContext();
	
	//投递发送
	_PostSend(pIoCtx);


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

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::EndWork()
{
	//通知工作结束

	bEndWork = true;
	
	for (int i = 0; i < ISL_NetManager::GetInstance()->GetNetWorkerNum(); i++)
	{
		// 通知所有的完成端口操作退出
		PostQueuedCompletionStatus(hIOCompletionPort, 0, (DWORD)ISL_NET_CODE_QUEUE_OUT, NULL);
	}

	//关闭监听socket
	closesocket(_listenerCtx.sBindSocket);

	//ToDo清除缓存的Socket信息


	return ISL_OK;
}

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::DeInitWork()
{
	

	// 关闭IOCP句柄
	CloseHandle(hIOCompletionPort);

	WSACleanup();


	return ISL_OK;
}

ISL_NET::ISL_PER_IO_CONTEXT* ISL_NET::ISL_NET_IOCP_Work::CreateIOCtx(T_CONN_ID connID)
{
	auto itFind = _mapSocketCtx.find(connID);

	if (itFind == _mapSocketCtx.end())
	{
		return nullptr;
	}
	else
	{
		return itFind->second->GetNewIoContext();
	}
}

bool ISL_NET::ISL_NET_IOCP_Work::_PostAccept(ISL_PER_IO_CONTEXT* pIoContext)
{
	//预建立socket 加入map
	SOCKET preSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	ISL_PER_SOCKET_CONTEXT* pPreSocketCTX = new ISL_PER_SOCKET_CONTEXT();
	pPreSocketCTX->sBindSocket = preSocket;


	DWORD dwBytes = 0;

	//异步接收
	_lpfnAcceptEx(pIoContext->_socket, pPreSocketCTX->sBindSocket, &(pIoContext->_wsabuf.buf), pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &(pIoContext->_overlapped));
	

	return ISL_OK;
}


void ISL_NET::ISL_NET_IOCP_Work::_DoAccept(ISL_PER_SOCKET_CONTEXT* pSocketCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);

	_lpfnGetAcceptExSockAddrs(pIoContext->_wsabuf.buf, pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), localLen + 16, remoteLen + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
		
	pSocketCtx->addr = *ClientAddr;
	//注册链接
	_mapSocketCtx.emplace(++_curMaxCONN, pSocketCtx);
	pSocketCtx->nBindConnID = _curMaxCONN;

	//端口与socket绑定
	AssociateWithIOCP(pSocketCtx);



	//通知等待接受
	_PostRecv(&(pSocketCtx->ioCtxReuse));


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
	int nBytesRecv = WSASend(pIoContext->_socket,p_wbuf,1,&(pIoContext->_dwBytes),dwFlags,p_ol,NULL);

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
	//把消息移除
	pSockerCtx->RemoveIoCtx(pIoContext);

}


bool ISL_NET::ISL_NET_IOCP_Work::_PostRecv(ISL_PER_IO_CONTEXT* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	pIoContext->ResetBuffer();

	// 初始化完成后，，投递WSARecv请求
	int nBytesRecv = WSARecv(pIoContext->_socket, p_wbuf, 1, &(pIoContext->_dwBytes), &dwFlags, p_ol, NULL);

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
	ISL_NetManager* pNetMgr =  ISL_NetManager::GetInstance();
	
	pNetMgr->RecvMSG(pSockerCtx->nBindConnID, pIoContext->_wsabuf.buf, pIoContext->_dwBytes);

	//重新等待接受
	_PostRecv(pIoContext);
}

