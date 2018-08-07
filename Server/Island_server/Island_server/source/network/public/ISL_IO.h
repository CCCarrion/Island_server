#pragma once

#include <list>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

#define MAX_BUFFER_LEN 8192
#define SOCKET_WAIT_SEND_COUNT_PRESET 64


namespace ISL_NET
{
	// 在完成端口上投递的I/O操作的类型
	enum ISL_NET_OpType
	{
		ACCEPT_POSTED,                     // 标志投递的Accept操作
		SEND_POSTED,                       // 标志投递的是发送操作
		RECV_POSTED,                       // 标志投递的是接收操作
		NULL_POSTED                        // 用于初始化，无意义
	};

	//IO重叠结构
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

		// 重置缓冲区内容
		void ResetBuffer()
		{
			ZeroMemory(_szBuffer, MAX_BUFFER_LEN);
		}
	};

	struct ISL_PER_SOCKET_CONTEXT
	{
		SOCKET			sBindSocket;
		SOCKADDR_IN		addr;

		ISL_PER_IO_CONTEXT recvData;
		std::list<ISL_PER_IO_CONTEXT*> listSendData;
		
		ISL_PER_SOCKET_CONTEXT()
			:listSendData(SOCKET_WAIT_SEND_COUNT_PRESET)
		{		
			recvData._opType = RECV_POSTED;
			recvData._socket = sBindSocket;
		}

		~ISL_PER_SOCKET_CONTEXT()
		{
			closesocket(sBindSocket);

			for (ISL_PER_IO_CONTEXT* pctx:listSendData)
			{
				delete pctx;
			}
			listSendData.clear();
		}


		// 添加一个新的IoContext
		ISL_PER_IO_CONTEXT* GetNewIoContext()
		{
			ISL_PER_IO_CONTEXT* p = new ISL_PER_IO_CONTEXT;

			listSendData.push_back(p);
			return p;
		}

		// 取出并移除IoContext
		ISL_PER_IO_CONTEXT* GetIoContext()
		{
			ISL_PER_IO_CONTEXT* p = listSendData.front();
			listSendData.pop_front();
			return p;
		}

		



	};




	
	LPFN_ACCEPTEX                m_lpfnAcceptEx;                // AcceptEx 和 GetAcceptExSockaddrs 的函数指针，用于调用这两个扩展函数
	LPFN_GETACCEPTEXSOCKADDRS    m_lpfnGetAcceptExSockAddrs;
}