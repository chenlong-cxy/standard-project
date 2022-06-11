#pragma once

#include "Common.h"

//template<size_t N> //������ģ�����
//class ObjectPool
//{};
//�����ڴ��
template<class T>
class ObjectPool
{
public:
	//�������
	T* New()
	{
		T* obj = nullptr;

		//���Ȱѻ��������ڴ������ٴ��ظ�����
		if (_freeList != nullptr)
		{
			//����������ͷɾһ������
			obj = (T*)_freeList;
			_freeList = NextObj(_freeList);
		}
		else
		{
			//��֤�����ܹ��洢���µ�ַ
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//ʣ���ڴ治��һ�������Сʱ�������¿����ռ�
			if (_remainBytes < objSize)
			{
				_remainBytes = 128 * 1024;
				//_memory = (char*)malloc(_remainBytes);
				_memory = (char*)SystemAlloc(_remainBytes >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//�Ӵ���ڴ����г�objSize�ֽڵ��ڴ�
			obj = (T*)_memory;
			_memory += objSize;
			_remainBytes -= objSize;
		}
		//��λnew����ʾ����T�Ĺ��캯����ʼ��
		new(obj)T;

		return obj;
	}
	//�ͷŶ���
	void Delete(T* obj)
	{
		//��ʾ����T�����������������
		obj->~T();

		//���ͷŵĶ���ͷ�嵽��������
		NextObj(obj) = _freeList;
		_freeList = obj;
	}
private:
	char* _memory = nullptr;     //ָ�����ڴ��ָ��
	size_t _remainBytes = 0;     //����ڴ����зֹ�����ʣ���ֽ���

	void* _freeList = nullptr;   //���������������ӵ����������ͷָ��
};

//struct TreeNode
//{
//	int _val;
//	TreeNode* _left;
//	TreeNode* _right;
//	TreeNode()
//		:_val(0)
//		, _left(nullptr)
//		, _right(nullptr)
//	{}
//};
//
//
//void TestObjectPool()
//{
//	// �����ͷŵ��ִ�
//	const size_t Rounds = 3;
//	// ÿ�������ͷŶ��ٴ�
//	const size_t N = 1000000;
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//
//	//malloc��free
//	size_t begin1 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v1.push_back(new TreeNode);
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			delete v1[i];
//		}
//		v1.clear();
//	}
//	size_t end1 = clock();
//
//	//�����ڴ��
//	ObjectPool<TreeNode> TNPool;
//	std::vector<TreeNode*> v2;
//	v2.reserve(N);
//	size_t begin2 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v2.push_back(TNPool.New());
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			TNPool.Delete(v2[i]);
//		}
//		v2.clear();
//	}
//	size_t end2 = clock();
//
//	cout << "new cost time:" << end1 - begin1 << endl;
//	cout << "object pool cost time:" << end2 - begin2 << endl;
//}