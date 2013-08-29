/*
 * tarfs.c -- the Simple Block Utility
 * Adapted from sbull.c as supplied in
 * Linux Device Drivers 3 & 2
 *
 */

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/malloc.h> /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/timer.h>
#include <linux/types.h>  /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>  /* HDIO_GETGEO */

#include <asm/system.h>   /* cli(), *_flags */

#define MAJOR_NR tarfs_major /* force definitions on in blk.h */
static int tarfs_major; /* must be declared before including blk.h */

#define DEVICE_NR(device) MINOR(device)   /* tarfs has no partition bits */
#define DEVICE_NAME "tarfs"               /* name for messaging */
#define DEVICE_INTR tarfs_intrptr         /* pointer to the bottom half */
#define DEVICE_NO_RANDOM                  /* no entropy to contribute */
#define DEVICE_REQUEST tarfs_request
#define DEVICE_OFF(d) /* do-nothing */

#include <linux/blk.h>

#include "tarfs.h"        /* local definitions */
#ifdef HAVE_BLKPG_H
#include <linux/blkpg.h>  /* blk_ioctl() */
#endif

/*
 * Do the raw char interface in 2.4.
 */
#ifdef LINUX_24
#  define DO_RAW_INTERFACE
#  include <linux/iobuf.h>
static void tarfsr_init();
static void tarfsr_release();
#  define TARFSR_SECTOR 512  /* insist on this */
#  define TARFSR_SECTOR_MASK (TARFSR_SECTOR - 1)
#  define TARFSR_SECTOR_SHIFT 9
#endif

/*
 * Non-prefixed symbols are static. They are meant to be assigned at
 * load time. Prefixed symbols are not static, so they can be used in
 * debugging. They are hidden anyways by register_symtab() unless
 * TARFS_DEBUG is defined.
 */
static int major    = TARFS_MAJOR;
static int devs     = TARFS_DEVS;
static int rahead   = TARFS_RAHEAD;
static int size     = TARFS_SIZE;
static int blksize  = TARFS_BLKSIZE;
static int hardsect = TARFS_HARDSECT;

MODULE_PARM(major, "i");
MODULE_PARM(devs, "i");
MODULE_PARM(rahead, "i");
MODULE_PARM(size, "i");
MODULE_PARM(blksize, "i");
MODULE_PARM(hardsect, "i");
MODULE_AUTHOR("Alessandro Rubini");


int tarfs_devs, tarfs_rahead, tarfs_size;
int tarfs_blksize, tarfs_hardsect;

/* The following items are obtained through kmalloc() in tarfs_init() */

Tarfs_Dev *tarfs_devices = NULL;
int *tarfs_blksizes = NULL;
int *tarfs_sizes = NULL;
int *tarfs_hardsects = NULL;

/*
 * We can do without a request queue, but only in 2.4
 */
#if defined(LINUX_24) && !defined(TARFS_MULTIQUEUE)
static int noqueue = 0;         /* Use request queue by default */
MODULE_PARM(noqueue, "i");
#endif

#ifdef DO_RAW_INTERFACE
int tarfsr_major = TARFSR_MAJOR;
MODULE_PARM(tarfsr_major, "i");
#endif


int tarfs_revalidate(kdev_t i_rdev);


/*
 * Open and close
 */

int tarfs_open (struct inode *inode, struct file *filp)
{
    Tarfs_Dev *dev; /* device information */
    int num = MINOR(inode->i_rdev);

    if (num >= tarfs_devs) return -ENODEV;
    dev = tarfs_devices + num;
    /* kill the timer associated to the device: it might be active */
    del_timer(&dev->timer);


    spin_lock(&dev->lock);
    /* revalidate on first open and fail if no data is there */
    if (!dev->usage) {
        check_disk_change(inode->i_rdev);
        if (!dev->data)
        {
            spin_unlock (&dev->lock);
            return -ENOMEM;
        }
    }
    dev->usage++;
    spin_unlock(&dev->lock);
    MOD_INC_USE_COUNT;
    return 0;          /* success */
}

