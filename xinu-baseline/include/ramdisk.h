/* ramdisk.h - definitions for a ram disk */

/* Ram disk block size */

#define	RM_BLKSIZ	512		/* block size			*/
#define	RM_BLKS		8192	/* number of blocks		*/

struct	ramdisk	{
	char	disk[RM_BLKSIZ * RM_BLKS];
	};

extern	struct	ramdisk	Ram;
