#pragma once

#include "INIMgr.h"

class RP1210API;
class TestRP1210
{
public:
	TestRP1210(void);
	~TestRP1210(void);

	void Test (vector<INIMgr::Devices> &devs, int idx1, int idx2);
	void LogError (RP1210API &api, int code, COLORREF clr = 0x000090);
};