int tarfs_release (struct inode *inode, struct file *filp)
{
    Tarfs_Dev *dev = tarfs_devices + MINOR(inode->i_rdev);

    spin_lock(&dev->lock);
    dev->usage--;
    /*
     * If the device is closed for the last time, start a timer
     * to release RAM in half a minute. The function and argument
     * for the timer have been setup in tarfs_init()
     */
    if (!dev->usage) {
        dev->timer.expires = jiffies + 30 * HZ;
        add_timer(&dev->timer);
        /* but flush it right now */
        fsync_dev(inode->i_rdev);
        invalidate_buffers(inode->i_rdev);
    }
    MOD_DEC_USE_COUNT;
    spin_unlock(&dev->lock);
    return 0;
}


/*
 * The timer function. As argument it receives the device
 */
void tarfs_expires(unsigned long data)
{
    Tarfs_Dev *dev = (Tarfs_Dev *)data;

    spin_lock(&dev->lock);
    if (dev->usage || !dev->data) {
        spin_unlock(&dev->lock);
        printk(KERN_WARNING "tarfs: timer mismatch for device %i\n",
               dev - tarfs_devices);
        return;
    }
    PDEBUG("freeing device %i\n",dev - tarfs_devices);
    vfree(dev->data);
    dev->data=0;
    spin_unlock(&dev->lock);
    return;
}

/*
 * The ioctl() implementation
 */

int tarfs_ioctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
    int err;
    long size;
    struct hd_geometry geo;

    PDEBUG("ioctl 0x%x 0x%lx\n", cmd, arg);
    switch(cmd) {

      case BLKGETSIZE:
        /* Return the device size, expressed in sectors */
        if (!arg) return -EINVAL; /* NULL pointer: not valid */
        err = ! access_ok (VERIFY_WRITE, arg, sizeof(long));
        if (err) return -EFAULT;
        size = blksize*tarfs_sizes[MINOR(inode->i_rdev)]
		/ tarfs_hardsects[MINOR(inode->i_rdev)];
	if (copy_to_user((long *) arg, &size, sizeof (long)))
	    return -EFAULT;
        return 0;

      case BLKRRPART: /* re-read partition table: can't do it */
        return -ENOTTY;

      case HDIO_GETGEO:
        /*
	 * Get geometry: since we are a virtual device, we have to make
	 * up something plausible.  So we claim 16 sectors, four heads,
	 * and calculate the corresponding number of cylinders.  We set the
	 * start of data at sector four.
         */
        err = ! access_ok(VERIFY_WRITE, arg, sizeof(geo));
        if (err) return -EFAULT;
        size = tarfs_size * blksize / tarfs_hardsect;
        geo.cylinders = (size & ~0x3f) >> 6;
	geo.heads = 4;
	geo.sectors = 16;
	geo.start = 4;
	if (copy_to_user((void *) arg, &geo, sizeof(geo)))
	    return -EFAULT;
        return 0;

      default:
        /*
         * For ioctls we don't understand, let the block layer handle them.
         */
        return blk_ioctl(inode->i_rdev, cmd, arg);
    }

    return -ENOTTY; /* unknown command */
}

/*
 * Support for removable devices
 */

int tarfs_check_change(kdev_t i_rdev)
{
    int minor = MINOR(i_rdev);
    Tarfs_Dev *dev = tarfs_devices + minor;

    if (minor >= tarfs_devs) /* paranoid */
        return 0;

    PDEBUG("check_change for dev %i\n",minor);
    if (dev->data)
        return 0; /* still valid */
    return 1; /* expired */
}


/*
 * Note no locks taken out here.  In a worst case scenario, we could drop
 * a chunk of system memory.  But that should never happen, since validation
 * happens at open or mount time, when locks are held.
 */
