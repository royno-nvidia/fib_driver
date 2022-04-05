#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/fs>
#include <linux/pagemap.h>
#include <linux/dcache.h>

#define DRIVER_LICENCE "GPL"
#define DRIVER_AUTHOR "Roy Novich"
#define DRIVER_VERSION  "1.0-0"
#define DRIVER_DESCRIPTION "Fibonacci number generator"


