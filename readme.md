# FileSystem (FS-SIM)

**Name: Liyao Jiang**

**ID: 1512446**

## System Calls Used

- - -

- open: used in fs_mount, fs_create, fs_delete, fs_read, fs_write, fs_resize, fs_defrag

- close: used in fs_mount, fs_create, fs_delete, fs_read, fs_write, fs_resize, fs_defrag

- read: used in fs_mount, fs_create, fs_delete, fs_read, fs_write, fs_resize, fs_defrag

- write: used in fs_mount, fs_create, fs_delete, fs_read, fs_write, fs_resize, fs_defrag

- lseek: used in fs_mount, fs_create, fs_delete, fs_read, fs_write, fs_resize, fs_defrag

## Testing

- - -

- I tested the code by running my program against the given 4 sample tests, and the consistency checks. I write a bash script to automate the testing, pipe the stdout and stderr to files, and use cmp to compare with the given expected results.

- All tests are passed, except the sample test case 4

- for test case 4, after running my program, my clean_disk result was has one byte different from the given expected clean_disk_result, the 37 byte (in the free_block_list of superblock) was different. 

- It took me a lot of time to debug, but unfortunately I couldn't locate the issue and solve the hidden bug. So, this test case remains failed with a slightly different disk file, but the stdout and stderr for this test case passed.

## References

- - -

* string tokenize function reused from CMPUT 397 assignment1 

* function `bool areEqual(uint8_t arr1[], uint8_t arr2[], int n, int m)` is adopted from <https://www.geeksforgeeks.org/check-if-two-arrays-are-equal-or-not/>