int tarfs_revalidate(kdev_t i_rdev)
{
    Tarfs_Dev *dev = tarfs_devices + MINOR(i_rdev);

    PDEBUG("revalidate for dev %i\n",MINOR(i_rdev));
    if (dev->data)
        return 0;
    dev->data = vmalloc(dev->size);
    if (!dev->data)
        return -ENOMEM;
    return 0;
}

/*
 * The file operations
 */

#ifdef LINUX_24
struct block_device_operations tarfs_bdops = {
    open:               tarfs_open,
    release:            tarfs_release,
    ioctl:              tarfs_ioctl,
    check_media_change: tarfs_check_change,
    revalidate:         tarfs_revalidate,
};
#else

#ifdef LINUX_20
void tarfs_release_20 (struct inode *inode, struct file *filp)
{
        (void) tarfs_release (inode, filp);
}
#define tarfs_release tarfs_release_20
#endif

struct file_operations tarfs_bdops = {
    read:       block_read,
    write:      block_write,
    ioctl:      tarfs_ioctl,
    open:       tarfs_open,
    release:    tarfs_release,
    fsync:      block_fsync,
    check_media_change: tarfs_check_change,
    revalidate: tarfs_revalidate
};
# endif /* LINUX_24 */




/*
 * Block-driver specific functions
 */

/*
 * Find the device for this request.
 */
static Tarfs_Dev *tarfs_locate_device(const struct request *req)
{
    int devno;
    Tarfs_Dev *device;

    /* Check if the minor number is in range */
    devno = DEVICE_NR(req->rq_dev);
    if (devno >= tarfs_devs) {
        static int count = 0;
        if (count++ < 5) /* print the message at most five times */
            printk(KERN_WARNING "tarfs: request for unknown device\n");
        return NULL;
    }
    device = tarfs_devices + devno;  /* Pick it out of our device array */
    return device;
}


/*
 * Perform an actual transfer.
 */
static int tarfs_transfer(Tarfs_Dev *device, const struct request *req)
{
    int size;
    u8 *ptr;

    ptr = device->data + req->sector * tarfs_hardsect;
    size = req->current_nr_sectors * tarfs_hardsect;

    /* Make sure that the transfer fits within the device. */
    if (ptr + size > device->data + tarfs_blksize*tarfs_size) {
        static int count = 0;
        if (count++ < 5)
            printk(KERN_WARNING "tarfs: request past end of device\n");
        return 0;
    }

    /* Looks good, do the transfer. */
    switch(req->cmd) {
        case READ:
            memcpy(req->buffer, ptr, size); /* from tarfs to buffer */
            return 1;
        case WRITE:
            memcpy(ptr, req->buffer, size); /* from buffer to tarfs */
            return 1;
        default:
            /* can't happen */
            return 0;
    }
}


#ifdef LINUX_24

/*
 * Transfer a buffer directly, without going through the request queue.
 */
int tarfs_make_request(request_queue_t *queue, int rw, struct buffer_head *bh)
{
    u8 *ptr;

    /* Figure out what we are doing */
    Tarfs_Dev *device = tarfs_devices + MINOR(bh->b_rdev);
    ptr = device->data + bh->b_rsector * tarfs_hardsect;

    /* Paranoid check, this apparently can really happen */
    if (ptr + bh->b_size > device->data + tarfs_blksize*tarfs_size) {
        static int count = 0;
        if (count++ < 5)
            printk(KERN_WARNING "tarfs: request past end of device\n");
        bh->b_end_io(bh, 0);
        return 0;
    }

    /* This could be a high memory buffer, shift it down */
#if CONFIG_HIGHMEM
    bh = create_bounce(rw, bh);
#endif

    /* Do the transfer */
    switch(rw) {
        case READ:
        case READA:  /* Readahead */
            memcpy(bh->b_data, ptr, bh->b_size); /* from tarfs to buffer */
            bh->b_end_io(bh, 1);
            break;
        case WRITE:
            refile_buffer(bh);
            memcpy(ptr, bh->b_data, bh->b_size); /* from buffer to tarfs */
            mark_buffer_uptodate(bh, 1);
            bh->b_end_io(bh, 1);
            break;
        default:
            /* can't happen */
            bh->b_end_io(bh, 0);
            break;
    }

    /* Nonzero return means we're done */
    return 0;
}


