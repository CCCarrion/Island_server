
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

bool ISL_NET::ISL_NET_IOCP_Work::_PostAccept(ISL_PER_IO_CONTEXT* pIoContext)
{
	//Ԥ����socket ����map
	SOCKET preSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	ISL_PER_SOCKET_CONTEXT* pPreSocketCTX = new ISL_PER_SOCKET_CONTEXT();
	pPreSocketCTX->sBindSocket = preSocket;

	//Todo map���ʻ���
	_mapSocketCtx.emplace(preSocket, pPreSocketCTX);

	DWORD dwBytes = 0;

	//�첽����
	AcceptEx(pIoContext->_socket, pPreSocketCTX->sBindSocket, &(pIoContext->_wsabuf.buf), pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &(pIoContext->_overlapped));
	

}


void ISL_NET::ISL_NET_IOCP_Work::_DoAccept(ISL_PER_SOCKET_CONTEXT* pSocketCtx, ISL_PER_IO_CONTEXT* pIoContext)
{
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);

	GetAcceptExSockaddrs(pIoContext->_wsabuf.buf, pIoContext->_wsabuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2), localLen + 16, remoteLen + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
		
	pSocketCtx->addr = *ClientAddr;
	
	//�˿���socket��
	AssociateWithIOCP(pSocketCtx);

	//֪ͨ�ȴ�����
	_PostRecv(&(pSocketCtx->recvData));


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
	int nBytesRecv = WSASend(pIoContext->_socket,p_wbuf,1,&dwBytes,dwFlags,p_ol,NULL);

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

}


bool ISL_NET::ISL_NET_IOCP_Work::_PostRecv(ISL_PER_IO_CONTEXT* pIoContext)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	WSABUF *p_wbuf = &pIoContext->_wsabuf;
	OVERLAPPED *p_ol = &pIoContext->_overlapped;

	pIoContext->ResetBuffer();

	// ��ʼ����ɺ󣬣�Ͷ��WSARecv����
	int nBytesRecv = WSARecv(pIoContext->_socket, p_wbuf, 1, &(pIoContext->dwBytes), &dwFlags, p_ol, NULL);

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




	//���µȴ�����
	_PostRecv(pIoContext);
}

