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
		mParent.lock()->mDirty = true;
		auto& ct = mParent.lock()->children;
		ct.erase(std::remove(ct.begin(), ct.end(), getShared()));
	}

	if (!p.expired())
		p.lock()->children.push_back(getShared());

	mParent = p;
	p.lock()->mDirty = true;
}

void Node::addObject(SceneObject::Ptr o)
{
	o->parentNode = getShared();
	objects.push_back(o);
	mDirty = true;
}

void Node::update()
{
	for (auto& c : children)
	{
		c->mDirty |= mDirty;
		c->update();
	}

	if (mDirty)
	{
		mDirty = false;
		notifyTransformChanged();
	}
}

void Node::notifyTransformChanged()
{
	for (auto& o : objects)
	{
		o->onTransformChanged(transform);
	}
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


Camera::Camera()
{
	mHardwarebuffer = Renderer::getSingleton()->createConstantBuffer(sizeof(CameraConstants));
}

void Camera::update()
{
	if (mDirty)
	{
		mDirty = false;
		mHardwarebuffer->blit(&mConstants, 0, sizeof(mConstants));
	}
}

void Camera::setView(const DirectX::SimpleMath::Vector3& eye, const DirectX::SimpleMath::Vector3& lookat, const DirectX::SimpleMath::Vector3& up)
{
	mConstants.view = DirectX::SimpleMath::Matrix::CreateLookAt(eye, lookat, up).Transpose();
	mDirty = true;
}

void Camera::setProjection(float fov, float radio, float n, float f)
{
	mConstants.proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fov, radio, n, f).Transpose();
	mDirty = true;
}

void Camera::setViewport(float left, float top, float width, float height, float min_depth, float max_depth)
{
	viewport = { left, top, width, height, min_depth, max_depth };
}

void Camera::setScissorRect(LONG left, LONG top, LONG right, LONG bottom)
{
	scissor = { left, top, right, bottom };
}