void tarfs_unused_request(request_queue_t *q)
{
    static int count = 0;
    if (count++ < 5)
        printk(KERN_WARNING "tarfs: unused_request called\n");
}

#endif /* LINUX_24 */



#if defined(TARFS_EMPTY_REQUEST)
/*
 * This empty request function just prints the interesting items
 * of the current request. The sectors affected by the request
 * are printed as <first-sector>-<number-of-sectors>.
 */
#ifdef LINUX_24
void tarfs_request(request_queue_t *q)
#else
void tarfs_request()
#endif
{
    while(1) {
        INIT_REQUEST;
        printk("<1>request %p: cmd %i sec %li (nr. %li)\n", CURRENT,
               CURRENT->cmd,
               CURRENT->sector,
               CURRENT->current_nr_sectors);
        end_request(1); /* success */
    }
}


#elif defined(TARFS_MULTIQUEUE)  /* 2.4 only */

/*
 * Clean up this request.
 */
int tarfs_end_request(struct request *req, int status)
{
    if (end_that_request_first(req, status, DEVICE_NAME))
        return 1;
    end_that_request_last(req);
    return 0;
}



void tarfs_request(request_queue_t *q)
{
    Tarfs_Dev *device;
    struct request *req;
    int status;

    /* Find our device */
    device = tarfs_locate_device (blkdev_entry_next_request(&q->queue_head));
    if (device->busy) /* no race here - io_request_lock held */
        return;
    device->busy = 1;

    /* Process requests in the queue */
    while(! list_empty(&q->queue_head)) {

    /* Pull the next request off the list. */
        req = blkdev_entry_next_request(&q->queue_head);
        blkdev_dequeue_request(req);
        spin_unlock_irq (&io_request_lock);
        spin_lock(&device->lock);

    /* Process all of the buffers in this (possibly clustered) request. */
        do {
            status = tarfs_transfer(device, req);
        } while (end_that_request_first(req, status, DEVICE_NAME));
        spin_unlock(&device->lock);
        spin_lock_irq (&io_request_lock);
        end_that_request_last(req);
    }
    device->busy = 0;
}

/*
 * Tell the block layer where to queue a request.
 */
request_queue_t *tarfs_find_queue(kdev_t device)
{
    int devno = DEVICE_NR(device);

    if (devno >= tarfs_devs) {
        static int count = 0;
        if (count++ < 5) /* print the message at most five times */
            printk(KERN_WARNING "tarfs: request for unknown device\n");
        return NULL;
    }
    return &tarfs_devices[devno].queue;
}



#else /* not TARFS_MULTIQUEUE */
#ifdef LINUX_24
void tarfs_request(request_queue_t *q)
#else
void tarfs_request()
#endif
{
    Tarfs_Dev *device;
    int status;

    while(1) {
        INIT_REQUEST;  /* returns when queue is empty */

        /* Which "device" are we using? */
        device = tarfs_locate_device (CURRENT);
        if (device == NULL) {
            end_request(0);
            continue;
        }

        /* Perform the transfer and clean up. */
	spin_lock(&device->lock);
        status = tarfs_transfer(device, CURRENT);
        spin_unlock(&device->lock);
        end_request(status);
    }
}
#endif /* not TARFS_EMPTY_REQUEST nor TARFS_MULTIQUEUE */

/*
 * Finally, the module stuff
 */

