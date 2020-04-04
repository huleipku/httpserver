#pragma once
#ifndef _MYLOCK_H
#define _MYLOCK_H
#include<iostream>
#include<list>
#include<cstdio>
#include<semaphore.h>
#include<exception>
#include<pthread.h>
#include"myhttp_coon.h"
using namespace std;

/*��װ�ź���*/
class sem {
private:
	sem_t m_sem;
public:
	sem();
	~sem();
	bool wait();//�ȴ��ź���
	bool post();//�����ź���
};
//�����ź���
sem::sem()
{
	if (sem_init(&m_sem, 0, 0) != 0)
	{
		throw std::exception();
	}
}
//�����ź���
sem :: ~sem()
{
	sem_destroy(&m_sem);
}
//�ȴ��ź���
bool sem::wait()
{
	return sem_wait(&m_sem) == 0;
}
//�����ź���
bool sem::post()
{
	return sem_post(&m_sem) == 0;
}

/*��װ������*/
class mylocker {
private:
	pthread_mutex_t m_mutex;
public:
	mylocker();
	~mylocker();
	bool lock();
	bool unlock();
};

mylocker::mylocker()
{
	if (pthread_mutex_init(&m_mutex, NULL) != 0)
	{
		throw std::exception();
	}
}

mylocker::~mylocker()
{
	pthread_mutex_destroy(&m_mutex);
}
/*����*/
bool mylocker::lock()
{
	return pthread_mutex_lock(&m_mutex) == 0;
}
/*�����*/
bool mylocker::unlock()
{
	return pthread_mutex_unlock(&m_mutex) == 0;
}

/*��װ��������*/
class mycond {
private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
public:
	mycond();
	~mycond();
	bool wait();
	bool signal();
};

mycond::mycond()
{
	if (pthread_mutex_init(&m_mutex, NULL) != 0)
	{
		throw std::exception();
	}
	if (pthread_cond_init(&m_cond, NULL) != 0)
	{
		throw std::exception();
	}
}

mycond::~mycond()
{
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
}

/*�ȴ���������*/
bool mycond::wait()
{
	int ret;
	pthread_mutex_lock(&m_mutex);
	ret = pthread_cond_wait(&m_cond, &m_mutex);
	pthread_mutex_unlock(&m_mutex);
	return ret == 0;
}

/*���ѵȴ������������߳�*/
bool mycond::signal()
{
	return pthread_cond_signal(&m_cond) == 0;
}

#endif