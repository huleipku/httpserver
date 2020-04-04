#pragma once
#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include<iostream>
#include<list>
#include<cstdio>
#include<semaphore.h>
#include<exception>
#include<pthread.h>
#include"myhttp_coon.h"
#include"mylock.h"
using namespace std;

template<typename T>
/*�̳߳صķ�װ*/
class threadpool
{
private:
	int max_thread;//�̳߳��е�����߳�����
	int max_job;//�������е��������
	pthread_t *pthread_poll;//�̳߳�����
	std::list<T*> m_myworkqueue;//�������
	mylocker m_queuelocker;//����������еĻ�����
	sem m_queuestat;//���ź������ж��Ƿ���������Ҫ����
	bool m_stop;;//�Ƿ�����߳�
public:
	threadpool();
	~threadpool();
	bool addjob(T* request);
private:
	static void* worker(void *arg);
	void run();
};
/*�̳߳صĴ���*/
template <typename T>
threadpool<T> ::threadpool()
{
	max_thread = 8;
	max_job = 1000;
	m_stop = false;
	pthread_poll = new pthread_t[max_thread];//Ϊ�̳߳ؿ��ٿռ�
	if (!pthread_poll)
	{
		throw std::exception();
	}
	for (int i = 0; i < max_thread; i++)
	{
		cout << "Create the pthread:" << i << endl;
		if (pthread_create(pthread_poll + i, NULL, worker, this) != 0)
		{
			delete[] pthread_poll;
			throw std::exception();
		}
		if (pthread_detach(pthread_poll[i]))//���̷߳���
		{
			delete[] pthread_poll;
			throw std::exception();
		}
	}
}

template <typename T>
threadpool<T>::~threadpool()
{
	delete[] pthread_poll;
	m_stop = true;
}

template <typename T>
bool threadpool<T>::addjob(T* request)
{
	m_queuelocker.lock();
	if (m_myworkqueue.size() > max_job)//���������д��������������У������
	{
		m_queuelocker.unlock();
		return false;
	}
	m_myworkqueue.push_back(request);//��������뵽���������
	m_queuelocker.unlock();
	m_queuestat.post();//���ź�������1
	return true;
}
template <typename T>
void* threadpool<T>::worker(void *arg)
{
	threadpool *pool = (threadpool*)arg;
	pool->run();
	return pool;
}

template <typename T>
void threadpool<T> ::run()
{
	while (!m_stop)
	{
		m_queuestat.wait();//�ź�����1��ֱ��Ϊ0��ʱ���̹߳���ȴ�
		m_queuelocker.lock();
		if (m_myworkqueue.empty())
		{
			m_queuelocker.unlock();
			continue;
		}
		T* request = m_myworkqueue.front();
		m_myworkqueue.pop_front();
		m_queuelocker.unlock();
		if (!request)
		{
			continue;
		}
		request->doit();//ִ�й�������
	}
}
#endif