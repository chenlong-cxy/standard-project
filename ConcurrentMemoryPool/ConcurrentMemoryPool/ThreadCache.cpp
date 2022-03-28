#include "ThreadCache.h"
#include "CentralCache.h"

//�����ڴ����
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); //thread cacheֻ����С�ڵ���MAX_BYTES���ڴ�����
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

//�ͷ��ڴ����
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	//�ҳ���Ӧ����������Ͱ���������
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	//�����������ȴ���һ����������Ķ������ʱ�Ϳ�ʼ��һ��list��central cache
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}
//1 2 3 4 5->5
//

//�����Ļ����ȡ����
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//����ʼ���������㷨
	//1���ʼ����һ����central cacheһ������Ҫ̫�࣬��ΪҪ̫���˿����ò���
	//2������㲻����size��С���ڴ�������ôbatchNum�ͻ᲻��������ֱ������
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 1;
	}
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum >= 1); //������һ��

	if (actualNum == 1) //���뵽����ĸ�����һ������ֱ�ӽ���һ�����󷵻ؼ���
	{
		assert(start == end);
		return start;
	}
	else //���뵽����ĸ����Ƕ��������Ҫ��ʣ�µĶ���ҵ�thread cache�ж�Ӧ�Ĺ�ϣͰ��
	{
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}
}

//�ͷŶ�������������������ڴ浽���Ļ���
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	//��list��ȡ��һ�����������Ķ���
	list.PopRange(start, end, list.MaxSize());
	
	//��ȡ���Ķ��󻹸�central cache�ж�Ӧ��span
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}