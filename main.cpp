
#include "DefaultFrame.h"
#include "Dispatcher.h"

#if defined(_CONSOLE)


std::function<void(void)> print(const std::string& str)
{
	return [=]() {
		std::cout << str << std::endl;
	};
}

int main()
{
	
	{

		DefaultFrame frame;
		try {
			frame.init(false);
			frame.update();
		}
		catch (...)
		{
		}
	}

	return 0;
}
#endif