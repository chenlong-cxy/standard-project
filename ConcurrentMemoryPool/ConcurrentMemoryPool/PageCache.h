#pragma once

#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"


//����ģʽ
class PageCache
{
public:
	//�ṩһ��ȫ�ַ��ʵ�
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	//��ȡһ��kҳ��span
	Span* NewSpan(size_t k);

	//��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	//�ͷſ��е�span�ص�PageCache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);

	std::mutex _pageMtx; //����
private:
	SpanList _spanLists[NPAGES];
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;

	ObjectPool<Span> _spanPool;
	
	PageCache() //���캯��˽��
	{}
	PageCache(const PageCache&) = delete; //������

	static PageCache _sInst;
};
