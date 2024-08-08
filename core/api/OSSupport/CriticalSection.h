
#pragma once

class KEEN_API_EXPORT cCriticalSection
{
	friend class cDeadlockDetect;

public:
	void Lock(void);
	void Unlock(void);

	cCriticalSection(void);

	bool IsLocked(void);

	bool IsLockedByCurrentThread(void);

private:

	int m_RecursionCount;

	std::thread::id m_OwningThreadID;

	std::recursive_mutex m_Mutex;
};

class KEEN_API_EXPORT cCSLock
{
	cCriticalSection * m_CS;
	bool m_IsLocked;

public:
	cCSLock(cCriticalSection * a_CS);
	cCSLock(cCriticalSection & a_CS);
	~cCSLock();

	void Lock(void);
	void Unlock(void);

private:
	DISALLOW_COPY_AND_ASSIGN(cCSLock);
} ;

class KEEN_API_EXPORT cCSUnlock
{
	cCSLock & m_Lock;
public:
	cCSUnlock(cCSLock & a_Lock);
	~cCSUnlock();

private:
	DISALLOW_COPY_AND_ASSIGN(cCSUnlock);
} ;




