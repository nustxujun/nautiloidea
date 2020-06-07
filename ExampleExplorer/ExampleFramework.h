#pragma once 
#include "RenderGraph.h"
#include <map>
#include <functional>
#include <string>

class ExampleFramework
{
public:
	using Ptr = std::shared_ptr<ExampleFramework>;

	virtual void update(float dtime) = 0;
	virtual void render() = 0;
};

class ExampleFrameworkManager
{
public:
	using Factory = std::function<ExampleFramework::Ptr()>;
	using ExampleFactories = std::map<std::string, Factory>;

	static  ExampleFrameworkManager& getSingleton()
	{
		static ExampleFrameworkManager mgr ;
		return mgr;
	}

	template<class T>
	int registerFactory(const std::string& n)
	{
		mFactories[n] = [](){
			return ExampleFramework::Ptr(new T);
		};
		return 1;
	}

	ExampleFramework::Ptr product(const std::string& name);
	void visit(std::function<void(const std::string&)>&& visitor);

private:
	ExampleFactories mFactories;
};


#define REGISTER_EXAMPLE(name) static int _##name = ExampleFrameworkManager::getSingleton().registerFactory<##name>(#name);