#pragma once 

#include <assert.h>
#include <memory>

template<class T>
class AutoObject: public std::enable_shared_from_this<T>
{
public:
	using Ptr = std::shared_ptr<T>;
	using Ref = std::weak_ptr<T>;
	Ptr getShared(){return this->shared_from_this();}
};