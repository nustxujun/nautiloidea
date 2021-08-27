#pragma once

#include "RenderContext.h"
#include "Common.h"
#include <regex>

//template<class T>
//class Functor
//{
//public:
//	operator T(){return mObject;}
//protected:
//	T mObject;
//};
//
//class TextureMaker: public Functor<Texture::Ptr>
//{
//public :
//	TextureMaker(const std::string& path, bool srgb)
//	{
//		mObject = RenderContext::getSingleton()->createObject<Texture>(path);
//		mObject->texture = Renderer::getSingleton()->createTextureFromFile(path, srgb);
//	}
//};
//
//class MaterialMaker: public Functor<Material::Ptr>
//{
//public:
//	MaterialMaker(const std::string& vs, const std::string& ps,const std::map<std::string, std::string>& textures)
//	{
//		auto renderer = Renderer::getSingleton();
//		auto rc = RenderContext::getSingleton();
//		mObject = rc->createObject<Material>(vs + ps);
//
//		for (auto& t : textures)
//		{
//			bool srgb = (t.first == "albedo");
//			mObject->textures[t.first] = TextureMaker(t.second, srgb);
//		}
//
//
//		mObject->init(vs, ps, {});
//	}
//};
//
//class ICOSphereCreater: public Functor<Mesh::Ptr>
//{
//public:
//	ICOSphereCreater(float radius,int resolution = 2);
//};
//
//class UVSphereCreator : public Functor<Mesh::Ptr>
//{
//public:
//	UVSphereCreator(float radius, int resolution = 10);
//};
//
//class QuadMesh
//{
//public:
//	Mesh::Ptr operator()(float w)const;
//};
//
//
//class ModelLoader
//{
//public:
//	Model::Ptr operator()(const std::string& path);
//};