#pragma once

#include "Common.h"

//单例模式
class CentralCache
{
public:
	//提供一个全局访问点
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//从central cache获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size);

	//获取一个非空的span
	Span* GetOneSpan(SpanList& spanList, size_t size);

	//将一定数量的对象还给对应的span
	void ReleaseListToSpans(void* start, size_t size);
private:
	SpanList _spanLists[NFREELISTS];

private:
	CentralCache() //构造函数私有
	{}
	CentralCache(const CentralCache&) = delete; //防拷贝

	static CentralCache _sInst;
};