int tarfs_init(void)
{
    int result, i;

    /*
     * Copy the (static) cfg variables to public prefixed ones to allow
     * snoozing with a debugger.
     */

    tarfs_major    = major;
    tarfs_devs     = devs;
    tarfs_rahead   = rahead;
    tarfs_size     = size;
    tarfs_blksize  = blksize;
    tarfs_hardsect = hardsect;

#ifdef LINUX_20
    /* Hardsect can't be changed :( */
    if (hardsect != 512) {
        printk(KERN_ERR "tarfs: can't change hardsect size\n");
        hardsect = tarfs_hardsect = 512;
    }
#endif
    /*
     * Register your major, and accept a dynamic number
     */
    result = register_blkdev(tarfs_major, "tarfs", &tarfs_bdops);
    if (result < 0) {
        printk(KERN_WARNING "tarfs: can't get major %d\n",tarfs_major);
        return result;
    }
    if (tarfs_major == 0) tarfs_major = result; /* dynamic */
    major = tarfs_major; /* Use `major' later on to save typing */

    /*
     * Assign the other needed values: request, rahead, size, blksize,
     * hardsect. All the minor devices feature the same value.
     * Note that `tarfs' defines all of them to allow testing non-default
     * values. A real device could well avoid setting values in global
     * arrays if it uses the default values.
     */

    read_ahead[major] = tarfs_rahead;
    result = -ENOMEM; /* for the possible errors */

    tarfs_sizes = kmalloc(tarfs_devs * sizeof(int), GFP_KERNEL);
    if (!tarfs_sizes)
        goto fail_malloc;
    for (i=0; i < tarfs_devs; i++) /* all the same size */
        tarfs_sizes[i] = tarfs_size;
    blk_size[major]=tarfs_sizes;

    tarfs_blksizes = kmalloc(tarfs_devs * sizeof(int), GFP_KERNEL);
    if (!tarfs_blksizes)
        goto fail_malloc;
    for (i=0; i < tarfs_devs; i++) /* all the same blocksize */
        tarfs_blksizes[i] = tarfs_blksize;
    blksize_size[major]=tarfs_blksizes;

    tarfs_hardsects = kmalloc(tarfs_devs * sizeof(int), GFP_KERNEL);
    if (!tarfs_hardsects)
        goto fail_malloc;
    for (i=0; i < tarfs_devs; i++) /* all the same hardsect */
        tarfs_hardsects[i] = tarfs_hardsect;
    hardsect_size[major]=tarfs_hardsects;

    /* FIXME: max_readahead and max_sectors */

    /*
     * allocate the devices -- we can't have them static, as the number
     * can be specified at load time
     */

    tarfs_devices = kmalloc(tarfs_devs * sizeof (Tarfs_Dev), GFP_KERNEL);
    if (!tarfs_devices)
        goto fail_malloc;
    memset(tarfs_devices, 0, tarfs_devs * sizeof (Tarfs_Dev));
    for (i=0; i < tarfs_devs; i++) {
        /* data and usage remain zeroed */
        tarfs_devices[i].size = 1024 * tarfs_size;
        init_timer(&(tarfs_devices[i].timer));
        tarfs_devices[i].timer.data = (unsigned long)(tarfs_devices+i);
        tarfs_devices[i].timer.function = tarfs_expires;
        spin_lock_init(&tarfs_devices[i].lock);
    }

    /*
     * Get the queue set up, and register our (nonexistent) partitions.
     */
#ifdef TARFS_MULTIQUEUE
    for (i = 0; i < tarfs_devs; i++) {
        blk_init_queue(&tarfs_devices[i].queue, tarfs_request);
        blk_queue_headactive(&tarfs_devices[i].queue, 0);
    }
    blk_dev[major].queue = tarfs_find_queue;
#else
#  ifdef LINUX_24
    if (noqueue)
        blk_queue_make_request(BLK_DEFAULT_QUEUE(major), tarfs_make_request);
    else
#  endif /* LINUX_24 */
        blk_init_queue(BLK_DEFAULT_QUEUE(major), tarfs_request);
#endif
    /* A no-op in 2.4.0, but all drivers seem to do it anyway */
    for (i = 0; i < tarfs_devs; i++)
            register_disk(NULL, MKDEV(major, i), 1, &tarfs_bdops,
                            tarfs_size << 1);

#ifndef TARFS_DEBUG
    EXPORT_NO_SYMBOLS; /* otherwise, leave global symbols visible */
#endif

    printk ("<1>tarfs: init complete, %d devs, size %d blks %d hs %d\n",
                    tarfs_devs, tarfs_size, tarfs_blksize, tarfs_hardsect);
#ifdef TARFS_MULTIQUEUE
    printk ("<1>tarfs: Using multiqueue request\n");
#elif defined(LINUX_24)
    if (noqueue)
            printk (KERN_INFO "tarfs: using direct make_request\n");
#endif

#ifdef DO_RAW_INTERFACE
    tarfsr_init();
#endif
    return 0; /* succeed */

  fail_malloc:
    read_ahead[major] = 0;
    if (tarfs_sizes) kfree(tarfs_sizes);
    blk_size[major] = NULL;
    if (tarfs_blksizes) kfree(tarfs_blksizes);
    blksize_size[major] = NULL;
    if (tarfs_hardsects) kfree(tarfs_hardsects);
    hardsect_size[major] = NULL;
    if (tarfs_devices) kfree(tarfs_devices);

    unregister_blkdev(major, "tarfs");
    return result;
}

