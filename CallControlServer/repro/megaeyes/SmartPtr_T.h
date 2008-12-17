#ifndef SMARTPTR_TS_H
#define SMARTPTR_TS_H

#include <assert.h>
#include "ace/Thread_Mutex.h"
#include "ace/RW_Thread_Mutex.h"
#include "ace/Synch_Traits.h" 
//used by shared store
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class LockObj
{
public:
    void acquire()
	{ 
	    m_mutex.acquire();
	}

    void release()
	{ 
	    m_mutex.release();
	}

private:
    ACE_Thread_Mutex m_mutex;
};


class CountObj
{
public:
    CountObj(int c=1)
	:count(c)
	{}

    int addcnt()
	{
	    return ++count;
	}

    int delcnt()
	{
	    return --count;
	}
private:
    int count;
};


template<class T, class Lock>
class smart_ptr {
public:
    friend class boost::serialization::access;

    template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_NVP(pointee);
        }

public:
    explicit smart_ptr(T *p = 0)              // description of "explicit"
	: pointee(p)
	{
	    p_nCount = new CountObj;
	    p_lock = new Lock;
	}
public:
    smart_ptr(const smart_ptr& rhs)                // copy constructor member
	: pointee(rhs.get()), p_nCount(rhs.p_nCount),p_lock(rhs.p_lock)
	{
	    incref();
	}
public:
    ~smart_ptr()
	{
	    decref();
	}

    smart_ptr& operator=(smart_ptr const& rhs)  // assignment operator
	{
	    if (this != &rhs) 
	    {
		decref();
		pointee = rhs.pointee; 	
		p_nCount = rhs.p_nCount;
		p_lock = rhs.p_lock;
		incref();
	    }

	    return *this;	
	}

    bool operator==(smart_ptr const& rhs)  // assignment operator
	{
	    return this->pointee == rhs.pointee;	
	}

    T& operator*() const                     // see Item 28
	{ 
	    assert( pointee!=0 );
	    return *pointee; 
	}

    T* operator->() const                    // see Item 28
	{
	    assert( pointee!=0 );
	    return pointee; 
	}

    T* get() const                           // return value of current
	{ return pointee; }

    int attach(T *newp)                     //don't allow change pointee except NULL pointer!!
	{
	    if(NULL == newp)
		return -1;
	    if(pointee)
		return -1;
	    pointee = newp;
	    return 0;
	}

private:
    T *pointee;
    CountObj *p_nCount;
    Lock *p_lock;

private:
    void decref()
	{
	    p_lock->acquire();

	    if( 0==p_nCount->delcnt() )
	    {
		delete pointee; 	
		delete p_nCount;
		p_lock->release();
		delete p_lock;
	    }else{
		p_lock->release();		
	    }
	}

    void incref()
	{
	    p_lock->acquire();		
	    p_nCount->addcnt();
	    p_lock->release();
	}

};

#endif
