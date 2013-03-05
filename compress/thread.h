#ifndef CUSTOM_THREAD_H
#define CUSTOM_THREAD_H

#include <pthread.h>
#include <vector>
#include <stdexcept>

/**
 * Unfortunately, casapy is using boost too, and linking a shared library that uses
 * a different boost implementation is giving problems. Hence, I've re-implemented
 * the most important parts of boost threading here. Interface is similar, but
 * it's a quick/simple implementation so details might be slightly different.
 */

class ZThread
{
	public:
		template<typename T>
		ZThread(T threadFunc) : _threadFunc(new T(threadFunc))
		{
			int status = pthread_create(&_thread, NULL, &ZThread::start<T>, this);
			if(status != 0)
				throw std::runtime_error("Could not create thread");
		}
		~ZThread()
		{
		}
		
		void join()
		{
			void *result;
			int status = pthread_join(_thread, &result);
			if(status != 0)
				throw std::runtime_error("Could not join thread");
		}
	private:
		template<typename T>
		static void *start(void *arg)
		{
			ZThread *obj = reinterpret_cast<ZThread*>(arg);
			T &threadFunc = (*reinterpret_cast<T*>(obj->_threadFunc));
			threadFunc();
			delete &threadFunc;
			return 0;
		}
		
		void *_threadFunc;
		pthread_t _thread;
};

class ZThreadGroup
{
	public:
		ZThreadGroup() { }
		~ZThreadGroup() { join_all(); }
		
		template<typename T>
		void create_thread(T threadFunc)
		{
			_threads.push_back(new ZThread(threadFunc));
		}
		void join_all()
		{
			for(std::vector<ZThread*>::iterator i=_threads.begin(); i!=_threads.end(); ++i)
			{
				(*i)->join();
				delete *i;
			}
			_threads.clear();
		}
		
	private:
		std::vector<ZThread*> _threads;
};

class ZMutex
{
	public:
		friend class ZCondition;
		ZMutex()
		{
			int status = pthread_mutex_init(&_mutex, NULL);
			if(status != 0)
				throw std::runtime_error("Could not init mutex");
		}
		~ZMutex()
		{
			int status = pthread_mutex_destroy(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not destroy mutex");
		}
		void lock()
		{
			int status = pthread_mutex_lock(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not lock mutex");
		}
		void unlock()
		{
			int status = pthread_mutex_unlock(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not unlock mutex");
		}
		
		class scoped_lock
		{
			public:
				friend class ZCondition;
				scoped_lock(ZMutex &mutex) : _hasLock(false), _mutex(mutex)
				{
					_mutex.lock();
					_hasLock = true;
				}
				~scoped_lock()
				{
					if(_hasLock) _mutex.unlock();
				}
				void lock() { _mutex.lock(); _hasLock = true; }
				void unlock() { _mutex.unlock(); _hasLock = false; }
			private:
				bool _hasLock;
				ZMutex &_mutex;
		};
		
	private:
		pthread_mutex_t _mutex;
};

class ZCondition
{
	public:
		ZCondition()
		{
			int status = pthread_cond_init(&_condition, NULL);
			if(status != 0)
				throw std::runtime_error("Could not init condition");
		}
		~ZCondition()
		{
			int status = pthread_cond_destroy(&_condition);
			if(status != 0)
				throw std::runtime_error("Could not destroy condition");
		}
		void notify_all()
		{
			int status = pthread_cond_broadcast(&_condition);
			if(status != 0)
				throw std::runtime_error("Could not broadcast condition");
		}
		void wait(ZMutex::scoped_lock &lock)
		{
			int status = pthread_cond_wait(&_condition, &lock._mutex._mutex);
			if(status != 0)
				throw std::runtime_error("Could not wait for condition");
		}
	private:
		pthread_cond_t _condition;
};

#endif
