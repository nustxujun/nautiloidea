#pragma once 

#include "Common.h"
#include "AutoObject.h"
#include <filesystem>
#include <memory>

class Resource:public AutoObject<Resource>
{
public:
	using Ptr = Resource*;

	Resource(const std::filesystem::path& path);
	void updateFileInfo() ;
	const std::filesystem::path& getPath()const {return mPath;};
	
	virtual void interact();
	virtual std::string type() = 0;
private:
	std::filesystem::path mPath;
	std::filesystem::file_time_type mTime;
	bool mNeedUpdate = true;
};

class ResourceSystem
{
public:
	struct Directory
	{
		std::map<std::filesystem::path, Directory> subs;
		std::map<std::filesystem::path, Resource::Ptr> resources;
	};

	using Resources = std::map<std::filesystem::path, Directory>;
	
	static ResourceSystem& getInstance()
	{
		static ResourceSystem rs;
		return rs;
	}

	ResourceSystem();
	void refresh();
	const Resources& getResources() const { return mDirectories; }
	const std::vector<Resource::Ptr>& getResourceList()const {return mOrderedRes;}
	
	template<class T>
	bool registerFactory(std::initializer_list<std::string> exts)
	{
		mTypes.insert(T("").type());
		for (auto& ext : exts)
			mFactories[ext] = [](std::filesystem::path p)->Resource::Ptr
			{
				return static_cast<Resource*>(new T(std::move(p)));
			};
		return true;
	}

	std::vector<std::string> getTypeList();
private:
	void refreshDirectory(const std::filesystem::path& path, Directory& dir);
	void refreshResource(Resource::Ptr res);
	Resource::Ptr createResource(const std::filesystem::path& path);
	void insert(Resource::Ptr);

private:

	Resources mDirectories;
	std::vector<Resource::Ptr> mOrderedRes;
	std::set<std::string> mTypes;
	std::set<std::string> mSearchPaths;
	std::map<std::string, std::function<Resource::Ptr(std::filesystem::path)>> mFactories;
};


#define IS_RESOURCE() const static bool initialized;
#define REGISTER_FACTORY(TYPE, ...) \
__if_exists(TYPE::initialized) { const bool TYPE::initialized = ResourceSystem::getInstance().registerFactory<TYPE>({__VA_ARGS__}); }\
__if_not_exists(TYPE::initialized) {static_assert(0 && "need member: initialized in "#TYPE);}



class UnknownResource : public Resource
{
	IS_RESOURCE()
public:
	using Resource::Resource;
	std::string type() override;

};
