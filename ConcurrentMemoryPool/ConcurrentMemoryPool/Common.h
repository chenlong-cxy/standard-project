#pragma once

#include <iostream>
#include <vector>
#include <ctime>
#include <cassert>
#include <thread>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <atomic>
#include "ObjectPool.h"

using std::cout;
using std::endl;

//С�ڵ���MAX_BYTES������thread cache����
//����MAX_BYTES����ֱ����page cache����ϵͳ������
static const size_t MAX_BYTES = 256 * 1024;
//thread cache��central cache���������ϣͰ�ı��С
static const size_t NFREELISTS = 208;
//page cache�й�ϣͰ�ĸ���
static const size_t NPAGES = 129;
//ҳ��Сת��ƫ�ƣ���һҳ����Ϊ2^13��Ҳ����8KB
static const size_t PAGE_SHIFT = 13;

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	//linux
#endif

#ifdef _WIN32
	#include <Windows.h>
#else
	//...
#endif

//ֱ��ȥ�������밴ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap��
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

//ֱ�ӽ��ڴ滹����
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	//linux��sbrk unmmap��
#endif
}

static void*& NextObj(void* ptr)
{
	return (*(void**)ptr);
}

//�����зֺõ�С�������������
class FreeList
{
public:
	//���ͷŵĶ���ͷ�嵽��������
	void Push(void* obj)
	{
		assert(obj);

		//ͷ��
		NextObj(obj) = _freeList;
		_freeList = obj;
		_size++;
	}
	//����������ͷ����ȡһ������
	void* Pop()
	{
		assert(_freeList);

		//ͷɾ
		void* obj = _freeList;
		_freeList = NextObj(_freeList);
		_size--;

		return obj;
	}
	//����һ�η�Χ�Ķ�����������
	void PushRange(void* start, void* end, size_t n)
	{
		assert(start);
		assert(end);

		//ͷ��
		NextObj(end) = _freeList;
		_freeList = start;
		_size += n;
	}
	//�����������ȡһ�η�Χ�Ķ���
	void PopRange(void*& start, void*& end, size_t n)
	{
		assert(n <= _size);

		//ͷɾ
		start = _freeList;
		end = start;
		for (size_t i = 0; i < n - 1;i++)
		{
			end = NextObj(end);
		}
		_freeList = NextObj(end); //��������ָ��end����һ������
		NextObj(end) = nullptr; //ȡ����һ������ı�β�ÿ�
		_size -= n;
	}
	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxSize;
	}

	size_t Size()
	{
		return _size;
	}

private:
	void* _freeList = nullptr; //��������
	size_t _maxSize = 1;
	size_t _size = 0;
};

//��������ӳ��ȹ�ϵ
class SizeClass
{
public:
	//������������10%���ҵ�����Ƭ�˷�
	//[1,128]              8byte����       freelist[0,16)
	//[128+1,1024]         16byte����      freelist[16,72)
	//[1024+1,8*1024]      128byte����     freelist[72,128)
	//[8*1024+1,64*1024]   1024byte����    freelist[128,184)
	//[64*1024+1,256*1024] 8*1024byte����  freelist[184,208)

	//һ��д��
	//static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	//{
	//	size_t alignSize = 0;
	//	if (bytes%alignNum != 0)
	//	{
	//		alignSize = (bytes / alignNum + 1)*alignNum;
	//	}
	//	else
	//	{
	//		alignSize = bytes;
	//	}
	//	return alignSize;
	//}

	//λ����д��
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + alignNum - 1)&~(alignNum - 1));
	}

	//��ȡ���϶������ֽ���
	static inline size_t RoundUp(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024)
		{
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024)
		{
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024)
		{
			return _RoundUp(bytes, 8 * 1024);
		}
		else
		{
			//assert(false);
			//return -1;
			//����256KB�İ�ҳ����
			return _RoundUp(bytes, 1 << PAGE_SHIFT);
		}
	}

	//һ��д��
	//static inline size_t _Index(size_t bytes, size_t alignNum)
	//{
	//	size_t index = 0;
	//	if (bytes%alignNum != 0)
	//	{
	//		index = bytes / alignNum;
	//	}
	//	else
	//	{
	//		index = bytes / alignNum - 1;
	//	}
	//	return index;
	//}

	//λ����д��
	static inline size_t _Index(size_t bytes, size_t alignShift)
	{
		return ((bytes + (1 << alignShift) - 1) >> alignShift) - 1;
	}

	//��ȡ��Ӧ��ϣͰ���±�
	static inline size_t Index(size_t bytes)
	{
		//ÿ�������ж��ٸ���������
		static size_t groupArray[4] = { 16, 56, 56, 56 };
		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			return _Index(bytes - 128, 4) + groupArray[0];
		}
		else if (bytes <= 8 * 1024)
		{
			return _Index(bytes - 1024, 7) + groupArray[0] + groupArray[1];
		}
		else if (bytes <= 64 * 1024)
		{
			return _Index(bytes - 8 * 1024, 10) + groupArray[0] + groupArray[1] + groupArray[2];
		}
		else if (bytes <= 256 * 1024)
		{
			return _Index(bytes - 64 * 1024, 13) + groupArray[0] + groupArray[1] + groupArray[2] + groupArray[3];
		}
		else
		{
			assert(false);
			return -1;
		}
	}

	//thread cacheһ�δ�central cache��ȡ���������
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);

		//����ԽС�������������Խ��
		//����Խ�󣬼����������Խ��
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;

		return num;
	}
	//central cacheһ����page cache��ȡ����ҳ
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size); //�����thread cacheһ����central cache�������ĸ�������
		size_t nPage = num*size; //num��size��С�Ķ���������ֽ���

		nPage >>= PAGE_SHIFT; //���ֽ���ת��Ϊҳ��
		if (nPage == 0) //���ٸ�һҳ
			nPage = 1;

		return nPage;
	}
};

//������ҳΪ��λ�Ĵ���ڴ�
struct Span
{
	PAGE_ID _pageId = 0;        //����ڴ���ʼҳ��ҳ��
	size_t _n = 0;              //ҳ������

	Span* _next = nullptr;      //˫����ṹ
	Span* _prev = nullptr;

	size_t _objSize = 0;        //�кõ�С����Ĵ�С
	size_t _useCount = 0;       //�кõ�С���ڴ棬�������thread cache�ļ���
	void* _freeList = nullptr;  //�кõ�С���ڴ����������

	bool _isUse = false;        //�Ƿ��ڱ�ʹ��
};

//��ͷ˫��ѭ������
class SpanList
{
public:
	SpanList()
	{
		//_head = new Span;
		_head = _spanPool.New();
		_head->_next = _head;
		_head->_prev = _head;
	}
	Span* Begin()
	{
		return _head->_next;
	}
	Span* End()
	{
		return _head;
	}
	bool Empty()
	{
		return _head == _head->_next;
	}
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}
	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}
	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;

		prev->_next = newSpan;
		newSpan->_prev = prev;

		newSpan->_next = pos;
		pos->_prev = newSpan;
	}
	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head); //����ɾ���ڱ�λ��ͷ���

		//if (pos == _head)
		//{
		//	int x = 0;
		//}

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}
private:
	Span* _head;
	static ObjectPool<Span> _spanPool;
public:
	std::mutex _mtx; //Ͱ��
};