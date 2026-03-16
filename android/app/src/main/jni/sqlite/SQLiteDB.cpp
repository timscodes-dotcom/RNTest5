#ifdef _WINDOWS
#include "stdafx.h"
#endif
#include <stdlib.h>
#include "SQLiteDB.h"

std::string SqliteEncode(const char *src, int len)
{
	std::string dst = "";
   size_t n = 0;
   for (int i=0; i<len; i++)
   {
      if (src[i] == '\'')
      {
         dst += '\'';
      }
      dst += src[i];
   }

   return dst;
}

SQLiteDB::SQLiteDB(void)
{
   m_Connection = NULL;
  m_batchSqls = "";
  m_batchCount = 0;
   InitializeCriticalSection(&mCS);
}

SQLiteDB::~SQLiteDB(void)
{
	EnterCriticalSection(&mCS);
	Close();
	LeaveCriticalSection(&mCS);

   DeleteCriticalSection(&mCS);
   
}

int SQLiteDB::Open(char *dbname, char *err_msg, bool needInit, sqlite3 **dbconn)
{
   int ret = SQLITE_OK;
	//char msg[2048];
   //char err_msg[2048];
   strcpy(err_msg, "");
   sqlite3 ** conn = NULL;

   if (dbconn != NULL)
      conn = dbconn;
   else
      conn = &m_Connection;

   if (*conn != NULL)
   {
      return SQLITE_OK;
   }

   strncpy(m_dbname, dbname, sizeof(m_dbname));
   ret = sqlite3_open(dbname, conn);

   if (ret != SQLITE_OK)
   {
      sprintf(err_msg, "sqlite3_open(%s) failed:%d", dbname, ret);
      //ExtServer->LogsOut(CmdErr,"plugin",msg);
      return ret;
   }

   // enable shared cache
   ret = sqlite3_enable_shared_cache(1);
   if (ret != SQLITE_OK)
   {
      sprintf(err_msg, "sqlite3_enable_shared_cache(1) failed:%d", ret);
      //ExtServer->LogsOut(CmdErr,"plugin",msg);
   }

   // default synchronous = FULL, the SQLite database engine will use the xSync method of the VFS to 
   //    ensure that all content is safely written to the disk surface prior to continuing. This ensures 
   //    that an operating system crash or power failure will not corrupt the database. FULL synchronous 
   //    is very safe, but it is also slower.
   // synchronous = NORMAL, the SQLite database engine will still sync at the most critical moments, but 
   //    less often than in FULL mode. There is a very small (though non-zero) chance that a power failure 
   //    at just the wrong time could corrupt the database in NORMAL mode. But in practice, you are more 
   //    likely to suffer a catastrophic disk failure or some other unrecoverable hardware fault. 
   // synchronous = OFF, SQLite continues without syncing as soon as it has handed data off to the operating 
   //    system. If the application running SQLite crashes, the data will be safe, but the database might 
   //    become corrupted if the operating system crashes or the computer loses power before that data has 
   //    been written to the disk surface. On the other hand, some operations are as much as 50 or more times 
   //    faster with synchronous OFF.
   sqlite3_exec(*conn, "PRAGMA synchronous = OFF ; ", 0, 0, 0);

   sqlite3_busy_timeout(*conn, 10);

   //sprintf(err_msg, "sqlite3_open(%s) ok", dbname);

   //ExtServer->LogsOut(CmdTrade,"plugin",msg);


   /*
   ret = sqlite3_exec(*conn, g_create_sql, NULL, NULL, (char **)&err_msg);
   if (ret == SQLITE_OK)
   {
      LocalLog("SQLiteDB::Open, sql=[%s]\n", g_create_sql);
      sprintf(msg, "%s: check table ok", ExtProgramName);
      ExtServer->LogsOut(CmdTrade,"plugin",msg);
   }
   else
   {
      sprintf(msg, "%s: check table failed:%d:%s", ExtProgramName, ret, err_msg);
      ExtServer->LogsOut(CmdErr,"plugin",msg);
      Close();
   }
   */
   return ret;
}