void tarfs_cleanup(void)
{
    int i;

/*
 * Before anything else, get rid of the timer functions.  Set the "usage"
 * flag on each device as well, under lock, so that if the timer fires up
 * just before we delete it, it will either complete or abort.  Otherwise
 * we have nasty race conditions to worry about.
 */
    for (i = 0; i < tarfs_devs; i++) {
        Tarfs_Dev *dev = tarfs_devices + i;
        del_timer(&dev->timer);
        spin_lock(&dev->lock);
        dev->usage++;
        spin_unlock(&dev->lock);
    }

#ifdef DO_RAW_INTERFACE
    tarfsr_release();
#endif

    /* flush it all and reset all the data structures */


    for (i=0; i<tarfs_devs; i++)
        fsync_dev(MKDEV(tarfs_major, i)); /* flush the devices */
    unregister_blkdev(major, "tarfs");
/*
 * Fix up the request queue(s)
 */
#ifdef TARFS_MULTIQUEUE
    for (i = 0; i < tarfs_devs; i++)
            blk_cleanup_queue(&tarfs_devices[i].queue);
    blk_dev[major].queue = NULL;
#else
    blk_cleanup_queue(BLK_DEFAULT_QUEUE(major));
#endif

    /* Clean up the global arrays */
    read_ahead[major] = 0;
    kfree(blk_size[major]);
    blk_size[major] = NULL;
    kfree(blksize_size[major]);
    blksize_size[major] = NULL;
    kfree(hardsect_size[major]);
    hardsect_size[major] = NULL;
    /* FIXME: max_readahead and max_sectors */


    /* finally, the usual cleanup */
    for (i=0; i < tarfs_devs; i++) {
        if (tarfs_devices[i].data)
            vfree(tarfs_devices[i].data);
    }
    kfree(tarfs_devices);
}



/*
 * Below here is the "raw device" implementation, available only
 * in 2.4.
 */
#ifdef DO_RAW_INTERFACE

/*
 * Transfer an iovec
 */
