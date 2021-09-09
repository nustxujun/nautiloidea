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
	virtual void onTransformChanged(const DirectX::SimpleMath::Matrix& transform){};
	virtual ~SceneObject(){}
};

struct RenderObject:public SceneObject
{
	using Ptr = std::shared_ptr<RenderObject>;
	virtual void draw(Renderer::CommandList * cmdlist) = 0;
	Material::Ptr material;
protected:
	Renderer::ConstantBuffer::Ptr mConstants;
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
	void setOffsetTransform(const DirectX::SimpleMath::Matrix& trans);
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

	void update();
private:
	void notifyTransformChanged();
private:
	Ref mParent ;
	DirectX::SimpleMath::Matrix mOffsetTransform = DirectX::SimpleMath::Matrix::Identity;
	bool mDirty = true;
};

struct Camera: public SceneObject
{
	struct CameraConstants
	{
		DirectX::SimpleMath::Matrix view;
		DirectX::SimpleMath::Matrix proj;

		float4 campos;
		float4 camdir;
	};

	using Ptr = std::shared_ptr<Camera>;

	
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;

	Camera();
	void update();

	void setView(const DirectX::SimpleMath::Vector3& eye, const DirectX::SimpleMath::Vector3& lookat, const DirectX::SimpleMath::Vector3& up);
	void setProjection(float fov, float radio, float nearPlane, float farPlane);
	void setViewport(float left, float top, float width, float height, float min_depth, float max_depth);
	void setScissorRect(LONG left, LONG top, LONG right, LONG bottom);

	Renderer::ConstantBuffer::Ptr getConstants() { return mHardwarebuffer; }
private:
	Renderer::ConstantBuffer::Ptr mHardwarebuffer;
	CameraConstants mConstants;
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