int SQLiteDB::Close()
{
   if (m_Connection != NULL)
   {
      sqlite3_close(m_Connection);
      m_Connection = NULL;
   }

   return 0;
}

int SQLiteDB::CompleteExecBatch(char *err_msg)
{
  int ret = SQLITE_OK;
  EnterCriticalSection(&mCS);
  if (m_batchSqls != "")
  {
    ret = _Exec(m_batchSqls.c_str(), err_msg);
    m_batchSqls = "";
    m_batchCount = 0;
  }
  LeaveCriticalSection(&mCS);
  return ret;
}

int SQLiteDB::Exec(const char *sql, char *err_msg, bool batchSave)
{
    if (batchSave == true)
   {
     EnterCriticalSection(&mCS);
     m_batchSqls += sql;
     m_batchCount++;
     
     if (m_batchCount == 50)
     {
       int ret = _Exec(m_batchSqls.c_str(), err_msg);
       m_batchSqls = "";
       m_batchCount = 0;
       LeaveCriticalSection(&mCS);
       return ret;
     }
     LeaveCriticalSection(&mCS);
     
     return SQLITE_OK;
   }
  
  EnterCriticalSection(&mCS);
  int ret = _Exec(sql, err_msg);
  LeaveCriticalSection(&mCS);
  return ret;
}

int SQLiteDB::_Exec(const char *sql, char *err_msg)
{
   int ret;

   //EnterCriticalSection(&mCS);
  
  for (int i=0; i<20; i++)
   {
      if ((ret=Open(m_dbname, err_msg)) != SQLITE_OK)
      {
         sprintf(err_msg, "Exec, Call Open failed");
         //ExtServer->LogsOut(CmdErr,"plugin",msg);

		 //LeaveCriticalSection(&mCS);
         return ret;
      }

      char *zErrMsg = 0;
      ret = sqlite3_exec(m_Connection, sql, NULL, NULL, &zErrMsg);
      if (ret == SQLITE_OK) {
    	  strcpy(err_msg, "");
    	  break;
      }
      if (zErrMsg)
        strcpy(err_msg, zErrMsg);
      else
          sprintf(err_msg, "err ret:%d", ret);
      //printf("_Exec: %d, %s\n", ret, zErrMsg);
     
      //sprintf(msg, "Exec %d failed:%d:%s", i, ret, err_msg);
      //ExtServer->LogsOut(CmdErr,"plugin",msg);
      if ((i%5) == 4)
         Close();
      //Sleep(1);
   }

   //LeaveCriticalSection(&mCS);

   return ret;
}

int SQLiteDB::Select(char *sql, int *nRow, char *err)
{
   int ret;
   char ** dbResult;
   int nColumn;

   for (int i=0; i<2; i++)
   {
      if ((ret=Open(m_dbname, err)) != SQLITE_OK)
      {
         sprintf(err, "Select, Call Open failed");
         //ExtServer->LogsOut(CmdErr,"plugin",msg);
         return ret;
      }

      char * err_msg = NULL;
      ret = sqlite3_get_table(m_Connection, sql, &dbResult, nRow, &nColumn, &err_msg);
      sqlite3_free_table(dbResult);
      if (ret == SQLITE_OK) {
    	  strcpy(err, "");
    	  break;
      }

      sprintf(err, "Select %d failed:%d:%s", i, ret, err_msg);
      //ExtServer->LogsOut(CmdErr,"plugin",msg);
      Close();
   }

   return ret;
}

char *SQLiteDB::SQLEncode(char *dst, char *src, int len)
{
   size_t n = 0;
   for (int i=0; i<len; i++)
   {
      if (src[i] == '\'')
      {
         dst[n++] = '\'';
      }
      dst[n++] = src[i];
   }
   
   return dst;
}