static int tarfsr_rw_iovec(Tarfs_Dev *dev, struct kiobuf *iobuf, int rw,
                int sector, int nsectors)
{
    struct request fakereq;
    struct page *page;
    int offset = iobuf->offset, ndone = 0, pageno, result;

    /* Perform I/O on each sector */
    fakereq.sector = sector;
    fakereq.current_nr_sectors = 1;
    fakereq.cmd = rw;

    for (pageno = 0; pageno < iobuf->nr_pages; pageno++) {
        page = iobuf->maplist[pageno];
        while (ndone < nsectors) {
            /* Fake up a request structure for the operation */
            fakereq.buffer = (void *) (kmap(page) + offset);
            result = tarfs_transfer(dev, &fakereq);
	    kunmap(page);
            if (result == 0)
                return ndone;
            /* Move on to the next one */
            ndone++;
            fakereq.sector++;
            offset += TARFSR_SECTOR;
            if (offset >= PAGE_SIZE) {
                offset = 0;
                break;
            }
        }
    }
    return ndone;
}


/*
 * Handle actual transfers of data.
 */
static int tarfsr_transfer (Tarfs_Dev *dev, char *buf, size_t count,
                loff_t *offset, int rw)
{
    struct kiobuf *iobuf;
    int result;

    /* Only block alignment and size allowed */
    if ((*offset & TARFSR_SECTOR_MASK) || (count & TARFSR_SECTOR_MASK))
        return -EINVAL;
    if ((unsigned long) buf & TARFSR_SECTOR_MASK)
        return -EINVAL;

    /* Allocate an I/O vector */
    result = alloc_kiovec(1, &iobuf);
    if (result)
        return result;

    /* Map the user I/O buffer and do the I/O. */
    result = map_user_kiobuf(rw, iobuf, (unsigned long) buf, count);
    if (result) {
        free_kiovec(1, &iobuf);
        return result;
    }
    spin_lock(&dev->lock);
    result = tarfsr_rw_iovec(dev, iobuf, rw, *offset >> TARFSR_SECTOR_SHIFT,
                    count >> TARFSR_SECTOR_SHIFT);
    spin_unlock(&dev->lock);

    /* Clean up and return. */
    unmap_kiobuf(iobuf);
    free_kiovec(1, &iobuf);
    if (result > 0)
        *offset += result << TARFSR_SECTOR_SHIFT;
    return result << TARFSR_SECTOR_SHIFT;
}


/*
 * Read and write syscalls.
 */
ssize_t tarfsr_read(struct file *filp, char *buf, size_t size, loff_t *off)
{
    Tarfs_Dev *dev = tarfs_devices + MINOR(filp->f_dentry->d_inode->i_rdev);
    return tarfsr_transfer(dev, buf, size, off, READ);
}

ssize_t tarfsr_write(struct file *filp, const char *buf, size_t size,
                loff_t *off)
{
    Tarfs_Dev *dev = tarfs_devices + MINOR(filp->f_dentry->d_inode->i_rdev);
    return tarfsr_transfer(dev, (char *) buf, size, off, WRITE);
}


static int tarfsr_registered = 0;
static struct file_operations tarfsr_fops = {
   read:        tarfsr_read,
   write:       tarfsr_write,
   open:        tarfs_open,
   release:     tarfs_release,
   ioctl:	tarfs_ioctl,
};


static void tarfsr_init()
{
    int result;

    /* Simplify the math */
    if (tarfs_hardsect != TARFSR_SECTOR) {
        printk(KERN_NOTICE "Tarfsr requires hardsect = %d\n", TARFSR_SECTOR);
        return;
    }
    SET_MODULE_OWNER(&tarfsr_fops);
    result = register_chrdev(tarfsr_major, "tarfsr", &tarfsr_fops);
    if (result >= 0)
        tarfsr_registered = 1;
    if (tarfsr_major == 0)
        tarfsr_major = result;
}


static void tarfsr_release()
{
    if (tarfsr_registered)
        unregister_chrdev(tarfsr_major, "tarfsr");
}




#endif /* DO_RAW_INTERFACE */

module_init(tarfs_init);
module_exit(tarfs_cleanup);
