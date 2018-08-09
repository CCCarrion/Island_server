#pragma once

#include "source/debugger/ISL_DebugDef.h"
#include <stdlib.h>




namespace ISL_MSG
{

	struct ISL_MsgBase
	{
		unsigned int msgType;
		unsigned long msgBatch;
		char content[1024];
	};

	//解码 二进制变成 消息类型
	inline ISL_RESULT_CODE DecodeMSG(ISL_MsgBase* msgData,char* pBuf, DWORD dataLen)
	{

		*(pBuf + dataLen) = '\0';

		char* tempBuffer = new char[dataLen+64];

		char* cBatchID = strtok_s(pBuf, "|", &tempBuffer);
		char* cMsgType = strtok_s(NULL, "|", &tempBuffer);
		char* cMsgContent = strtok_s(NULL, "|", &tempBuffer);

		msgData->msgType = atoi(cMsgType);
		msgData->msgBatch = atol(cBatchID);
		strcpy_s(msgData->content, cMsgContent);


		delete[dataLen + 64] tempBuffer;
		return ISL_OK;
	};

	//编码 消息类型  转换 二进制
	inline ISL_RESULT_CODE EncodeMSG(ISL_MsgBase* msgData, char* pBuf, DWORD bufLen, DWORD* pDataLen)
	{

		char cBatchID[64];
		char cMsgType[64];
		char cMsgContent[1024];

		_itoa_s(msgData->msgType, cMsgType, 64);
		_ltoa_s(msgData->msgBatch, cBatchID, 64);
		strcpy_s(cMsgContent, cMsgContent);

		char* pSend = pBuf;

		char* cBatchID_t = cBatchID;
		char* cMsgType_t = cMsgType;
		char* cMsgContent_t = cMsgContent;

		(*pDataLen)++;

		while (*cBatchID != '\0')
		{
			*pSend = *cBatchID;
			cBatchID_t++;
			pSend++;
			(*pDataLen)++;
		}
		*pSend++ = '|';
		(*pDataLen)++;
		while (*cMsgType != '\0')
		{
			*pSend = *cMsgType;
			cMsgType_t++;
			pSend++;
			(*pDataLen)++;
		}
		*pSend++ = '|';
		(*pDataLen)++;
		while (*cMsgContent_t != '\0')
		{
			*pSend = *cMsgContent_t;
			cMsgContent_t++;
			pSend++;
			(*pDataLen)++;
		}


		*pSend = '\0';
		(*pDataLen)++;


		return ISL_OK;
	};












}