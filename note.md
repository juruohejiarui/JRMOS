# PS

## Memory Management
- for user space memory mapping, default block size is 4 KiB for tiny usage situation (< 1 GiB) and 2M for large usage situation (> 1GiB), kernel space use 2M
- for dmas mapping, we use the largest mapping block for 