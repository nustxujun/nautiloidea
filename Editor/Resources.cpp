#include "Resources.h"
#include "imgui/imgui.h"
#include "World.h"


ResourceSystem::ResourceSystem()
{
	mSearchPaths = {"resources"};

	//refresh();
}
void ResourceSystem::refresh()
{
	namespace fs = std::filesystem;

	mOrderedRes.clear();

	std::vector<fs::path> paths;
	for (auto& p : mSearchPaths)
	{
		if (fs::exists(p))
		{
			paths.push_back(fs::absolute(p));
		}
	}


	for (auto& p : paths)
	{
		auto& dir = mDirectories[p];
		refreshDirectory(p, dir);
	}
	
}

void ResourceSystem::insert(Resource::Ptr res)
{
	mOrderedRes.insert(std::upper_bound(mOrderedRes.begin(), mOrderedRes.end(), res, [](auto& a, auto& b) {
		return a->getPath().filename() < b->getPath().filename();
		}), res);
}

std::vector<std::string> ResourceSystem::getTypeList()
{
	std::vector<std::string> list;
	for (auto& t: mTypes)
		list.push_back(t);
	return list;
}

void ResourceSystem::refreshDirectory(const std::filesystem::path& path, Directory& dir)
{
	namespace fs = std::filesystem;
	for (auto& f : fs::directory_iterator(path))
	{
		auto name = f.path().filename();
		auto first = name.c_str()[0];
		if (first == '.')
			continue;
		else if (f.is_directory())
		{
			refreshDirectory(f, dir.subs[f]);
		}
		else
		{
			auto ret = dir.resources.find(f);
			if (ret == dir.resources.end())
			{
				auto res = createResource(f);
				dir.resources[f] = res;
				insert(res);
			}
			else
				refreshResource(ret->second);
		}
	}
}

void ResourceSystem::refreshResource(Resource::Ptr res)
{
	res->updateFileInfo();
	insert(res);
}

Resource::Ptr ResourceSystem::createResource(const std::filesystem::path& path)
{
	namespace fs = std::filesystem;
	auto ext = path.extension().string();
	ext.erase(ext.begin());
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	auto ret = mFactories.find(ext);
	if (ret != mFactories.end())
		return ret->second(path);
	return Resource::Ptr(new UnknownResource(path));
}

Resource::Resource(const std::filesystem::path& path):
	mPath(path)
{
}

void Resource::updateFileInfo()
{
}

void Resource::interact()
{
	if (ImGui::BeginMenu("path"))
	{
		ImGui::Text(getPath().string().c_str());

		ImGui::EndMenu();
	}
}

std::string UnknownResource::type()
{
	return {"File"};
}

