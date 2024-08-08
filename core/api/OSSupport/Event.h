#pragma once
class KEEN_API_EXPORT cEvent
{
public:
	cEvent(void);

	void Wait(void);

	void Set(void);

	void SetAll(void);

	bool Wait(unsigned a_TimeoutMSec);

private:

	bool m_ShouldContinue;

	std::mutex m_Mutex;

	std::condition_variable m_CondVar;
} ;





