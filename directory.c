#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "directory.h"
#include "inode.h"
#include "slist.h"

#define DIR_COUNT 4096/sizeof(dirent_t)

/**
 * initial setup for the root directory
*/
void directory_init() {
  int inum = alloc_inode(040755, 0);
  inode_t* root = get_inode(inum);
  directory_put(root, ".", inum);
  print_directory(root);
}

/**
 * returns the inum of the given node
 * returns -1 on failure (unable to find the given file)
*/
int directory_lookup(inode_t *dd, const char *name) {
  // getting the entires in the directory
  dirent_t* ents = (dirent_t*) blocks_get_block(dd->block);

  // finds the given directory and returns the inum at that spot
  for(int ii = 0; ii < DIR_COUNT; ii++) {
    if (strcmp(ents[ii].name, name) == 0) {
        print_directory(dd);
        return ents[ii].inum;
    }
  }
  return -1;
}

/**
 * returns the inum of the given path
*/
int parent_lookup(const char* path) {
  int rv = 0;

  // splitting the list using delimiter as '/'
  slist_t* list = s_explode(path, '/');

  // iterate over the list
  while(list->next && list->next->next && rv >= 0) {
    rv = directory_lookup(get_inode(rv), list->next->data);
    list = list->next;
  }

  // return the inum of the parent
  return rv;
}

/**
 * returns the name of the last file/directory in the path
*/
char* name_lookup(const char* path) {
  // splitting the list using delimiter as '/'
  slist_t* list = s_explode(path, '/');

  // iterate over the list
  while(list->next) {
    list = list->next;
  }

  // return the data stored in the element (name)
  return list->data;
}

/**
 * returns the inum of the given file in the tree
 * returns 0 if it's the root directory
 * returns errno if not found
*/
int tree_lookup(const char *path) {
  // checking if the provided isn't just the root directory
  if (strcmp(path, "/") == 0) {
    return 0;
  }

  char *childName = name_lookup(path);
  int parentInum = parent_lookup(path);
  inode_t *parentInode = get_inode(parentInum);
  int inum = directory_lookup(parentInode, childName);

  // could not find the child in the parent directory
  if (inum < 0) {
    return -ENOENT;
  }
  return inum;
}

/**
 * inserts a given node into the given directory
 * return 0 on success
 * return -1 on failure (unable to find space for new entry)
*/
int directory_put(inode_t *dd, const char *name, int inum) {
  // getting the entires in the directory
  dirent_t* ents = (dirent_t*) blocks_get_block(dd->block);

  // finds the first free block to store the entry in 
  for(int ii = 0; ii < DIR_COUNT; ii++) {
    if(ents[ii].inum == 0 && strcmp(ents[ii].name, ".") != 0) {
      ents[ii].inum = inum;
      strcpy(ents[ii].name, name);
      dd->size = dd->size + sizeof(dirent_t);
      return 0;
    }
  }
  return -1;
}

/**
 * delete the given directory
*/
int directory_delete(inode_t *dd, const char *name, int inum) {
  // getting the entries in the node
  void* dataPtr = blocks_get_block(dd->block);
  dirent_t* ents = (dirent_t*) blocks_get_block(dd->block);

  // looping over the entries and resetting the size of the appropriate entry
  for (int ii = 0; ii < DIR_COUNT; ii++) {
    if (ents[ii].inum == inum) {
      memmove(ents + ii, ents + ii + 1, 4096 - (ii * sizeof(dirent_t)));
      dd->size = dd->size - sizeof(dirent_t);
      return 0;
    }
    dataPtr = dataPtr + sizeof(dirent_t);
  }
  return -1;
}

/**
 * returns the contents of the given directory
 * return NULL if the directory is empty
*/
slist_t *directory_list(const char *path) {
  // get the inum of the path
  int inum = tree_lookup(path);

  if (inum < 0) {
    return 0;
  }

  // get the node of the corresponding inum
  inode_t* dd = get_inode(inum);
  // get the entries in the directory
  dirent_t* ents = (dirent_t*) blocks_get_block(dd->block);
  // adding the first entry to a list
  slist_t* contents = 0;

  // adding the entry to the list if it exists
  for(int ii = 0; ii < dd->size / sizeof(dirent_t); ii++) {
    contents = s_cons(ents[ii].name, contents);
  }
  return contents;
}

/**
 * prints the contenct of the directory
*/
void print_directory(inode_t *dd) {
  // getting the entries in the directory
  dirent_t* ents = (dirent_t*) blocks_get_block(dd->block);

  printf("Directory Contents:\n");
  // printing each entry
  for (int ii = 0; ii < DIR_COUNT; ii++) {
    if (ii < 5) {
      printf("name: %s, inum: %d\n", ents[ii].name, ents[ii].inum);
    }
  }
  printf("End of Content\n");
}