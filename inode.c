#include <assert.h>

#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

/**
 * prints the fields of the node
*/
void print_inode(inode_t *node) {
  printf("node{refs: %d, mode: %d, size: %d, block: %d}\n",
  node->refs, node->mode, node->size, node->block);
}

/**
 * returns the inode of the given inum
*/
inode_t *get_inode(int inum) {
  assert(inum < 64);
  // pointer to the start of the blocks
  void* ptr = blocks_get_block(0) + 64;
  // using pointer arithmetic to get to the node corresponding to the inum
  inode_t* currNode = (inode_t*) (ptr + inum * sizeof(inode_t));
  return currNode;
}

/**
 * returns the inum for a newly allocated node
 * return index of node on success
 * return -1 on failure
*/
int alloc_inode(int m, int s) {
  void* ptr = get_inode_bitmap();
  int nodeCount = 4096/sizeof(inode_t);

  // finds the first free node and sets it to allocated
  for(int ii = 0; ii < nodeCount; ii++) {
    if(!bitmap_get(ptr, ii)) {
      bitmap_put(ptr, ii, 1);
      inode_t* currNode = get_inode(ii);
      int block = alloc_block();
      // setting the fields of then node
      currNode->size = s;
      currNode->mode = m;
      currNode->refs = 1;
      currNode->block = block;
      return ii;
    }
  }
  return -1;
}

/**
 * frees a node
*/
void free_inode(inode_t* inodeChild) {
  free_block(inodeChild->block);
  inodeChild->block = 0;
}