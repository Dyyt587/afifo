#include <errno.h>
#include "kfifo.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
void BUG_ON(bool b);

unsigned int roundup_pow_of_two(unsigned int size);


int spin_lock_irqrestore(spinlock_t* lock,unsigned int flags)
{
	flags = (unsigned int)lock+1;
	return flags;
}
int spin_unlock_irqrestore(spinlock_t* lock,unsigned int flags)
{
	flags = (unsigned int)lock+1;
		return flags;

}



bool is_power_of_2(int n) {
//    int i,j;
    if(n<=0)
        return false;
    if(n==1)
        return true;
    while(n!=1)
    {
        if(n%2==0)
            n=n/2;
        else
            return false;
    }
    return true;
}



void *kmalloc(size_t i) {
    return malloc(i);
}

void kfree(unsigned char *buffer) {
    free(buffer);
}
struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size, spinlock_t *lock)
{
    struct kfifo *fifo;
    /* size must be a power of 2 */
    BUG_ON(!is_power_of_2(size));
    fifo = kmalloc(sizeof(struct kfifo));
    if (!fifo)
        return ERR_PTR(-ENOMEM);
    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;
    fifo->lock = lock;

    return fifo;
}


struct kfifo *kfifo_alloc(unsigned int size, spinlock_t *lock)
{
    unsigned char *buffer;
    struct kfifo *ret;
    if (!is_power_of_2(size)) {
        BUG_ON(size > 0x80000000);
        size = roundup_pow_of_two(size);
    }
    buffer = kmalloc(size);
    if (!buffer)
        return ERR_PTR(-ENOMEM);
    ret = kfifo_init(buffer, size, lock);

    if (IS_ERR(ret))
        kfree(buffer);
    return ret;
}



static unsigned int fls(unsigned int x)
{
    unsigned int position;
    unsigned int i;
    if(0 != x)
    {
        for (i = (x >> 1), position = 0; i != 0; ++position)
            i >>= 1;
    }
    else
    {
        position = -1;
    }
    return position+1;
}
unsigned int roundup_pow_of_two(unsigned int size) {
    return 1UL << fls(size - 1);
}

void BUG_ON(bool b) {
    while(b){}
}


unsigned int __kfifo_put(struct kfifo *fifo, const unsigned char *buffer, unsigned int len)
{
    unsigned int l;
    //buffer中空的长度
    len = min(len, fifo->size - fifo->in + fifo->out);
    /*
     * Ensure that we sample the fifo->out index -before- we
     * start putting bytes into the kfifo.
     */
    // smp_mb();
    /* first put the data starting from fifo->in to buffer end */
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(fifo->buffer, buffer + l, len - l);

    /*
     * Ensure that we add the bytes to the kfifo -before-
     * we update the fifo->in index.
     */
    // smp_wmb();
    fifo->in += len;  //每次累加，到达最大值后溢出，自动转为0
    return len;
}

unsigned int __kfifo_get(struct kfifo *fifo, unsigned char *buffer, unsigned int len)
{
    unsigned int l;
    //有数据的缓冲区的长度
    len = min(len, fifo->in - fifo->out);
    /*
     * Ensure that we sample the fifo->in index -before- we
     * start removing bytes from the kfifo.
     */
    //  smp_rmb();
    /* first get the data from fifo->out until the end of the buffer */
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, fifo->buffer, len - l);
    /*
     * Ensure that we remove the bytes from the kfifo -before-
     * we update the fifo->out index.
     */
    // smp_mb();
    fifo->out += len; //每次累加，到达最大值后溢出，自动转为0
    return len;
}

unsigned int __kfifo_put_try(struct kfifo *fifo, const unsigned char *buffer, unsigned int len)
{
    unsigned int l;
    //buffer中空的长度
    len = min(len, fifo->size - fifo->in + fifo->out);
    /*
     * Ensure that we sample the fifo->out index -before- we
     * start putting bytes into the kfifo.
     */
    // smp_mb();
    /* first put the data starting from fifo->in to buffer end */
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(fifo->buffer, buffer + l, len - l);

    /*
     * Ensure that we add the bytes to the kfifo -before-
     * we update the fifo->in index.
     */
    // smp_wmb();
    //fifo->in += len;  //每次累加，到达最大值后溢出，自动转为0
    return len;
}

unsigned int __kfifo_get_try(struct kfifo *fifo, unsigned char *buffer, unsigned int len)
{
    unsigned int l;
    //有数据的缓冲区的长度
    len = min(len, fifo->in - fifo->out);
    /*
     * Ensure that we sample the fifo->in index -before- we
     * start removing bytes from the kfifo.
     */
    //  smp_rmb();
    /* first get the data from fifo->out until the end of the buffer */
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, fifo->buffer, len - l);
    /*
     * Ensure that we remove the bytes from the kfifo -before-
     * we update the fifo->out index.
     */
    // smp_mb();
    //fifo->out += len; //每次累加，到达最大值后溢出，自动转为0
    return len;
}

