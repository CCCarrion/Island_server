#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>

namespace ISL_TOOL
{
	//�����߳�ָ��
	class ISL_Worker_Indicator
	{

	public:
		int nThreadID;

		void(*fWork)(ISL_Worker_Indicator*);  //����ָ��


		//todo���ź���֪ͨ����������֪ͨ������ɺͽ���
		std::mutex mtxWorkerState;
		bool bEndThread;
		std::condition_variable cvWaitNewWork;

	};


	//�߳����
	inline void _ThreadEntry(ISL_Worker_Indicator* worker_info)
	{

		while (!worker_info->bEndThread)
		{			
			worker_info->mtxWorkerState.lock();
			if (worker_info->fWork != nullptr)
			{
				//ִ�й���
				worker_info->fWork(worker_info);
			}
			worker_info->mtxWorkerState.unlock();
			std::unique_lock<std::mutex> lk(worker_info->mtxWorkerState);
			worker_info->cvWaitNewWork.wait(lk);
		}
	}

	//�����߳�
	inline std::thread _CreateWorkerThread(ISL_Worker_Indicator* worker_info)
	{
		return std::thread(_ThreadEntry, worker_info);
	}


	
}