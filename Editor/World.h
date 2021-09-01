#pragma once

#include "Common.h"
#include <unordered_map>
#include <functional>
#include "Resources.h"
#include "AutoObject.h"
#include "SimpleMath.h"
#include "Renderer.h"
#include "Material.h"

class Node;
struct SceneObject: public AutoObject<SceneObject>
{
	std::string name;

	std::weak_ptr<Node> parentNode;

	virtual void update() {};
	virtual ~SceneObject(){}
};

struct RenderObject: SceneObject
{
	using Ptr = std::shared_ptr<RenderObject>;
	virtual void draw(Renderer::CommandList * cmdlist) = 0;

	Material::Ptr material;

};

class Node: public AutoObject<Node>
{
	friend class World;
public:

	std::vector<Ptr> children;
	std::vector<SceneObject::Ptr> objects;
	DirectX::SimpleMath::Matrix transform = DirectX::SimpleMath::Matrix::Identity;
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

private:
	Ref mParent ;
	bool mDirty = true;
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


	void attachToRoot(Node::Ptr n);
	Node::Ptr getRoot()const {return mRoot;}
	void visitRenderable(std::function<void(RenderObject::Ptr)>&& visitor);

private:
	Node::Ptr mRoot = 0;
};


