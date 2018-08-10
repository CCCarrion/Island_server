#include "../NetManager.h"
#include "source/global/ISL_Macro.h"
#include "source/debugger/ISL_Debugger.h"

using namespace ISL_MSG;

ISL_RESULT_CODE ISL_NET::ISL_NetManager::InitNetwork()
{
	_netWork.InitWork();


	//获取核心数量
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	_maxWorkersNum = si.dwNumberOfProcessors;
	


	_arsWorkers = new std::thread[_maxWorkersNum];
	_workerIndicators = new ISL_NET_Work_Indicator[_maxWorkersNum];


	for (int i = 0; i < _maxWorkersNum; i++)
	{
		(_workerIndicators + i)->nThreadID = i;
		(_workerIndicators + i)->bEndThread = false;

		(_workerIndicators + i)->fWork = nullptr;

		_arsWorkers[i]= ISL_TOOL::_CreateWorkerThread(_workerIndicators + i);
	}




	




	return ISL_OK;
}

ISL_RESULT_CODE ISL_NET::ISL_NetManager::Start()
{

	//Todo 通过配置获取端口
	USHORT port = 6666;
	_netWork.StartWork(port);

	for (int i = 0; i < _maxWorkersNum; i++)
	{		
		(_workerIndicators + i)->fWork = ISL_NET_IOCP_Work::arsWork;
		(_workerIndicators + i)->pAttachedWork = &_netWork;

		(_workerIndicators + i)->cvWaitNewWork.notify_all();
	}

	return ISL_OK;
}


//还没想好需要做什么
ISL_RESULT_CODE ISL_NET::ISL_NetManager::Tick()
{
	ISL_RESULT_CODE rst = ISL_OK;

	



	return ISL_OK;
}


ISL_RESULT_CODE ISL_NET::ISL_NetManager::Stop()
{
	_netWork.EndWork();

	for (int i = 0; i < _maxWorkersNum; i++)
	{
		(_workerIndicators + i)->fWork = nullptr;
	}

	return ISL_OK;
}




ISL_RESULT_CODE ISL_NET::ISL_NetManager::SendMSG(T_CONN_ID connID, ISL_MsgBase* pMSG)
{
	ISL_PER_IO_CONTEXT* pSendBfr = _netWork.CreateIOCtx(connID);

	ISL_MSG::EncodeMSG(pMSG, pSendBfr->_wsabuf.buf, pSendBfr->_wsabuf.len, &(pSendBfr->_dwBytes));

	_netWork._PostSend(pSendBfr);
	return ISL_OK;
}


ISL_RESULT_CODE ISL_NET::ISL_NetManager::RecvMSG(T_CONN_ID connID, char* data, DWORD dataLen)
{
	ISL_MsgBase* recvMsg = new ISL_MsgBase();
	ISL_MSG::DecodeMSG(recvMsg, data, dataLen);

	//打印消息
	std::cout << connID << std::endl;
	std::cout << recvMsg->msgBatch << std::endl;
	std::cout << recvMsg->msgType << std::endl;
	std::cout << recvMsg->content << std::endl;


	//把消息传递出去进行处理

	const char* cRespond = "Have Receive MSG: ";

	int nStrlen = strlen(cRespond) + strlen(recvMsg->content) + 1;

	char temp[1024];

	strcpy_s(temp, recvMsg->content);
	strcpy_s(recvMsg->content, cRespond);
	char* tpos = recvMsg->content + strlen(cRespond);
	strcpy_s(tpos,1024, temp);

	//此时测试 把数据原路返回
	SendMSG(connID, recvMsg);

	delete recvMsg;

	return ISL_OK;
}

ISL_NET::ISL_NetManager::~ISL_NetManager()
{
	for (int i = 0; i < _maxWorkersNum; i++)
	{
		(_workerIndicators + i)->bEndThread = false;

		(_workerIndicators + i)->cvWaitNewWork.notify_all();
	}
	for (int i = 0; i < _maxWorkersNum; i++)
	{
		_arsWorkers[i].join();
	}



	_netWork.DeInitWork();

	delete[] _arsWorkers;
	delete[] _workerIndicators;
}