int SQLiteDB::Select(char *sql, Json::Value & jRet, char *err_msg) {
  int ret;
  std::string val = "";
  //char msg[2048];
  char ** dbResult;
  int nColumn, nRow = 0;

  for (int i=0; i<2; i++)
  {
    if ((ret=Open(m_dbname, err_msg)) != SQLITE_OK)
    {
      sprintf(err_msg, "failed to open db[1]");
      return -1;
    }

    char * err_msg1 = NULL;
    ret = sqlite3_get_table(m_Connection, sql, &dbResult, &nRow, &nColumn, &err_msg1);
    if (ret == SQLITE_OK)
    {
      Json::Reader reader;
		reader.parse("[]", jRet);

      std::string * colName = new std::string[nColumn];
      int nIndex = 0;
      for (int i1 = 0; i1 <= nRow; i1++) {
        Json::Value jRow;
        for (int i2 = 0; i2 < nColumn; i2++) {
          if (dbResult[nIndex] == NULL) {
            nIndex++;
            continue;
          }
          val = dbResult[nIndex];
          nIndex++;
          if (i1 == 0) {
            colName[i2] = val;
          }
          else {
            jRow[colName[i2]] = val;
          }
          //printf("%s\n", val.c_str());
        }
        if (i1 > 0)jRet.append(jRow);
      }
      delete [] colName;
      if (nRow == 0) {
         Json::Reader jReader;
         jReader.parse("[]", jRet);
      }
      
      sqlite3_free_table(dbResult);
      strcpy(err_msg,"");
      break;
    }
    else
    {
      //sprintf(err_msg, "sql err[1]");
      sprintf(err_msg, "%s", err_msg1);
    }
    Close();
  }
   return 0;
}

std::string SQLiteDB::GetOneValue(char * sql, char * err)
{
	int ret;
	std::string val = "";
	//char msg[2048];
	char ** dbResult;
	int nColumn, nRow = 0;

	for (int i=0; i<2; i++)
	{
		if ((ret=Open(m_dbname, err)) != SQLITE_OK)
		{
			sprintf(err, "failed to open db[1]");
			return val;
		}

		char * err_msg = NULL;
		ret = sqlite3_get_table(m_Connection, sql, &dbResult, &nRow, &nColumn, &err_msg);
		if (nRow == 0)
		{
			sprintf(err, "no value[1]");
		}
		else if (ret == SQLITE_OK)
		{
			int nIndex = nColumn;
			val = dbResult[nIndex];
			sqlite3_free_table(dbResult);
			strcpy(err,"");
			break;
		}
		else
		{
			sprintf(err, "sql err[1]");
		}
		Close();
	}

	return val;
}

std::string escapeSQL(const std::string& input) {
    std::string output;
    output.reserve(input.size() * 2);  // 预留足够的空间以避免频繁的内存分配

    for (char c : input) {
        switch (c) {
            case '\'':
                output += "''";  // 单引号转义（对于某些SQL服务器，如SQL Server）
                break;
            case '\"':
                output += "\\\""; // 双引号转义
                break;
            case '\\':
                output += "\\\\"; // 反斜杠转义
                break;
            case '\0':
                output += "\\0";  // 空字符转义
                break;
            case '\n':
                output += "\\n";  // 换行符转义
                break;
            case '\r':
                output += "\\r";  // 回车符转义
                break;
            case '\b':
                output += "\\b";  // 退格符转义
                break;
            case '\t':
                output += "\\t";  // 制表符转义
                break;
            case '\x1a':
                output += "\\Z";  // 控制字符转义
                break;
            default:
                output += c;  // 其他字符不转义
                break;
        }
    }

    return output;
}

std::string escapeSQLite(const std::string& input) {
    std::string output;
    output.reserve(input.size() * 2); // 预留足够的空间

    for (char c : input) {
        if (c == '\'') {
            output += "''"; // 在 SQLite 中，单引号通过双写来转义
        } else {
            output += c;
        }
    }

    return output;
}
