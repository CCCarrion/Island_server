#include <WinSock2.h>
#include <Ws2tcpip.h>

#include "../public/ConnectLayer.h"
#include "source/debugger/ISL_Debugger.h"

#pragma comment(lib,"ws2_32.lib")

using namespace ISL_NET;


ISL_RESULT_CODE ISL_Listener::CreateListener(const char* ipAddr, USHORT pport)
{
	

	//�����׽���  
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socket == INVALID_SOCKET)
	{
		return ISL_NET_ERR;
	}

	//��IP�Ͷ˿�  
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

	//��ʼ����  
	if (listen(_socket, 5) == SOCKET_ERROR)
	{
		//printf("listen error !");
		return ISL_NET_ERR;
	}

}

ISL_RESULT_CODE ISL_Listener::Tick()
{

	//ѭ����������  
	SOCKET sClient;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);

	//printf("�ȴ�����...\n");
	sClient = accept(_socket, (SOCKADDR *)&remoteAddr, &nAddrlen);
	_cnnSocketList.push_back(sClient);

   
	
	for(SOCKET soc : _cnnSocketList)
	{
		//��������  
		int ret = recv(soc, _revData, 255, 0);
		if (ret > 0)
		{
			_revData[ret] = 0x00;
		}
		ISL_Logger(_revData);

		//��ʱ����
		//����ת����Ϣ
		char* tempBuffer = _tempBuffer;
		char* cBatchID = strtok_s(_revData, "|", &tempBuffer);
		char* cMsgType = strtok_s(NULL, "|", &tempBuffer);
		char* cMsgContent = strtok_s(NULL, "|", &tempBuffer);

		char* pSend = _sendBuffer;

		const char* cServerRespond = "Server has Get Your Msg";
		while (*cBatchID != '\0')
		{
			*pSend = *cBatchID;
			cBatchID++;
			pSend++;
		}
		*pSend++ = '|';
		while (*cMsgType != '\0')
		{
			*pSend = *cMsgType;
			cMsgType++;
			pSend++;
		}
		*pSend++ = '|';
		while (*cServerRespond != '\0')
		{
			*pSend = *cServerRespond;
			cServerRespond++;
			pSend++;
		}

		*pSend = '\0';


		//��������  
		send(soc, _sendBuffer, strlen(_sendBuffer), 0);
		//closesocket(sClient);
	
	}

	return ISL_OK;
}


//�ر�socket
ISL_NET::ISL_Listener::~ISL_Listener()
{
	if (_socket)
	{
		closesocket(_socket);
	}	
}
