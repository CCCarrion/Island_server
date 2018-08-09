#pragma once

#include <WinSock2.h>
#include <Mswsock.h>

#include <vector>
#include <algorithm>

#define MAX_BUFFER_LEN 8192
#define SOCKET_WAIT_SEND_COUNT_PRESET 64



namespace ISL_NET
{
	// ����ɶ˿���Ͷ�ݵ�I/O����������
	enum ISL_NET_OpType
	{
		ACCEPT_POSTED,                     // ��־Ͷ�ݵ�Accept����
		SEND_POSTED,                       // ��־Ͷ�ݵ��Ƿ��Ͳ���
		RECV_POSTED,                       // ��־Ͷ�ݵ��ǽ��ղ���
		NULL_POSTED                        // ���ڳ�ʼ����������
	};

	//IO�ص��ṹ
	struct ISL_PER_IO_CONTEXT
	{
		OVERLAPPED		_overlapped;
		SOCKET			_socket;
		WSABUF			_wsabuf;
		char			_szBuffer[MAX_BUFFER_LEN];
		DWORD			_dwBytes;
		ISL_NET_OpType	_opType;


		ISL_PER_IO_CONTEXT()
		{
			ZeroMemory(&_overlapped, sizeof(_overlapped));
			ZeroMemory(_szBuffer, MAX_BUFFER_LEN);
			_socket = INVALID_SOCKET;
			_wsabuf.buf = _szBuffer;
			_wsabuf.len = MAX_BUFFER_LEN;
			_opType = NULL_POSTED;
		}

		// ���û���������
		void ResetBuffer()
		{
			ZeroMemory(_szBuffer, MAX_BUFFER_LEN);
		}
	};

	struct ISL_PER_SOCKET_CONTEXT
	{
		T_CONN_ID		nBindConnID;
		SOCKET			sBindSocket;
		SOCKADDR_IN		addr;

		ISL_PER_IO_CONTEXT ioCtxReuse;
		std::vector<ISL_PER_IO_CONTEXT*> vecSendData;
		
		ISL_PER_SOCKET_CONTEXT()
			:vecSendData(SOCKET_WAIT_SEND_COUNT_PRESET)
		{		
			ioCtxReuse._opType = RECV_POSTED;
			ioCtxReuse._socket = sBindSocket;
		}

		~ISL_PER_SOCKET_CONTEXT()
		{
			closesocket(sBindSocket);

			for (ISL_PER_IO_CONTEXT* pctx:vecSendData)
			{
				delete pctx;
			}
			vecSendData.clear();
		}


		// ���һ���µ�IoContext
		ISL_PER_IO_CONTEXT* GetNewIoContext()
		{
			ISL_PER_IO_CONTEXT* p = new ISL_PER_IO_CONTEXT();
			p->_socket = sBindSocket;
			p->_opType = SEND_POSTED;

			vecSendData.push_back(p);
			return p;
		}

		//// ȡ�����Ƴ�IoContext
		//ISL_PER_IO_CONTEXT* GetIoContext()
		//{
		//	ISL_PER_IO_CONTEXT* p = listSendData.front();
		//	listSendData.pop_front();
		//	return p;
		//}

		bool RemoveIoCtx(ISL_PER_IO_CONTEXT* pIoCtx)
		{
			if (pIoCtx->_socket != sBindSocket)
			{
				//socket��һ��

				return false;
			}

			std::vector<ISL_PER_IO_CONTEXT*>::iterator itEnd = vecSendData.end();

			auto itIoCtx = std::remove(vecSendData.begin(), itEnd, pIoCtx);

			if (itIoCtx == itEnd)
			{
				//û���ҵ�
				return false;
			}

			//ɾ�� �ͷ��ڴ�
			delete *itIoCtx;

			vecSendData.erase(itIoCtx, itEnd);
			
			return true;
		}



	};


	
	
}