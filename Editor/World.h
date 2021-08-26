#pragma once

#include "Common.h"
#include <unordered_map>
#include <functional>
#include "Resources.h"
#include "AutoObject.h"
#include "SimpleMath.h"
#include "Renderer.h"

struct SceneObject: public AutoObject<SceneObject>
{
	std::string name;
	virtual ~SceneObject(){}
};

struct RenderObject: SceneObject
{
	using Ptr = std::shared_ptr<RenderObject>;
	virtual void updateConstants(std::function<void(Renderer::PipelineState::Ref)>&& updater) = 0;
	virtual void draw(Renderer::CommandList * cmdlist) = 0;
};

class Node: public AutoObject<Node>
{
	friend class World;
public:
	Node();
	~Node();

	void setParent(Ref p);

	void addObject(SceneObject::Ptr o);

	template<class T>
	void visitObject(const std::function<void(std::shared_ptr<T>)>& visitor)
	{
		for (auto& o : objects)
		{
			auto p = std::dynamic_pointer_cast<T>(o);
			if (p)
				visitor(p);
		}

		for (auto& c : children)
		{
			c->visitObject(visitor);
		}
	}

	
	std::vector<Ptr> children;
	std::vector<SceneObject::Ptr> objects;

private:
	Ref mParent ;

	DirectX::SimpleMath::Matrix transform = DirectX::SimpleMath::Matrix::Identity;
};

class World
{
public:
	~World();
	static World& getInstance()
	{
		static World world;
		return world;
	}

	void newWorld();
	void openWorld(const std::string& path);
	void saveWorld(const std::string& path = 0);

	template<class T, class ... Args>
	T* createObject( Args&& ... args)
	{
		auto o = new T( args ...);
		o->incRef();
		return o;
	}


	Node::Ptr createNode();
	void attachToRoot(Node::Ptr n);
	Node::Ptr getRoot()const {return mRoot;}
	void visitRenderable(std::function<void(RenderObject::Ptr)>&& visitor);

private:
	Node::Ptr mRoot = 0;
};


