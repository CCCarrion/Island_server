#pragma once



namespace ISL_HELPER
{
	template<class T>
	class ISL_Instance
	{
	public:
		static T* GetInstance()
		{
			static T* _pInstance = new T();
			return _pInstance;
		}
	};
}