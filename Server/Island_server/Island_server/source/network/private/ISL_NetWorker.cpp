
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

		// ����յ������˳���־����ֱ���˳�
		if (ISL_NET_CODE_QUEUE_OUT == (int)pSocketCtx)
		{
			break;
		}

		// �ж��Ƿ�����˴���
		if (!bReturn)
		{
			DWORD dwErr = GetLastError();

			// ��ʾһ����ʾ��Ϣ
			/*if (!pIOCPModel->HandleError(pIOCtx, dwErr))
			{
				break;
			}*/

			continue;
		}
		else
		{
			// ��ȡ����Ĳ���
			ISL_PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, ISL_PER_IO_CONTEXT, _overlapped);
			// �ж��Ƿ��пͻ��˶Ͽ���
			if ((0 == dwBytesTransfered) && (RECV_POSTED == pIoContext->_opType || SEND_POSTED == pIoContext->_opType))
			{
				//������ Todo ���ߴ���


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
					// ��Ӧ��ִ�е�����
					
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
		//��ȡ���ö˿ڳ���

		return ISL_NET_ERR;
	}



	return ISL_OK;

}

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::StartWork(USHORT port)
{

	_listenerCtx.sBindSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _listenerCtx.sBindSocket)
	{
		//�����˿�ʧ��
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

	// ��ʼ���м���
	if (SOCKET_ERROR == listen(_listenerCtx.sBindSocket, SOMAXCONN))
	{
		//
		return ISL_NET_ERR;
	}

	//��ʼ���ؼ�����

	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	// ʹ��AcceptEx��������Ϊ���������WinSock2�淶֮���΢�������ṩ����չ����
	// ������Ҫ�����ȡһ�º�����ָ�룬
	// ��ȡAcceptEx����ָ��
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

	// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��
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
	//Todo  ���Accept
	_PostAccept(&_listenerCtx.ioCtxReuse);


	return ISL_OK;

}


void ISL_NET::ISL_NET_IOCP_Work::SendData(SOCKET skt, byte* datBufr, DWORD datlen)
{

	auto itKV = _mapSocketCtx.find(skt);

	if (itKV->first != skt)
	{
		//û���ҵ� 

		return;
	}

	ISL_PER_SOCKET_CONTEXT* pSocCtx = itKV->second;


	ISL_PER_IO_CONTEXT*  pIoCtx = pSocCtx->GetNewIoContext();
	
	//Ͷ�ݷ���
	_PostSend(pIoCtx);


}

bool ISL_NET::ISL_NET_IOCP_Work::AssociateWithIOCP(ISL_PER_SOCKET_CONTEXT* pSocCTX)
{
	// �����ںͿͻ���ͨ�ŵ�SOCKET�󶨵���ɶ˿���
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pSocCTX->sBindSocket, hIOCompletionPort, (DWORD)pSocCTX, 0);

	if (NULL == hTemp)
	{
		//this->_ShowMessage(("ִ��CreateIoCompletionPort()���ִ���.������룺%d"), GetLastError());
		return false;
	}

	return true;
}

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::EndWork()
{
	//֪ͨ��������

	bEndWork = true;
	
	for (int i = 0; i < ISL_NetManager::GetInstance()->GetNetWorkerNum(); i++)
	{
		// ֪ͨ���е���ɶ˿ڲ����˳�
		PostQueuedCompletionStatus(hIOCompletionPort, 0, (DWORD)ISL_NET_CODE_QUEUE_OUT, NULL);
	}

	//�رռ���socket
	closesocket(_listenerCtx.sBindSocket);

	//ToDo��������Socket��Ϣ


	return ISL_OK;
}

ISL_RESULT_CODE ISL_NET::ISL_NET_IOCP_Work::DeInitWork()
{
	

	// �ر�IOCP���
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
	//Ԥ����socket ����map
	SOCKET preSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	ISL_PER_SOCKET_CONTEXT* pPreSocketCTX = new ISL_PER_SOCKET_CONTEXT();
	pPreSocketCTX->sBindSocket = preSocket;


	DWORD dwBytes = 0;

	//�첽����
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
	//ע������
	_mapSocketCtx.emplace(++_curMaxCONN, pSocketCtx);
	pSocketCtx->nBindConnID = _curMaxCONN;

	//�˿���socket��
	AssociateWithIOCP(pSocketCtx);



	//֪ͨ�ȴ�����
	_PostRecv(&(pSocketCtx->ioCtxReuse));


	pIoContext->ResetBuffer();
	//��������
	_PostAccept(pIoContext);

}


bool ISL_NET::ISL_NET_IOCP_Work::_PostSend(ISL_PER_IO_CONTEXT* pIoContext)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	// ��ʼ����ɺ󣬣�Ͷ��WSARecv����
	int nBytesRecv = WSASend(pIoContext->_socket,p_wbuf,1,&(pIoContext->_dwBytes),dwFlags,p_ol,NULL);

	// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//this->_ShowMessage("Ͷ�ݵ�һ��WSARecvʧ�ܣ�");
		return false;
	}
	return true;
}

void ISL_NET::ISL_NET_IOCP_Work::_DoSend(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	//����Ϣ�Ƴ�
	pSockerCtx->RemoveIoCtx(pIoContext);

}


bool ISL_NET::ISL_NET_IOCP_Work::_PostRecv(ISL_PER_IO_CONTEXT* pIoContext)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	pIoContext->ResetBuffer();

	// ��ʼ����ɺ󣬣�Ͷ��WSARecv����
	int nBytesRecv = WSARecv(pIoContext->_socket, p_wbuf, 1, &(pIoContext->_dwBytes), &dwFlags, p_ol, NULL);

	// �������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//this->_ShowMessage("Ͷ�ݵ�һ��WSARecvʧ�ܣ�");
		return false;
	}
	return true;
}



void ISL_NET::ISL_NET_IOCP_Work::_DoRecv(ISL_PER_SOCKET_CONTEXT* pSockerCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	//������Ϣ
	ISL_NetManager* pNetMgr =  ISL_NetManager::GetInstance();
	
	pNetMgr->RecvMSG(pSockerCtx->nBindConnID, pIoContext->_wsabuf.buf, pIoContext->_dwBytes);

	//���µȴ�����
	_PostRecv(pIoContext);
}

