#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES) //����256KB���ڴ�����
	{
		//������������Ҫ�����ҳ��
		size_t alignSize = SizeClass::RoundUp(size);
		size_t kPage = alignSize >> PAGE_SHIFT;

		//��page cache����kPageҳ��span
		PageCache::GetInstance()->_pageMtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kPage);
		span->_objSize = size;
		PageCache::GetInstance()->_pageMtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		//ͨ��TLS��ÿ���߳������Ļ�ȡ�Լ�ר����ThreadCache����
		if (pTLSThreadCache == nullptr)
		{
			static std::mutex tcMtx;
			//cout << std::this_thread::get_id() <<"  "<<&tcMtx<< endl;
			static ObjectPool<ThreadCache> tcPool;
			tcMtx.lock();
			//pTLSThreadCache = new ThreadCache;
			pTLSThreadCache = tcPool.New();
			tcMtx.unlock();
		}
		//cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

		return pTLSThreadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objSize;
	if (size > MAX_BYTES) //����256KB���ڴ��ͷ�
	{
		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();
	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}
