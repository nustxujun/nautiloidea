#include "World.h"

Node::Node()
{
}

Node::~Node()
{
}

void Node::setParent(Ref p)
{
	if (!mParent.expired())
	{
		auto& ct = mParent.lock()->children;
		ct.erase(std::remove(ct.begin(), ct.end(), getShared()));
	}

	if (!p.expired())
		p.lock()->children.push_back(getShared());

	mParent = p;
}

void Node::addObject(SceneObject::Ptr o)
{
	objects.push_back(o);
}



void World::newWorld()
{
	mRoot = std::make_shared<Node>();
}

World::~World()
{

}



void World::attachToRoot(Node::Ptr n)
{
	n->setParent(mRoot);
}

void World::visitRenderable(std::function<void(RenderObject::Ptr)>&& visitor)
{
	mRoot->visitObject<RenderObject>([visitor = std::move(visitor)](RenderObject::Ptr so){
		visitor(so);
	});
}

