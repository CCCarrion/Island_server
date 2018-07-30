#include <WinSock2.h>
#include <Ws2tcpip.h>

#include "../public/ConnectLayer.h"
#include "source/debugger/ISL_Debugger.h"

#pragma comment(lib,"ws2_32.lib")

using namespace ISL_NET;


ISL_RESULT_CODE ISL_Listener::CreateListener(const char* ipAddr, USHORT pport)
{
	

	//创建套接字  
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socket == INVALID_SOCKET)
	{
		return ISL_NET_ERR;
	}

	//绑定IP和端口  
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(pport);
	//sin.sin_addr.S_un.S_addr = INADDR_ANY;
	inet_pton(AF_INET, ipAddr, &sin.sin_addr);
	if (bind(_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		//printf("bind error !");

		return ISL_NET_ERR;
	}

	//开始监听  
	if (listen(_socket, 5) == SOCKET_ERROR)
	{
		//printf("listen error !");
		return ISL_NET_ERR;
	}


}

ISL_RESULT_CODE ISL_Listener::Tick()
{

	//循环接收数据  
	SOCKET sClient;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	while (true)
	{
		//printf("等待连接...\n");
		sClient = accept(_socket, (SOCKADDR *)&remoteAddr, &nAddrlen);
		if (sClient == INVALID_SOCKET)
		{
			//没有就退出
			break;
		}

		//接收数据  
		int ret = recv(sClient, _revData, 255, 0);
		if (ret > 0)
		{
			_revData[ret] = 0x00;
		}
		ISL_Logger(_revData);
		//发送数据  
		const char * sendData = "你好，TCP客户端！\n";
		send(sClient, sendData, strlen(sendData), 0);
		closesocket(sClient);
	}

	return ISL_OK;
}


//关闭socket
ISL_NET::ISL_Listener::~ISL_Listener()
{
	if (_socket)
	{
		closesocket(_socket);
	}	
}
