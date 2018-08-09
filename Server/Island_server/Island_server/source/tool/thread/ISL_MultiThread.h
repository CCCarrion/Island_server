#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>

namespace ISL_TOOL
{
	//工作线程指导
	class ISL_Worker_Indicator
	{

	public:
		int nThreadID;

		void(*fWork)(ISL_Worker_Indicator*);  //函数指针


		//todo：信号量通知继续工作和通知工作完成和结束
		std::mutex mtxWorkerState;
		bool bEndThread;
		std::condition_variable cvWaitNewWork;

	};


	//线程入口
	inline void _ThreadEntry(ISL_Worker_Indicator* worker_info)
	{

		while (!worker_info->bEndThread)
		{			
			worker_info->mtxWorkerState.lock();
			if (worker_info->fWork != nullptr)
			{
				//执行工作
				worker_info->fWork(worker_info);
			}
			worker_info->mtxWorkerState.unlock();
			std::unique_lock<std::mutex> lk(worker_info->mtxWorkerState);
			worker_info->cvWaitNewWork.wait(lk);
		}
	}

	//建立线程
	inline std::thread _CreateWorkerThread(ISL_Worker_Indicator* worker_info)
	{
		return std::thread(_ThreadEntry, worker_info);
	}


	
}