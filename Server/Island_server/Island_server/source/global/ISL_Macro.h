#pragma once



//如果出错，错误码向上传递
#define CHECKRST_FAILRETURN(rst_val)\
											if(rst_val != ISL_OK)\
												 return rst_val;

#define AAA(x) x++