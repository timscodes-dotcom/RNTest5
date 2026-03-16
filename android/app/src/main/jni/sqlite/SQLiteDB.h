#pragma once
#include "sqlite3.h"

#include "../CRITICAL_SECTION.h"
#include "../json/json/json.h"
#include <string>

std::string escapeSQL(const std::string& input);
std::string escapeSQLite(const std::string& input);

class SQLiteDB
{
protected:
   static const int     DBNAME_LENGTH = 128;
   sqlite3              *m_Connection;
   CRITICAL_SECTION     mCS;

   char					m_dbname[256];
  std::string   m_batchSqls;
  int           m_batchCount;

public:
   SQLiteDB(void);
   ~SQLiteDB(void);
   virtual int Open(char *dbname, char *err_msg, bool needInit = false, sqlite3 **dbconn = NULL);
   int Close();
   int Exec(const char *sql, char *err_msg, bool batchSave=false);
  int CompleteExecBatch(char *err_msg);
   int Select(char *sql, int *nRow, char *err_msg);
   char *SQLEncode(char *dst, char *src, int len);
   int Select(char *sql, Json::Value & jRet, char *err_msg);

   std::string GetOneValue(char * sql, char * err);
  
protected:
  int _Exec(const char *sql, char *err_msg);
};

std::string SqliteEncode(const char *src, int len);

extern SQLiteDB ExtDB;

//int LocalLog(const char *fmt, ...);
