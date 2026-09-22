/*
 * Minimal scull.h for Assignment 7
 * Matches the simplified modern SCULL driver
 */

#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/cdev.h>

/* Default values */
#define SCULL_MAJOR     0
#define SCULL_NR_DEVS   4

/* Minimal device structure */
struct scull_dev {
    char *data;              /* data buffer */
    size_t size;             /* amount of data stored */
    struct cdev cdev;        /* char device structure */
};

#endif /* _SCULL_H_ */
