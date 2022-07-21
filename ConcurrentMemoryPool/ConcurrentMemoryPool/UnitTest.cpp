#include "ConcurrentAlloc.h"

void Alloc1()
{
	for (size_t i = 0; i < 5; i++)
	{
		void* ptr = ConcurrentAlloc(6);
	}
}
void Alloc2()
{
	for (size_t i = 0; i < 5; i++)
	{
		void* ptr = ConcurrentAlloc(7);
	}
}
void TLSTest()
{
	std::thread t1(Alloc1);
	std::thread t2(Alloc2);

	t1.join();
	t2.join();
}
void TestConcurrentAlloc1()
{
	void* p1 = ConcurrentAlloc(6);
	void* p2 = ConcurrentAlloc(8);
	void* p3 = ConcurrentAlloc(1);
	//void* p4 = ConcurrentAlloc(7);
	//void* p5 = ConcurrentAlloc(8);
	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	//cout << p4 << endl;
	//cout << p5 << endl;
	ConcurrentFree(p1);
	ConcurrentFree(p2);
	ConcurrentFree(p3);
	//ConcurrentFree(p4, 7);
	//ConcurrentFree(p5, 8);
}
void TestConcurrentAlloc2()
{
	for (size_t i = 0; i < 1024; i++)
	{
		void* p1 = ConcurrentAlloc(6);
	}
	void* p2 = ConcurrentAlloc(6);
}
void MultiThreadAlloc1()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 33; i++)
	{
		void* ptr = ConcurrentAlloc(6);
		v.push_back(ptr);
	}
	//for (auto e : v)
	//{
	//	ConcurrentFree(e);
	//}
}
void MultiThreadAlloc2()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 33; i++)
	{
		void* ptr = ConcurrentAlloc(7);
		v.push_back(ptr);
	}
	//for (auto e : v)
	//{
	//	ConcurrentFree(e);
	//}
}
void MultiThreadAllocTest()
{
	std::thread t1(MultiThreadAlloc1);
	
	std::thread t2(MultiThreadAlloc2);

	t1.join();
	t2.join();
}

void BigAlloc()
{
	//ÕÒpage cacheÉêÇë
	void* p1 = ConcurrentAlloc(257 * 1024); //257KB
	ConcurrentFree(p1);

	//ÕÒ¶ÑÉêÇë
	void* p2 = ConcurrentAlloc(129 * 8 * 1024); //129Ò³
	ConcurrentFree(p2);
}

//int main()
//{
//	TLSTest();
//	//TestObjectPool();
//	//TLSTest();
//	//cout << sizeof(size_t) << endl;
//	//TestConcurrentAlloc1();
//	//TestConcurrentAlloc2();
//	//MultiThreadAllocTest();
//	//BigAlloc();
//	//cout << (16 * 1024)/sizeof(ThreadCache) << endl;
//	//cout << 128 * 1024 << endl;
//	//cout << (337 * sizeof(Span))/1024 << endl;
//	//cout << 16 * 1024 << endl;
//	//cout << SpanList::_n << endl; //337
//
//	//TCMalloc_PageMap3<64 - PAGE_SHIFT> p;
//
//	//std::vector<void*> v;
//	//for (int i = 0; i < 7; i++)
//	//{
//	//	void* ptr = ConcurrentAlloc(6);
//	//	v.push_back(ptr);
//	//}
//	//for (auto e : v)
//	//{
//	//	ConcurrentFree(e);
//	//}
//	return 0;
//}