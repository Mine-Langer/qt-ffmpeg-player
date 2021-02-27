#pragma once
#include "Common.h"

class CPlayer 
{
public:
	CPlayer();
	~CPlayer();

	bool Start(const char* szInput);

	void Stop();

private:
	bool m_bRun = false;
};
