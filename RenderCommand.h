#pragma once
#include "SimpleIPC.h"

class CommandReceiver
{
public:
	void init(bool host);
	void receive();
private:
	SimpleIPC mIPC;

};

