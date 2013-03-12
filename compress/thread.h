#ifndef CUSTOM_THREAD_H
#define CUSTOM_THREAD_H

#include <pthread.h>
#include <vector>
#include <stdexcept>

/**
 * @file Header for the @ref Thread group.
 */

/**
 * @defgroup Thread Threading operations
 * This module contains threading classes similar to the boost threading
 * library. Unfortunately, casapy is using boost too, and linking a shared library that uses
 * a different boost implementation is giving problems. Hence, I've re-implemented
 * the most important parts of boost threading here. Interface is similar, but
 * it's a quick/simple implementation so details might be slightly different.
 * @author André Offringa
 */

/** \addtogroup Thread
 *  @{
 */

/**
 * A thread of execution. Can call a functor from a newly created thread.
 */
class ZThread
{
	public:
		/**
		 * Create a new thread and call the functor @p threadFunc from the
		 * function.
		 * @param threadFunc the functor to be called from the new thread.
		 */
		template<typename T>
		ZThread(T threadFunc) : _threadFunc(new T(threadFunc))
		{
			int status = pthread_create(&_thread, NULL, &ZThread::start<T>, this);
			if(status != 0)
				throw std::runtime_error("Could not create thread");
		}
		
		/**
		 * Destructor. Will not call join() !
		 */
		~ZThread()
		{
		}
		
		/**
		 * Wait until the thread is finished and clean up.
		 */
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

/**
 * Group of threads.
 */
class ZThreadGroup
{
	public:
		/** Constructor */
		ZThreadGroup() { }
		/** Destructor. Will join all threads that have not been joined yet. */
		~ZThreadGroup() { join_all(); }
		
		/**
		 * Create a new thread that will exeute the given functor. The new thread
		 * will be added to the group.
		 * @param threadFunc The functor to be called.
		 */
		template<typename T>
		void create_thread(T threadFunc)
		{
			_threads.push_back(new ZThread(threadFunc));
		}
		
		/**
		 * Join all threads in the group that have not yet been joined.
		 */
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

/**
 * A mutex. Use ZMutex::scoped_lock to safely lock it.
 */
class ZMutex
{
	public:
		friend class ZCondition;
		
		/** Create an unlocked mutex. */
		ZMutex()
		{
			int status = pthread_mutex_init(&_mutex, NULL);
			if(status != 0)
				throw std::runtime_error("Could not init mutex");
		}
		
		/** Destruct the mutex. */
		~ZMutex()
		{
			int status = pthread_mutex_destroy(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not destroy mutex");
		}
		
		/** Lock the mutex. Preferably done by a scoped_lock .*/
		void lock()
		{
			int status = pthread_mutex_lock(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not lock mutex");
		}
		
		/** Unlock the mutex. Preferably done by a scoped_lock . */
		void unlock()
		{
			int status = pthread_mutex_unlock(&_mutex);
			if(status != 0)
				throw std::runtime_error("Could not unlock mutex");
		}
		
		/** A lock that is released when it goes out of scope. */
		class scoped_lock
		{
			public:
				/**
				 * Create a locked scoped_lock for the given mutex.
				 * @param mutex Mutex that will be locked.
				 */
				scoped_lock(ZMutex &mutex) : _hasLock(false), _mutex(mutex)
				{
					_mutex.lock();
					_hasLock = true;
				}
				/**
				 * Destruct the lock, release if it was locked.
				 */
				~scoped_lock()
				{
					if(_hasLock) _mutex.unlock();
				}
				/**
				 * Lock the mutex.
				 */
				void lock() { _mutex.lock(); _hasLock = true; }
				/**
				 * Unlock the mutex.
				 */
				void unlock() { _mutex.unlock(); _hasLock = false; }
			private:
				friend class ZCondition;
				bool _hasLock;
				ZMutex &_mutex;
		};
		
	private:
		pthread_mutex_t _mutex;
};

/**
 * Synchronization class for waiting for conditions.
 */
class ZCondition
{
	public:
		/** Create a condition */
		ZCondition()
		{
			int status = pthread_cond_init(&_condition, NULL);
			if(status != 0)
				throw std::runtime_error("Could not init condition");
		}
		/** Destruct the condition. */
		~ZCondition()
		{
			int status = pthread_cond_destroy(&_condition);
			if(status != 0)
				throw std::runtime_error("Could not destroy condition");
		}
		/**
		 * Notify all waiting threads that the condition might have changed.
		 */
		void notify_all()
		{
			int status = pthread_cond_broadcast(&_condition);
			if(status != 0)
				throw std::runtime_error("Could not broadcast condition");
		}
		/**
		 * Wait for a possible change of the condition.
		 * @param lock The lock that is associated with the condition.
		 */
		void wait(ZMutex::scoped_lock &lock)
		{
			int status = pthread_cond_wait(&_condition, &lock._mutex._mutex);
			if(status != 0)
				throw std::runtime_error("Could not wait for condition");
		}
	private:
		pthread_cond_t _condition;
};

// end of Doxygen group
/** @}*/

#endif

