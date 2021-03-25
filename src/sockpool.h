#ifndef __SOCKPOOL_H__
#define __SOCKPOOL_H__


class SockPool {    
public:
    explicit SockPool(int capacity);
    virtual ~SockPool();
    
    virtual int init();
    virtual void finish(); 
    
    int addEvent(int fd, int type, void* data);
    int delEvent(int fd, int type, void* data);	

protected:
    virtual void dealEvent(int type, void* data) = 0;
    
    int chkRdEvent(); 
    int chkWrEvent(); 
    	
protected:
    const int m_capacity;
	int epfdrd;
	int epfdwr;
};

#endif

