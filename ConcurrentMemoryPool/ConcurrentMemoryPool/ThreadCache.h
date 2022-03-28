#pragma once

#include "Common.h"

class ThreadCache
{
public:
	//�����ڴ����
	void* Allocate(size_t size);

	//�ͷ��ڴ����
	void Deallocate(void* ptr, size_t size);

	//�����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);

	//�ͷŶ�������������������ڴ浽���Ļ���
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[NFREELISTS]; //��ϣͰ
};

//TLS - Thread Local Storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;