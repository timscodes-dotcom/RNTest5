#pragma once

#ifndef _WINDOWS
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <string>
#include <mutex>
/*
class base_lock
{	
	pthread_mutex_t m_mutex;
public:	
	base_lock() { pthread_mutex_init(&m_mutex, NULL); }	
	virtual ~base_lock() { pthread_mutex_destroy(&m_mutex); } 	
	virtual void lock() { pthread_mutex_lock(&m_mutex); }	
	virtual void unlock() { pthread_mutex_unlock(&m_mutex); }
};

*/
class CRITICAL_SECTION
{
public:
	CRITICAL_SECTION(void) {m_pid = 0;}
	~CRITICAL_SECTION(void) {}
	void init()
  {
    pthread_mutexattr_t Attr;

    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &Attr);
  }
  
	void release() { pthread_mutex_destroy(&m_mutex); }
	void lock() 
	{
		//pid_t _now = getpid();
		//if (_now == m_pid) return;
		pthread_mutex_lock(&m_mutex); 
		//m_pid = _now;
	}
	void unlock()
  {
    pthread_mutex_unlock(&m_mutex);
    //m_pid = 0;
  }
private:
	pthread_mutex_t m_mutex;
	pid_t m_pid;
};

class CRITICAL_SECTION1
{
public:
    CRITICAL_SECTION1(void) {}
    ~CRITICAL_SECTION1(void) {}

    std::string init() { return ""; }

    void release() {  }
    std::string lock()
    {
        m_mutex.lock();
        return "";
    }
    void unlock()
    {
        m_mutex.unlock();
    }
private:
    std::recursive_mutex m_mutex;
};

void InitializeCriticalSection(CRITICAL_SECTION * p);
void DeleteCriticalSection(CRITICAL_SECTION * p);
void EnterCriticalSection(CRITICAL_SECTION * p);
void LeaveCriticalSection(CRITICAL_SECTION * p);

class CMySafeLock {
public:
	CMySafeLock(CRITICAL_SECTION * lock) {
		m_lock = lock;
		m_lock->lock();
	}
	~CMySafeLock() {
		m_lock->unlock();
	}
private:
	CRITICAL_SECTION * m_lock;
};

inline unsigned long long getTimeStampMS() {
	auto now = std::chrono::system_clock::now(); // 获取当前时间点
	auto duration = now.time_since_epoch(); // 从1970-01-01 00:00:00 UTC到现在的持续时间

	// 转换为毫秒
	return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

inline void randString(int len, char * sz) {
	srand( (unsigned)time(NULL) );
	std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	int tot = characters.length();
	for (int i = 0; i < len; i++) {
		int pos = rand() % tot;
		sz[i] = characters.at(pos);
	}
	sz[len] = 0;
}

#endif
