
#ifndef KFIFO_H
#define KFIFO_H

#ifdef __cplusplus
extern "C"{
#endif

#include "string.h"
#include "stdint.h"
#include "stdbool.h"
#define __NSX_PASTE__(A,B) A##B

#define min(A,B) __NSMIN_IMPL__(A,B,__COUNTER__)

#define __NSMIN_IMPL__(A,B,L) ({ __typeof__(A) __NSX_PASTE__(__a,L) = (A); \
                             __typeof__(B) __NSX_PASTE__(__b,L) = (B); \
                             (__NSX_PASTE__(__a,L) < __NSX_PASTE__(__b,L)) ? __NSX_PASTE__(__a,L) : __NSX_PASTE__(__b,L); \
                          })

typedef struct{
    int TODO;
}spinlock_t;
//#define spin_lock_irqrestore(lock, flags) flags = (unsigned long)lock+1
//#define spin_unlock_irqrestore(lock, flags) flags = (unsigned long)lock+1
int spin_unlock_irqrestore(spinlock_t* lock,unsigned int flags);
int spin_lock_irqrestore(spinlock_t* lock,unsigned int flags);

#define EPERM 1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3 /* No such process */
#define EINTR 4 /* Interrupted system call */


typedef struct kfifo {
    unsigned char *buffer;     /* the buffer holding the data */
    unsigned int size;         /* the size of the allocated buffer */
    unsigned int in;           /* data is added at offset (in % size) */
    unsigned int out;          /* data is extracted from off. (out % size) */
    spinlock_t *lock;          /* protects concurrent modifications */
}kfifo_t;


bool is_power_of_2(int n) ;
unsigned int __kfifo_put(struct kfifo *fifo, const unsigned char *buffer, unsigned int len);
unsigned int __kfifo_get(struct kfifo *fifo, unsigned char *buffer, unsigned int len);
unsigned int __kfifo_put_try(struct kfifo *fifo, const unsigned char *buffer, unsigned int len);
unsigned int __kfifo_get_try(struct kfifo *fifo, unsigned char *buffer, unsigned int len);

static inline void *ERR_PTR(long error)
{
    return (void *) error;
}
static inline long PTR_ERR(const void *ptr)
{
    return (long) ptr;
}
static inline long IS_ERR(const void *ptr)
{
    return (unsigned long)ptr > (unsigned long)-1000L;
}

static inline unsigned int kfifo_get_used(struct kfifo *kfifo)
{
	return (kfifo->in - kfifo->out);
}
static inline unsigned int kfifo_get_free(struct kfifo *kfifo)
{
	return kfifo->size - kfifo_get_used(kfifo);
}

struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size, spinlock_t *lock);

#define KFIFO_INIT(fifo,_size,_lock) do { \
    static kfifo_t fifo##_body;               \
    static uint8_t fifo##_buf[_size];               \
     fifo##_body.buffer =  fifo##_buf;      \
     fifo##_body.size=_size;           \
    (fifo##_body).in = (fifo##_body).out = 0; \
    (fifo##_body).lock = (_lock);         \
    fifo = &(fifo##_body);                                          \
} while (0)
//arm stm32f7/h7 use
#define kfifo_alloc_static(pout,size,lock) do{\
	static uint8_t __##size##lock_buffer[size];\
	pout = kfifo_init(__##size##lock_buffer,size,lock);\
}	while(0)
#define kfifo_alloc_static_dma(pout,size,lock) do{\
	static uint8_t __##size##lock_buffer[size] ;\
	pout = kfifo_init(__##size##lock_buffer,size,lock);\
}	while(0)

struct kfifo *kfifo_alloc(unsigned int size, spinlock_t *lock);
static inline unsigned int kfifo_put(struct kfifo *fifo, const unsigned char *buffer, unsigned int len)
{
    unsigned long flags;
    unsigned int ret;
    spin_lock_irqrestore(fifo->lock, flags);
    ret = __kfifo_put(fifo, buffer, len);
    spin_unlock_irqrestore(fifo->lock, flags);
    return ret;
}
static inline unsigned int kfifo_get(struct kfifo *fifo, unsigned char *buffer, unsigned int len)
{
    unsigned long flags;
    unsigned int ret;
    spin_lock_irqrestore(fifo->lock, flags);
    ret = __kfifo_get(fifo, buffer, len);
    //当fifo->in == fifo->out时，buufer为空
//    if (fifo->in == fifo->out)
//        fifo->in = fifo->out = 0;
    spin_unlock_irqrestore(fifo->lock, flags);
    return ret;
}
static inline unsigned int kfifo_put_try(struct kfifo *fifo, const unsigned char *buffer, unsigned int len)
{
    unsigned long flags;
    unsigned int ret;
    spin_lock_irqrestore(fifo->lock, flags);
    ret = __kfifo_put_try(fifo, buffer, len);
    spin_unlock_irqrestore(fifo->lock, flags);
    return ret;
}
static inline unsigned int kfifo_get_try(struct kfifo *fifo, unsigned char *buffer, unsigned int len)
{
    unsigned long flags;
    unsigned int ret;
    spin_lock_irqrestore(fifo->lock, flags);
    ret = __kfifo_get_try(fifo, buffer, len);
    //当fifo->in == fifo->out时，buufer为空
//    if (fifo->in == fifo->out)
//        fifo->in = fifo->out = 0;
    spin_unlock_irqrestore(fifo->lock, flags);
    return ret;
}
static inline unsigned int kfifo_put_index(struct kfifo *fifo, unsigned int len)
{
    //unsigned long flags;
    unsigned int ret;
    //unsigned int l;
    //buffer中空的长度
    len = min(len, fifo->size - fifo->in + fifo->out);
    fifo->in += len;  //每次累加，到达最大值后溢出，自动转为0
    ret = len;
     return ret;
}
static inline unsigned int kfifo_get_index(struct kfifo *fifo, unsigned int len)
{
    unsigned int ret;
    len = min(len, fifo->in - fifo->out);
    fifo->out += len; //每次累加，到达最大值后溢出，自动转为0
    ret = len;
    //当fifo->in == fifo->out时，buufer为空
//    if (fifo->in == fifo->out)
//        fifo->in = fifo->out = 0;
    return ret;
}

static inline unsigned int kfifo_get_in_pointer(struct kfifo *fifo)
{
    return (unsigned int)(fifo->buffer + (fifo->in & (fifo->size - 1)));
}

static inline unsigned int kfifo_get_free_block(struct kfifo *fifo)
{
    return (unsigned int)(fifo->size - (fifo->out & (fifo->size - 1)));
}
static inline unsigned int kfifo_get_out_pointer(struct kfifo *fifo)
{

    return (unsigned int)(fifo->buffer + (fifo->out & (fifo->size - 1)));
}
#ifdef __cplusplus
};
#endif
#endif
