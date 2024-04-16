
#ifndef afifo_H
#define afifo_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "string.h"
#include "stdint.h"
#include "stdbool.h"
#define __NSX_PASTE__(A, B) A##B
#ifndef min
#define min(A, B) __NSMIN_IMPL__(A, B, __COUNTER__)
#endif
#define __NSMIN_IMPL__(A, B, L) ({                                                                   \
	__typeof__(A) __NSX_PASTE__(__a, L) = (A);                                                       \
	__typeof__(B) __NSX_PASTE__(__b, L) = (B);                                                       \
	(__NSX_PASTE__(__a, L) < __NSX_PASTE__(__b, L)) ? __NSX_PASTE__(__a, L) : __NSX_PASTE__(__b, L); \
})

	typedef struct
	{
		int TODO;
	} spinlock_t;
	// #define spin_lock_irqrestore(lock, flags) flags = (unsigned long)lock+1
	// #define spin_unlock_irqrestore(lock, flags) flags = (unsigned long)lock+1
	int spin_unlock_irqrestore(spinlock_t *lock, unsigned int flags);
	int spin_lock_irqrestore(spinlock_t *lock, unsigned int flags);

	typedef struct afifo
	{
		unsigned char *buffer; /* the buffer holding the data */
		unsigned int size;	   /* the size of the allocated buffer */
		unsigned int in;	   /* data is added at offset (in % size) */
		unsigned int out;	   /* data is extracted from off. (out % size) */
		spinlock_t *lock;	   /* protects concurrent modifications */
	} afifo_t;

	bool is_power_of_2(int n);
	unsigned int __afifo_in(struct afifo *fifo, const unsigned char *buffer, unsigned int len);
	unsigned int __afifo_out(struct afifo *fifo, unsigned char *buffer, unsigned int len);
	unsigned int __afifo_in_data(struct afifo *fifo, const unsigned char *buffer, unsigned int len);
	unsigned int __afifo_out_data(struct afifo *fifo, unsigned char *buffer, unsigned int len);

	static inline unsigned int afifo_get_used(struct afifo *afifo)
	{
		return (afifo->in - afifo->out);
	}
	static inline unsigned int afifo_get_free(struct afifo *afifo)
	{
		return afifo->size - afifo_get_used(afifo);
	}

	struct afifo *afifo_init(unsigned char *buffer, unsigned int size, spinlock_t *lock);

#define afifo_INIT(fifo, _size, _lock)            \
	do                                            \
	{                                             \
		static afifo_t fifo##_body;               \
		static uint8_t fifo##_buf[_size];         \
		fifo##_body.buffer = fifo##_buf;          \
		fifo##_body.size = _size;                 \
		(fifo##_body).in = (fifo##_body).out = 0; \
		(fifo##_body).lock = (_lock);             \
		fifo = &(fifo##_body);                    \
	} while (0)
// arm stm32f7/h7 use
#define afifo_alloc_static(pout, size, lock)                  \
	do                                                        \
	{                                                         \
		static uint8_t __##size##lock_buffer[size];           \
		pout = afifo_init(__##size##lock_buffer, size, lock); \
	} while (0)
#define afifo_alloc_static_section(pout, size, lock, section) \
	do                                                        \
	{                                                         \
		static uint8_t __##size##lock_buffer[size];           \
		pout = afifo_init(__##size##lock_buffer, size, lock); \
	} while (0)

	struct afifo *afifo_alloc(unsigned int size, spinlock_t *lock);

	static inline unsigned int afifo_in_data(struct afifo *fifo, const unsigned char *buffer, unsigned int len)
	{
		unsigned long flags;
		unsigned int ret;
		spin_lock_irqrestore(fifo->lock, flags);
		ret = __afifo_in_data(fifo, buffer, len);
		spin_unlock_irqrestore(fifo->lock, flags);
		return ret;
	}
	static inline unsigned int afifo_add_in(struct afifo *fifo, unsigned int len)
	{
		// unsigned long flags;
		unsigned int ret;
		// unsigned int l;
		// buffer中空的长度
		len = min(len, fifo->size - fifo->in + fifo->out);
		fifo->in += len; // 每次累加，到达最大值后溢出，自动转为0
		ret = len;
		return ret;
	}
	static inline unsigned int afifo_in(struct afifo *fifo, const unsigned char *buffer, unsigned int len)
	{
		unsigned long flags;
		unsigned int ret;
		spin_lock_irqrestore(fifo->lock, flags);
		ret = __afifo_in(fifo, buffer, len);
		spin_unlock_irqrestore(fifo->lock, flags);
		return ret;
	}

	static inline unsigned int afifo_out_data(struct afifo *fifo, unsigned char *buffer, unsigned int len)
	{
		unsigned long flags;
		unsigned int ret;
		spin_lock_irqrestore(fifo->lock, flags);
		ret = __afifo_out_data(fifo, buffer, len);

		spin_unlock_irqrestore(fifo->lock, flags);
		return ret;
	}

	static inline unsigned int afifo_add_out(struct afifo *fifo, unsigned int len)
	{
		unsigned int ret;
		len = min(len, fifo->in - fifo->out);
		fifo->out += len; // 每次累加，到达最大值后溢出，自动转为0
		ret = len;
		return ret;
	}
	
	static inline unsigned int afifo_out(struct afifo *fifo, unsigned char *buffer, unsigned int len)
	{
		unsigned long flags;
		unsigned int ret;
		spin_lock_irqrestore(fifo->lock, flags);
		ret = __afifo_out(fifo, buffer, len);

		spin_unlock_irqrestore(fifo->lock, flags);
		return ret;
	}
	/**
	 * @brief 获取in在数组的地址
	 * 
	 * @param fifo 
	 * @return uint8_t* 
	 */
	static inline uint8_t *afifo_get_in_pointer(struct afifo *fifo)
	{
		return (fifo->buffer + (fifo->in & (fifo->size - 1)));
	}
	static inline uint8_t *afifo_get_out_pointer(struct afifo *fifo)
	{
		return (fifo->buffer + (fifo->out & (fifo->size - 1)));
	}
	/**
	 * @brief 获取剩余的连续的空间
	 *
	 * @param fifo
	 * @return unsigned int
	 */
	static inline unsigned int afifo_get_free_block(struct afifo *fifo)
	{
		return (unsigned int)(fifo->size - (fifo->out & (fifo->size - 1)));
	}
	/**
	 * @brief 获取fifo的大小
	 *
	 * @param fifo
	 * @return unsigned int
	 */
	static inline unsigned int afifo_size(struct afifo *fifo)
	{
		return (unsigned int)(fifo->size);
	}
	/**
	 * @brief 重置fifo
	 *
	 * @param fifo
	 */
	static inline void afifo_reset(struct afifo *fifo)
	{
		unsigned long flags;
		spin_lock_irqrestore(fifo->lock, flags);
		fifo->in = fifo->out = 0;
		spin_unlock_irqrestore(fifo->lock, flags);
	}

#ifdef __cplusplus
};
#endif
#endif
