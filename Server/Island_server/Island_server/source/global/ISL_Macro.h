#pragma once



//����������������ϴ���
#define CHECKRST_FAILRETURN(rst_val)\
											if(rst_val != ISL_OK)\
												 return rst_val;

#define AAA(x) x++