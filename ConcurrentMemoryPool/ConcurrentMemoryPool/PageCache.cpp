#include "PageCache.h"
//#include "CentralCache.h"

PageCache PageCache::_sInst;

//��ȡһ��kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	//assert(k > 0 && k < NPAGES);
	assert(k > 0);
	if (k > NPAGES - 1) //����128ҳֱ���Ҷ�����
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();

		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;
		//����ҳ����span֮���ӳ��
		//_idSpanMap[span->_pageId] = span;
		_idSpanMap.set(span->_pageId, span);

		return span;
	}
	//�ȼ���k��Ͱ������û��span
	if (!_spanLists[k].Empty())
	{
		Span* kSpan = _spanLists[k].PopFront();

		//����ҳ����span��ӳ�䣬����central cache����С���ڴ�ʱ���Ҷ�Ӧ��span
		for (PAGE_ID i = 0; i < kSpan->_n; i++)
		{
			//_idSpanMap[kSpan->_pageId + i] = kSpan;
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}

		return kSpan;
	}
	//���һ�º����Ͱ������û��span������п��Խ�������з�
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New();

			//��nSpan��ͷ����kҳ����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;
			//��ʣ�µĹҵ���Ӧӳ���λ��
			_spanLists[nSpan->_n].PushFront(nSpan);
			//�洢nSpan����βҳ����nSpan֮���ӳ�䣬����page cache�ϲ�spanʱ����ǰ��ҳ�Ĳ���
			//_idSpanMap[nSpan->_pageId] = nSpan;
			//_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;
			_idSpanMap.set(nSpan->_pageId, nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);

			//����ҳ����span��ӳ�䣬����central cache����С���ڴ�ʱ���Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_n; i++)
			{
				//_idSpanMap[kSpan->_pageId + i] = kSpan;
				_idSpanMap.set(kSpan->_pageId + i, kSpan);
			}

			//cout << "dargon" << endl; //for test
			return kSpan;
		}
	}
	//�ߵ�����˵������û�д�ҳ��span�ˣ���ʱ���������һ��128ҳ��span
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New();

	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	//������������ظ����ݹ�����Լ�
	return NewSpan(k);
}

//��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT; //ҳ��

	//std::unique_lock<std::mutex> lock(_pageMtx); //����ʱ����������ʱ�Զ�����
	//auto ret = _idSpanMap.find(id);
	//if (ret != _idSpanMap.end())
	//{
	//	return ret->second;
	//}
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//}

	Span* ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

//�ͷſ��е�span�ص�PageCache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	if (span->_n > NPAGES - 1) //����128ҳֱ���ͷŸ���
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);

		return;
	}
	//��span��ǰ��ҳ�����Խ��кϲ��������ڴ���Ƭ����
	//1����ǰ�ϲ�
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		//auto ret = _idSpanMap.find(prevId);
		////ǰ���ҳ��û�У���δ��ϵͳ���룩��ֹͣ��ǰ�ϲ�
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}
		Span* ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}
		//ǰ���ҳ�Ŷ�Ӧ��span���ڱ�ʹ�ã�ֹͣ��ǰ�ϲ�
		//Span* prevSpan = ret->second;
		Span* prevSpan = ret;
		if (prevSpan->_isUse == true)
		{
			break;
		}
		//�ϲ�������128ҳ��span�޷����й���ֹͣ��ǰ�ϲ�
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}
		//������ǰ�ϲ�
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		//��prevSpan�Ӷ�Ӧ��˫�������Ƴ�
		_spanLists[prevSpan->_n].Erase(prevSpan);

		//delete prevSpan;
		_spanPool.Delete(prevSpan);
	}
	//2�����ϲ�
	while (1)
	{
		PAGE_ID nextId = span->_pageId + span->_n;
		//auto ret = _idSpanMap.find(nextId);
		////�����ҳ��û�У���δ��ϵͳ���룩��ֹͣ���ϲ�
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}
		Span* ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr)
		{
			break;
		}
		//�����ҳ�Ŷ�Ӧ��span���ڱ�ʹ�ã�ֹͣ���ϲ�
		//Span* nextSpan = ret->second;
		Span* nextSpan = ret;
		if (nextSpan->_isUse == true)
		{
			break;
		}
		//�ϲ�������128ҳ��span�޷����й���ֹͣ���ϲ�
		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}
		//�������ϲ�
		span->_n += nextSpan->_n;

		//��nextSpan�Ӷ�Ӧ��˫�������Ƴ�
		_spanLists[nextSpan->_n].Erase(nextSpan);

		//delete nextSpan;
		_spanPool.Delete(nextSpan);
	}
	//���ϲ����span�ҵ���Ӧ��˫������
	_spanLists[span->_n].PushFront(span);
	//������span������βҳ��ӳ��
	//_idSpanMap[span->_pageId] = span;
	//_idSpanMap[span->_pageId + span->_n - 1] = span;
	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);

	//����span����Ϊδ��ʹ�õ�״̬
	span->_isUse = false;
}