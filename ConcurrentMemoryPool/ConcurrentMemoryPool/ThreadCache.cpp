#include "ThreadCache.h"
#include "CentralCache.h"

//申请内存对象
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); //thread cache只处理小于等于MAX_BYTES的内存申请
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		return FetchFromCentralCache(index, alignSize);
	}
}

//释放内存对象
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	//找出对应的自由链表桶将对象插入
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	//当自由链表长度大于一次批量申请的对象个数时就开始还一段list给central cache
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}
//1 2 3 4 5->5
//

//从中心缓存获取对象
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//慢开始反馈调节算法
	//1、最开始不会一次向central cache一次批量要太多，因为要太多了可能用不完
	//2、如果你不断有size大小的内存需求，那么batchNum就会不断增长，直到上限
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 1;
	}
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum >= 1); //至少有一个

	if (actualNum == 1) //申请到对象的个数是一个，则直接将这一个对象返回即可
	{
		assert(start == end);
		return start;
	}
	else //申请到对象的个数是多个，还需要将剩下的对象挂到thread cache中对应的哈希桶中
	{
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}
}

//释放对象导致链表过长，回收内存到中心缓存
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	//从list中取出一次批量个数的对象
	list.PopRange(start, end, list.MaxSize());
	
	//将取出的对象还给central cache中对应的span
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}