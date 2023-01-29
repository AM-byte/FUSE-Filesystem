#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "storage.h"
#include "blocks.h"
#include "inode.h"
#include "bitmap.h"
#include "directory.h"
#include "slist.h"

/**
 * initial setup for blocks and directories
 */
void storage_init(const char *path) {
  // initializing the blocks
  blocks_init(path);
  void* inodeBitmap = get_inode_bitmap();

  // if the given file does not exist
  if (bitmap_get(inodeBitmap, 0) == 0) {
    // initialize it
    directory_init();
  }
}

/**
 * reads the given size of bytes into the given buffer
*/
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  // getting the inum of the last element
  int inumThis = tree_lookup(path);

  // return erro on failure (file not found)
  if (inumThis < 0) {
    return -ENONET;
  }

  // getting the inode and reading from it
  inode_t* inodeThis = get_inode(inumThis);
  void* dataThis = blocks_get_block(inodeThis->block);
  dataThis = dataThis + offset;
  memcpy(buf, dataThis, size);

  return 4096;
}

/**
 * write the given size of bytes into the given buffer
*/
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
  // getting the inum of the last element
  int inumThis = tree_lookup(path);

  // return erro on failure (file not found)
  if (inumThis < 0) {
    return -ENONET;
  }

  // getting the inode and writing to it
  inode_t* inodeThis = get_inode(inumThis);
  void* dataThis = blocks_get_block(inodeThis->block);
  dataThis = dataThis + offset;
  memcpy(dataThis, buf, size);
  // incrementing the size of the inode by the size that was just written
  inodeThis->size = inodeThis->size + size;

  return size;
}

/**
 * creates a file/directory at the given path
*/
int storage_mknod(const char *path, int mode) {
  // getting the name of the file/dir and inum of the parent dir
  int inumChild = alloc_inode(mode, 0);

  assert(inumChild != 0);

  if (inumChild < 0) {
    printf("result: ENOENT\n");
    return -ENOENT;
  }

  char* nameChild = name_lookup(path);
  int inumParent = parent_lookup(path);

  if(inumParent < 0) {
    printf("result: ENOENT\n");
    return -ENOENT;
  }
  
  // setting up the nodes for the child and the parent
  inode_t* inodeParent = get_inode(inumParent);

  // setting up the child as an entry of the parent dir
  int rv = directory_put(inodeParent, nameChild, inumChild);
  printf("result: %d\n", rv);
  return 0;
}

/**
 * unlinks the given file from its parent to remove from a dir
*/
int storage_unlink(const char *path) {
  // getting root dir, parent and child nodes from the path
  inode_t* inodeRoot = get_inode(0);
  print_directory(inodeRoot);
  int inumChild = tree_lookup(path);
  int inumParent = parent_lookup(path);
  inode_t* inodeParent = get_inode(inumParent);
  char* nameChild = name_lookup(path);
  // checking if the child was found
  // return errno if not
  if (inumChild < 0) {
    return -ENOENT;
  }
  inode_t* inodeChild = get_inode(inumChild);

  // getting the contenct of the child
  void* ptr = blocks_get_block(inodeChild->block);
  inodeChild->size = 0;
  inodeChild->refs = 0;

  // updating the bitmap entries
  void* childBM = get_blocks_bitmap();
  void* ptrBM = get_blocks_bitmap();
  bitmap_put(childBM, inumChild, 0);
  bitmap_put(ptrBM, inodeChild->block, 0);

  // actually deleting the directory
  int deleteStatus = directory_delete(inodeParent, nameChild, inumChild);
  if (deleteStatus < 0) {
    return -ENOENT;
  }

  // freeing the inode
  free_inode(inodeChild);

  return 0;
}

int storage_rename(const char *from, const char *to) {
  // getting the parents of the files/dirs
  int fromInumParent = parent_lookup(from);
  int toInumParent = parent_lookup(to);
  // getting the names from the paths
  char* fromName = name_lookup(from);
  char* toName = name_lookup(to);
  int inumChild = tree_lookup(from);
  // adding the new name to the directory
  inode_t* toParentNode = get_inode(toInumParent);
  directory_put(toParentNode, fromName, inumChild);
  // deleting the old name from the directory
  inode_t* fromParentNode = get_inode(fromInumParent);
  directory_delete(fromParentNode, toName, inumChild);
  return 0;
}