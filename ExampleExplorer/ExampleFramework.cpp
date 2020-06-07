#include "ExampleFramework.h"


ExampleFramework::Ptr ExampleFrameworkManager::product(const std::string& name)
{
	return mFactories[name]();
}

void ExampleFrameworkManager::visit(std::function<void(const std::string&)>&& visitor)
{
	for (auto& ef: mFactories)
		visitor(ef.first);
}

