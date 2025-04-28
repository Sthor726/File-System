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
static int disk_offset = 0;

void fs_debug()
{
    union fs_block block;

    disk_read(disk_offset + 0, block.data);

    printf("superblock:\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);
    for (int inode_block_number = 0; inode_block_number < block.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(disk_offset + inode_block_number + 1, block.data);
        
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
                    disk_read(disk_offset + inode.indirect, indirect_block.data);

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
    union fs_block temp_superblock;
    int found_magic_num = 0;
    for (int byte_offset = 0; byte_offset < disk_size(); byte_offset++){ // check all byte offsets to find wher the file system starts
        disk_read(disk_offset, temp_superblock.data);

        if (temp_superblock.super.magic == FS_MAGIC){
            printf("found magic number at %d\n", byte_offset);
            disk_offset = byte_offset;
            found_magic_num = 1;
            break;
        }
    }
    if (found_magic_num){
    superblock = temp_superblock;
    } else {
        printf("Could not find magic number on disk\n");
        return 0;
    }


    // intalise based on the files on disk a free block bitmap
    freemap = malloc(sizeof(int) * superblock.super.nblocks);
    memset(freemap, 0, sizeof(freemap));
    for (int i = 0; i <superblock.super.nblocks; i++){
        if (freemap[i]){
            printf("NOT INITALIZED CORRECTLY\n"); //XXXXXXXXXXX
        }
    }
    union fs_block block;
    for (int inode_block_number = 0; inode_block_number < temp_superblock.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(disk_offset + inode_block_number + 1, block.data);
        
        //printf("Inode block number = %d \n", inode_block_number);
        for (int inode_num_in_block = 0; inode_num_in_block < INODES_PER_BLOCK; inode_num_in_block++){
            struct fs_inode inode = block.inode[inode_num_in_block]; 
            if (inode.isvalid == 1){
                // printf("inode: %d\n", inode_block_number * INODES_PER_BLOCK + inode_num_in_block);
                freemap[inode_block_number * INODES_PER_BLOCK + inode_num_in_block] = 1;
                // printf("    size: %d bytes\n", inode.size);
                // printf("    direct blocks: ");
                for (int direct_index = 0; direct_index < POINTERS_PER_INODE; direct_index++){
                    if (inode.direct[direct_index] != 0){
                        freemap[inode.direct[direct_index]] = 1;
                    }
                }
                // printf("\n");
                
                if (inode.indirect != 0){
                    union fs_block indirect_block;
                    disk_read(disk_offset + inode.indirect, indirect_block.data);

                    // printf("    Indirect block: %d\n", inode.indirect);
                    // printf("    Indirect data blocks: ");
                    freemap[inode.indirect] = 1;
                    for (int indirect_block_pointer_index = 0; indirect_block_pointer_index < POINTERS_PER_BLOCK; indirect_block_pointer_index ++){
                        if (indirect_block.pointers[indirect_block_pointer_index] != 0){
                            //printf("%d ", indirect_block.pointers[indirect_block_pointer_index]);
                            freemap[indirect_block.pointers[indirect_block_pointer_index]] = 1;
                        }
                    }
                    //printf("\n");
                }
            }
        }
    }
    
    


    return 1;
}

int fs_unmount()
{
// As needed, deallocate any structures allocated when mounting the file
// system. Returns one on success, zero otherwise.

    //deallocate free map
    if (freemap != NULL) {
    free(freemap);
    freemap = NULL;
    }

    // reset superblock and disk offset in memory
    memset(&superblock, 0, sizeof(superblock));
    disk_offset = 0;

    return 1;
}

int fs_create()
// Create a new inode of zero length. On success, return the inumber. On failure,
// return negative one.
{
    union fs_block block;
    for (int inode_block_number = 0; inode_block_number < superblock.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(disk_offset + inode_block_number + 1, block.data);
        
        for (int inode_num_in_block = 0; inode_num_in_block < INODES_PER_BLOCK; inode_num_in_block++){
            struct fs_inode *inode = &block.inode[inode_num_in_block];
            if (inode->isvalid == 0) {
                inode->isvalid = 1;
                inode->size = 0;
                memset(inode->direct, 0, sizeof(inode->direct));
                inode->indirect = 0;
            
                // write to disk and return inumber
                disk_write(disk_offset + inode_block_number + 1, block.data);
                return (inode_block_number * INODES_PER_BLOCK + inode_num_in_block);
            }

        }
        // no avalable space for inode
        printf("inode table full, unable to create inode");
        return -1;
    }
    return -1;
}

int fs_delete(int inumber)
{
// Delete the inode indicated by the inumber. Release all data and indirect blocks
// assigned to this inode and return them to the free block map. On success, return one. On
// failure, return 0
    union fs_block block;
    for (int inode_block_number = 0; inode_block_number < superblock.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(disk_offset + inode_block_number + 1, block.data);
        for (int inode_num_in_block = 0; inode_num_in_block < INODES_PER_BLOCK; inode_num_in_block++){
            if (inumber == (inode_block_number * INODES_PER_BLOCK + inode_num_in_block)){
                struct fs_inode *inode = &block.inode[inode_num_in_block];
                printf("found inode %d\n", inumber);
                if(inode->isvalid == 0){
                    printf("inode %d not found valid\n", inumber);
                    return 0;
                }

                // clear direct pointers on freemap
                for (int i = 0; i < POINTERS_PER_INODE; i++) {
                    if (inode->direct[i] != 0) {
                        freemap[inode->direct[i]] = 0;
                        inode->direct[i] = 0;
                    }
                }

                if (inode->indirect != 0) {
                    union fs_block indirect_pointer;
                    disk_read(inode->indirect, indirect_pointer.data);
                    for (int i = 0; i < POINTERS_PER_BLOCK; i++) {
                        if (indirect_pointer.pointers[i] != 0) {
                            freemap[indirect_pointer.pointers[i]] = 0;        
                            indirect_pointer.pointers[i] = 0;
                        }
                    }
                    freemap[inode->indirect] = 0;
                    inode->indirect = 0;
                }


                // reset state variables
                inode->isvalid = 0;
                inode->size = 0;
                
                disk_write(disk_offset + inode_block_number + 1, block.data);

                return 1;
            }
        }
    }
    printf("inode %d not found on disk\n", inumber);
    return 0;
}

int fs_getsize(int inumber)
{
// Return the logical size of the given inode, in bytes. Note that zero is a valid
// logical size for an inode! On failure, return -1
    struct fs_inode inode;

    inode_load(inumber, &inode);
    printf("loaded inode %d\n", inumber);
    if(inode.isvalid == 0){
        printf("inode %d not valid\n", inumber);
        return -1;
    }
    return inode.size;
}

int fs_read(int inumber, char *data, int length, int offset)
{
// Read data from a valid inode. Copy "length" bytes from the inode into the
// "data" pointer, starting at "offset" in the inode. Return the total number of bytes read. The
// number of bytes actually read could be smaller than the number of bytes requested,
// perhaps if the end of the inode is reached. If the given inumber is invalid, or any other
// error is encountered, return 0.
    struct fs_inode inode;
    inode_load(inumber, &inode);

    if (inode.isvalid == 0) {
        return 0;
    }

    if (offset >= inode.size) {
        return 0;
    }

    if (offset + length > inode.size) {
        length = inode.size - offset;
    }

    int bytes_read = 0;
    int block_index = offset / DISK_BLOCK_SIZE;
    int block_offset = offset % DISK_BLOCK_SIZE;

    union fs_block indirect_block;
    int has_indirect = 0;
    if (inode.indirect != 0) {
        disk_read(disk_offset + inode.indirect, indirect_block.data);
        has_indirect = 1;
    }

    while (bytes_read < length) {
        int block_num = 0;

        if (block_index < POINTERS_PER_INODE) {
            // Direct block
            block_num = inode.direct[block_index];
        } else if (has_indirect) {
            // Indirect block
            block_num = indirect_block.pointers[block_index - POINTERS_PER_INODE];
        } else {
            // No more blocks
            break;
        }

        if (block_num == 0) {
            break;
        }

        union fs_block block;
        disk_read(disk_offset + block_num, block.data);

        int chunk_size = DISK_BLOCK_SIZE - block_offset;
        if (chunk_size > (length - bytes_read)) {
            chunk_size = length - bytes_read;
        }

        memcpy(data + bytes_read, block.data + block_offset, chunk_size);

        bytes_read += chunk_size;
        block_index++;
        block_offset = 0;
    }

    return bytes_read;
}


int fs_write(int inumber, const char *data, int length, int offset)
{
// Write data to a valid inode. Copy "length" bytes from the pointer "data" into
// the inode starting at "offset" bytes. Allocate any necessary direct and indirect blocks in
// the process. Return the number of bytes actually written. The number of bytes actually
// written could be smaller than the number of bytes request, perhaps if the disk becomes
// full. If the given inumber is invalid, or any other error is encountered, return 0

    return 0;
}

void inode_load( int inumber, struct fs_inode *inode ) {
    union fs_block block;
    for (int inode_block_number = 0; inode_block_number < superblock.super.ninodeblocks; inode_block_number++){
        // read the inode block. This will have INODES_PER_BLOCK inodes wihtin it 
        disk_read(disk_offset + inode_block_number + 1, block.data);
        for (int inode_num_in_block = 0; inode_num_in_block < INODES_PER_BLOCK; inode_num_in_block++){
            if (inumber == (inode_block_number * INODES_PER_BLOCK + inode_num_in_block)){
                *inode = block.inode[inode_num_in_block];
                return;
            }
        }
    }
}
void inode_save( int inumber, struct fs_inode *inode ){
    return;
}

