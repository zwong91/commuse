/** 打印日志
*   @FileName  : CommLog.h
*   @Author    : Double Sword
*   @date      : 2011-8-28
*   @Copyright : Copyright belong to Author and ZhiZunNet.CO.
*   @RefDoc    : CommLogOpt.ini
				 1.为控制是否打印的文件，没有此文件不打印
				 2.[CommLog]
					Print = [0|1|2] 0-不打印 1-Debugview打印 2-文件打印
*/
#ifndef __CommLog_H
#define __CommLog_H

#define G_COM_PRINT_LOG COMMUSE::ComPrintLog::GetInstance()

#define LOGEVENW G_COM_PRINT_LOG.LogEventW
#define LOGEVENA G_COM_PRINT_LOG.LogEventA

#ifdef UNICODE
#	define LOGEVEN  LOGEVENW
#else
#	define LOGEVEN  LOGEVENA
#endif

namespace COMMUSE
{
	/** 
	*   @ClassName	: ComPrintLog
	*   @Purpose	: 管理日志
	*
	*
	*
	*
	*   @Author	: Double Sword
	*   @Data	: 2011-8-28
	*/
	class ComPrintLog
	{
	public:
		ComPrintLog();
		~ComPrintLog();
		//静态实例
		static ComPrintLog& GetInstance()
		{
			static ComPrintLog c;
			return c;
		}

		//ini控制打印
		void InitLogState();
		int GetLogState(){return _nLogState;}

		void LogEventW(wchar_t* format,...);
		void LogEventA(char* format,...);

		//-------------------------------------------------------------------------------
		//用debugview打印
		void LogEvenDW (wchar_t* message);
		void LogEvenDA (char* message);

		//用File打印
		void LogEvenFW (wchar_t* message);
		void LogEvenFA (char* message);
	private:
		HANDLE				_LogFile ;					//打印文件句柄
		char				_szLogFilePath[MAX_PATH];	//log文件路径
		char				_szAppFileName[MAX_PATH];	//应用程序名
		int					_nLogState;					//是否打印日志
		char				_szLogStateIniPath[MAX_PATH];//ini文件路径

		CRITICAL_SECTION	_cs_log;				//打印文件锁
		//初始化
		void PrintInit();
		//反初始化
		void PrintUnInit();
	};
}


#endif //__CommLog_H




