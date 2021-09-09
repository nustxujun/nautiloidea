#include "Texture.h"

Texture::Texture(std::string name, bool srgb)
{
	mPath = name;
	mDeviceResource =  Renderer::getSingleton()->createTextureFromFile(name, srgb);
}