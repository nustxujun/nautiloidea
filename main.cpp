
#include "DefaultFrame.h"


#if defined(_CONSOLE)

int main()
{

	{

		DefaultFrame frame;
		try {
			frame.init(true);
			frame.update();
		}
		catch (...)
		{
		}
	}

	return 0;
}
#endif