#include "CRITICAL_SECTION.h"

#ifndef _WINDOWS
void InitializeCriticalSection(CRITICAL_SECTION * p) 
{
	p->init();
}
void DeleteCriticalSection(CRITICAL_SECTION * p) 
{
	p->release();
}
void EnterCriticalSection(CRITICAL_SECTION * p) 
{
	p->lock(); 
}
void LeaveCriticalSection(CRITICAL_SECTION * p) 
{
	p->unlock();
}
#endif