#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;
ObjectPool<Span> SpanList::_spanPool;

//��central cache��ȡһ�������Ķ����thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock(); //����

	//�ڶ�Ӧ��ϣͰ�л�ȡһ���ǿյ�span
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span); //span��Ϊ��
	assert(span->_freeList); //span���е���������Ҳ��Ϊ��

	//��span�л�ȡn������
	//�������n�����ж����ö���
	start = span->_freeList;
	end = span->_freeList;
	size_t actualNum = 1;
	while (NextObj(end)&&n - 1)
	{
		end = NextObj(end);
		actualNum++;
		n--;
	}
	span->_freeList = NextObj(end); //ȡ���ʣ�µĶ�������ŵ���������
	NextObj(end) = nullptr; //ȡ����һ������ı�β�ÿ�
	span->_useCount += actualNum; //���±������thread cache�ļ���

	_spanLists[index]._mtx.unlock(); //����
	return actualNum;
}

//��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& spanList, size_t size)
{
	//1������spanList��Ѱ�ҷǿյ�span
	Span* it = spanList.Begin();
	while (it != spanList.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}
	//2��spanList��û�зǿյ�span��ֻ����page cache����
	//�Ȱ�central cache��Ͱ�����������������������ͷ��ڴ�����������������
	spanList._mtx.unlock();
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size; //��span���ᱻ�г�һ����size��С�Ķ���
	PageCache::GetInstance()->_pageMtx.unlock();
	//��ȡ��span����Ҫ�������¼���central cache��Ͱ��

	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С���ֽ�����
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;

	//�Ѵ���ڴ��г�size��С�Ķ�����������
	char* end = start + bytes;
	//����һ������ȥ��β������β��
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	//β��
	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr; //β��ָ���ÿ�
	
	//���кõ�spanͷ�嵽spanList
	spanList._mtx.lock(); //span�з���Ϻ���Ҫ�ҵ�Ͱ��ʱ�����¼�Ͱ��
	spanList.PushFront(span);

	return span;
}

//��һ�������Ķ��󻹸���Ӧ��span
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock(); //����
	while (start)
	{
		void* next = NextObj(start); //��¼��һ��
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		//������ͷ�嵽span����������
		NextObj(start) = span->_freeList;
		span->_freeList = start;

		span->_useCount--; //���±������thread cache�ļ���
		if (span->_useCount == 0) //˵�����span�����ȥ�Ķ���ȫ����������
		{
			//��ʱ���span�Ϳ����ٻ��ո�page cache��page cache�����ٳ���ȥ��ǰ��ҳ�ĺϲ�
			_spanLists[index].Erase(span);
			span->_freeList = nullptr; //���������ÿ�
			span->_next = nullptr;
			span->_prev = nullptr;

			//�ͷ�span��page cacheʱ��ʹ��page cache�����Ϳ����ˣ���ʱ��Ͱ�����
			_spanLists[index]._mtx.unlock(); //��Ͱ��
			PageCache::GetInstance()->_pageMtx.lock(); //�Ӵ���
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock(); //�����
			_spanLists[index]._mtx.lock(); //��Ͱ��
		}

		start = next;
	}

	_spanLists[index]._mtx.unlock(); //����
}