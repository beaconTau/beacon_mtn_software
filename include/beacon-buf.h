#ifndef _BEACON_BUF_H
#define _BEACON_BUF_H

/** no-lock circular buffer
 *
 *
 * Cosmin Deaconu <cozzyd@kicp.uchicago.edu> 
 *
 * Supports one producer and one consumer. Implementation uses memory barriers, although 
 * they're probably not strictly necessary on a single-cpu ARM (or maybe they are since ARM has a 
 * different memory model than x86) 
 *
 **/


/* opaque type*/ 
struct beacon_buf; 
typedef struct beacon_buf beacon_buf_t; 

/* Initialize a new buffer. The buffer will have the listed maximum capacity and 
 * each member will have size member_size (should be sizeof()) whatever is stored in there  */

beacon_buf_t*  beacon_buf_init(size_t max_capacity, size_t member_size ); 

/* Retrieve the capacity of the buffer */ 
size_t beacon_buf_capacity(const beacon_buf_t *);

/* Query the current occupancy of the buffer (be careful, because the buffer is meant
 * to be used by different threads, the value can change very quickly with no action */
size_t beacon_buf_occupancy(const beacon_buf_t *); 

/* Retrieve a pointer that we can write to. This will block if the buffer is full. Must call
 * commit after to the buffer know it's ready */ 
void * beacon_buf_getmem(beacon_buf_t *);

/* Let the buffer know that you are ready */
void beacon_buf_commit(beacon_buf_t * ); 

/* This will copy memory into next available location (essentially combining getmem, memcpy and commit)  */
void beacon_buf_push(beacon_buf_t *, const void * mem); 

/* Copy data to destination, freeing the spot in the buffer. This will block on empty. 
 * If destination is NULL, memory will be allocated for it 
 * */ 
void* beacon_buf_pop(beacon_buf_t *, void * destination);

/* Deinits and frees the buffer.
 *
 * You probably should wait until it's empty if you don't want to lose anything! 
 **/ 
int beacon_buf_destroy(beacon_buf_t *);

#endif
