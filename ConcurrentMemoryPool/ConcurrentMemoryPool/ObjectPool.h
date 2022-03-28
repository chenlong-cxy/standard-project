#pragma once

#include "Common.h"

//template<size_t N> //非类型模板参数
//class ObjectPool
//{};

//定长内存池
template<class T>
class ObjectPool
{
public:
	//申请对象
	T* New()
	{
		T* obj = nullptr;

		//优先把还回来的内存块对象，再次重复利用
		if (_freeList != nullptr)
		{
			//从自由链表头删一个对象
			obj = (T*)_freeList;
			_freeList = NextObj(_freeList);
		}
		else
		{
			//保证对象能够存储得下地址
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//剩余内存不够一个对象大小时，则重新开大块空间
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
			//从大块内存中切出objSize字节的内存
			obj = (T*)_memory;
			_memory += objSize;
			_remainBytes -= objSize;
		}
		//定位new，显示调用T的构造函数初始化
		new(obj)T;

		return obj;
	}
	//释放对象
	void Delete(T* obj)
	{
		//显示调用T的析构函数清理对象
		obj->~T();

		//将释放的对象头插到自由链表
		NextObj(obj) = _freeList;
		_freeList = obj;
	}
private:
	char* _memory = nullptr;     //指向大块内存的指针
	size_t _remainBytes = 0;     //大块内存在切分过程中剩余字节数

	void* _freeList = nullptr;   //还回来过程中链接的自由链表的头指针
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
//	// 申请释放的轮次
//	const size_t Rounds = 3;
//	// 每轮申请释放多少次
//	const size_t N = 1000000;
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//
//	//malloc和free
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
//	//定长内存池
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