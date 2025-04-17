#include "fs.h"
#include "disk.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

// Returns the number of dedicated inode blocks given the disk size in blocks
#define NUM_INODE_BLOCKS(disk_size_in_blocks) (1 + (disk_size_in_blocks / 10))

struct fs_superblock
{
    int magic;        // Magic bytes
    int nblocks;      // Size of the disk in number of blocks
    int ninodeblocks; // Number of blocks dedicated to inodes
    int ninodes;      // Number of dedicated inodes
};

struct fs_inode
{
    int isvalid;                    // 1 if valid (in use), 0 otherwise
    int size;                       // Size of file in bytes
    int direct[POINTERS_PER_INODE]; // Direct data block numbers (0 if invalid)
    int indirect;                   // Indirect data block number (0 if invalid)
};

union fs_block
{
    struct fs_superblock super;              // Superblock
    struct fs_inode inode[INODES_PER_BLOCK]; // Block of inodes
    int pointers[POINTERS_PER_BLOCK];        // Indirect block of direct data block numbers
    char data[DISK_BLOCK_SIZE];              // Data block
};

static union fs_block superblock;
static union fs_block null_block;

/**
 * + Freemap should be allocated in fs_mount (via malloc) 
 *   based on the number of blocks (nblocks) in your super block
 * 
 * + freemap can then be indexed like an array for determining whether or
 *   not a block is free... e.g.
 *      if(!freemap[123])
 *          printf("Block 123 is NOT free\n");
*/
static int *freemap = 0;

void fs_debug()
{
    union fs_block block;

    disk_read(0, block.data);

    printf("superblock:\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);
    for (int inode_block_number = 0; inode_block_number < block.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(inode_block_number + 1, block.data);
        
        //printf("Inode block number = %d \n", inode_block_number);
        for (int inode_num_in_block = 0; inode_num_in_block < INODES_PER_BLOCK; inode_num_in_block++){
            struct fs_inode inode = block.inode[inode_num_in_block]; 
            if (inode.isvalid == 1){
                printf("inode: %d\n", inode_block_number * INODES_PER_BLOCK + inode_num_in_block);

                printf("    size: %d bytes\n", inode.size);
                printf("    direct blocks: ");
                for (int direct_index = 0; direct_index < POINTERS_PER_INODE; direct_index++){
                    if (inode.direct[direct_index] != 0){
                        printf("%d ", inode.direct[direct_index]);
                    }
                }
                printf("\n");
                
                if (inode.indirect != 0){
                    union fs_block indirect_block;
                    disk_read(inode.indirect, indirect_block.data);

                    printf("    Indirect block: %d\n", inode.indirect);
                    printf("    Indirect data blocks: ");
                    for (int indirect_block_pointer_index = 0; indirect_block_pointer_index < POINTERS_PER_BLOCK; indirect_block_pointer_index ++){
                        if (indirect_block.pointers[indirect_block_pointer_index] != 0){
                            printf("%d ", indirect_block.pointers[indirect_block_pointer_index]);
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}

int fs_format()
{
    // clear all existing data
    memset(null_block.data, 0, DISK_BLOCK_SIZE);

    for (int i = 0; i < disk_size(); i ++){
        disk_write(i, null_block.data);
    }
    
    // setup super block
    superblock.super.magic = FS_MAGIC;
    superblock.super.nblocks = disk_size();
    superblock.super.ninodeblocks = NUM_INODE_BLOCKS(superblock.super.nblocks);   // #define NUM_INODE_BLOCKS(disk_size_in_blocks) (1 + (disk_size_in_blocks / 10))
    superblock.super.ninodes = 0; // no inodes at the start

    // write super blocks
    disk_write(0, superblock.data);

    return 1;
}

int fs_mount()
{
    return 0;
}

int fs_unmount()
{
    return 0;
}

int fs_create()
{
    return -1;
}

int fs_delete(int inumber)
{
    return 0;
}

int fs_getsize(int inumber)
{
    return -1;
}

int fs_read(int inumber, char *data, int length, int offset)
{
    return 0;
}

int fs_write(int inumber, const char *data, int length, int offset)
{
    return 0;
}
