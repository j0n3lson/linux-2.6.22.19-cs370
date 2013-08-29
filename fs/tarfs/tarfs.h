
/*
 * tarfs.h -- definitions for the char module
 * Adapted from sbull.h as supplied in
 * Linux Device Drivers 3 & 2
 */


#include <linux/ioctl.h>

/* version dependencies have been confined to a separate file */

#include "sysdep.h"

/* Multiqueue only works on 2.4 */
#ifdef TARFS_MULTIQUEUE
#  ifndef LINUX_24
#    warning "Multiqueue only works on 2.4 kernels"
#    undef TARFS_MULTIQUEUE
#  endif
#endif

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef TARFS_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "tarfs: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */


#define TARFS_MAJOR 0       /* dynamic major by default */
#define TARFS_DEVS 2        /* two disks */
#define TARFS_RAHEAD 2      /* two sectors */
#define TARFS_SIZE 2048     /* two megs each */
#define TARFS_BLKSIZE 1024  /* 1k blocks */
#define TARFS_HARDSECT 512  /* 2.2 and 2.4 can used different values */

#define TARFSR_MAJOR 0      /* Dynamic major for raw device */
/*
 * The tarfs device is removable: if it is left closed for more than
 * half a minute, it is removed. Thus use a usage count and a
 * kernel timer
 */

typedef struct Tarfs_Dev {
   int size;
   int usage;
   struct timer_list timer;
   spinlock_t lock;
   u8 *data;
#ifdef TARFS_MULTIQUEUE
   request_queue_t queue;
   int busy;
#endif
}              Tarfs_Dev;
