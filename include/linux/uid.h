/* inlcude/linux/uid.h
 * struct definition for the Fair Scheduler 
 * CS370 Project 4
 * Authors: Joe Nelson
 *          Ken Fox
 * 
 * Comments - table size is arbitrary
 */

#ifndef __UID_H__
#define __UID_H__

#define MAX_TAB_SZ 2000

struct uid_acct{
    spinlock_t lock;
    uid_t *uid_tab;
    int nr_users;
};

#endif
