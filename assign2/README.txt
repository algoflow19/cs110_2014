Saravanan Dhakshinamurthy, SUNETid : saravan2

Optimization catalog :

Most beneficial Optimizations :
1. Pathstore Checksum optimization :
Storing the checksum of already know files in store element structure for easy checksum comparison with incoming file.

 Analysis :
The "IsSameFile" module of pathstore was highly inefficient in comparing the checksums of contents in the incoming file with already known files. The checksum calculation took up a ton of CPU and I/O cyles as the stored files kept on increasing. The incoming file's checksum was freshly calculated with 

 Prediction :
Significant reduction in large.img which has very large number of files, there wont be any significant drop in simple.img which has smaller footprint.

Measurements :
Before :
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/simple.img
Starting index build ....
Index disk ../../assign2/simple.img (latency 0) completed in 0.000236 seconds
************ Stats ***************
Disksim: 403 reads, 0 writes
Diskimg: 403 reads, 0 writes
Scan: 3 files, 3 words, 15 characters, 1 directories, 5 dirents, 0 duplicates
Index: 3 stores, 3 allocates, 0 lookups
Pathstore:  3 stores, 0 duplicates
Pathstore2: 3 compares, 3 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 4 opens, 6 reads, 99 getchars, 4 isfiles
Usage: 0.000000 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 2 involuntary ctxt switches

After:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/simple.img
Starting index build ....
Index disk ../../assign2/simple.img (latency 0) completed in 0.000228 seconds
************ Stats ***************
Disksim: 385 reads, 0 writes
Diskimg: 385 reads, 0 writes
Scan: 3 files, 3 words, 15 characters, 1 directories, 5 dirents, 0 duplicates
Index: 3 stores, 3 allocates, 0 lookups
Pathstore:  3 stores, 0 duplicates
Pathstore2: 3 compares, 3 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 4 opens, 6 reads, 99 getchars, 4 isfiles
Usage: 0.000000 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 21 involuntary ctxt switches


2. Fileops_getChar optimization :
Capturing the file block content associated with fd after the first call to file_getblock. The captured file content can be served for subsequent get next char requests.

Analysis:
fileops_Getchar was performing a costly disk I/O operation that returned about 512 bytes of disk content and utilized just 1 byte of it. Storing the disk block contents in open file table structure would save atleast 512 I/Os for ever file and directory found in the disk system by scan tree and index recursive function.

Prediction :
Significant reduction in I/O across all disk images. Serious throttleneck removed.

Measurements :
Before:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/simple.img
Starting index build ....
Index disk ../../assign2/simple.img (latency 0) completed in 0.000228 seconds
************ Stats ***************
Disksim: 385 reads, 0 writes
Diskimg: 385 reads, 0 writes
Scan: 3 files, 3 words, 15 characters, 1 directories, 5 dirents, 0 duplicates
Index: 3 stores, 3 allocates, 0 lookups
Pathstore:  3 stores, 0 duplicates
Pathstore2: 3 compares, 3 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 4 opens, 6 reads, 99 getchars, 4 isfiles
Usage: 0.000000 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 21 involuntary ctxt switches


After:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/simple.img
Starting index build ....
Index disk ../../assign2/simple.img (latency 0) completed in 0.000164 seconds
************ Stats ***************
Disksim: 221 reads, 0 writes
Diskimg: 221 reads, 0 writes
Scan: 3 files, 3 words, 15 characters, 1 directories, 5 dirents, 0 duplicates
Index: 3 stores, 3 allocates, 0 lookups
Pathstore:  3 stores, 0 duplicates
Pathstore2: 3 compares, 3 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 4 opens, 6 reads, 99 getchars, 4 isfiles
Usage: 0.000000 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 21 involuntary ctxt switches


3. Fileops inode fetch optimization :
Fileops was doing a costly pathname_lookup to find the inode and inumber of the open fd to fetch disk data. Storing the inode and inumber associated with the file during fileops_open would lead us to avoid ever more expensive pathname_lookups.

Analysis:
Pathname_lookup is vey costly because it recursively searches from the root directory to find the inumber of the last subpath. For a long pathname, it would lead to significant overload if done in file_getChar. The inode_iget operation performed to retrieve the inode to get size of the file again contributes to significant repetitive disk I/O.

Prediction :
Storing the inode and inumber associated with the fd during fileops_open would lead to significant I/O decrease in all disks, particularly large and vlarge image disks.

Measurements :
Before:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/large.img
Starting index build ....
Index disk ../../assign2/large.img (latency 0) completed in 2.490733 seconds
************ Stats ***************
Disksim: 4566635 reads, 0 writes
Diskimg: 4566635 reads, 0 writes
Scan: 1 files, 213417 words, 1139863 characters, 1 directories, 3 dirents, 0 duplicates
Index: 213417 stores, 11199 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 2 opens, 4 reads, 1139913 getchars, 2 isfiles
Usage: 0.664041 usertime, 1.824114 systemtime, 0 voluntary ctxt switches, 38 involuntary ctxt switches

After:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/large.img
Starting index build ....
Index disk ../../assign2/large.img (latency 0) completed in 0.115377 seconds
************ Stats ***************
Disksim: 7130 reads, 0 writes
Diskimg: 7130 reads, 0 writes
Scan: 1 files, 213417 words, 1139863 characters, 1 directories, 3 dirents, 0 duplicates
Index: 213417 stores, 11199 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 2 opens, 4 reads, 1139913 getchars, 2 isfiles
Usage: 0.112007 usertime, 0.004000 systemtime, 3 voluntary ctxt switches, 21 involuntary ctxt switches

4. Cache :
Adding a cache layer sandwiched between diskimg and disksim. 

Analysis :
Every file present in the disk mostly gets read to calculate checksum and then subsequently read for indexing. Caching the inode and disk blocks that were retreived during checksum calculation would significantly reduce the I/O footprint of indexing, as most if not all blocks required for indexing could have been preserved on the cache. A fast direct mapped cache would be ideal for such a requirement, with slight modifications in diskimg layer to utilize the cache fetch and store apis.

Prediction:
Most of the disk I/O so far had been reading already once accessed ( and now cannot be used ) inode or data block. Adding a cache layer would bring down the effective I/O further by a factor of 2 or more.

Measurements:
Before:
myth3:~/cs110/assignment-original-srcfiles/assign2> ./disksearch -l 0 -c 1024 ../../assign2/medium.img
Starting index build ....
Index disk ../../assign2/medium.img (latency 0) completed in 0.085918 seconds
************ Stats ***************
Disksim: 2156 reads, 0 writes
Diskimg: 2156 reads, 0 writes
Scan: 1 files, 63971 words, 365444 characters, 1 directories, 3 dirents, 0 duplicates
Index: 63971 stores, 7650 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 2 opens, 4 reads, 365494 getchars, 2 isfiles
Usage: 0.040002 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 32 involuntary ctxt switches

After:
myth20:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./large.img
Starting index build ....
Index disk ./large.img (latency 0) completed in 0.393731 seconds
************ Stats ***************
Disksim: 2655 reads, 0 writes
Diskimg: 14242 reads, 0 writes
Scan: 1 files, 213417 words, 1139863 characters, 1 directories, 3 dirents, 0 duplicates
Index: 213417 stores, 11199 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 0 opens, 4 reads, 1139913 getchars, 2 isfiles
Usage: 0.120007 usertime, 0.012000 systemtime, 0 voluntary ctxt switches, 57 involuntary ctxt switches
myth20:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./vlarge.img
Starting index build ....
Index disk ./vlarge.img (latency 0) completed in 1.790019 seconds
************ Stats ***************
Disksim: 27801 reads, 0 writes
Diskimg: 196468 reads, 0 writes
Scan: 856 files, 1542882 words, 13482054 characters, 1 directories, 858 dirents, 0 duplicates
Index: 1542882 stores, 28042 allocates, 0 lookups
Pathstore:  856 stores, 0 duplicates
Pathstore2: 365940 compares, 365940 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 0 opens, 859 reads, 13496635 getchars, 857 isfiles
Usage: 1.380086 usertime, 0.040002 systemtime, 0 voluntary ctxt switches, 319 involuntary ctxt switches
myth20:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./medium.img
Starting index build ....
Index disk ./medium.img (latency 0) completed in 0.075979 seconds
************ Stats ***************
Disksim: 721 reads, 0 writes
Diskimg: 4294 reads, 0 writes
Scan: 1 files, 63971 words, 365444 characters, 1 directories, 3 dirents, 0 duplicates
Index: 63971 stores, 7650 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 0 opens, 4 reads, 365494 getchars, 2 isfiles
Usage: 0.040002 usertime, 0.000000 systemtime, 0 voluntary ctxt switches, 27 involuntary ctxt switches

Miscellaneous Optimizations :
1. Optimizing Pathname_lookup
Creating pathname_lookup_optimized function to search of inumber of the subpath from the parent directory passed as argument.

 Analysis :
Running disksearch on manyfiles.img produced showed a highly undesirable nature of pathname_lookup. 
It gets prohibitively expensive as the pathnames get longer and longer. We could quicken the inumber search if we could pass the name of the parent directory retrieved from scanTreeAndIndex. Morever, these Pathname_lookups could end up overwriting critical blocks in the direct mapped cache thereby cascading cache misses. The scanTreeAndIndex recursive function was slightly modified to get parentDir name as argument in every level of the recursive call.

Prediction :
Would lead to slight improvement in manyfiles.img

Measurements:
Before: Disksim was around ~9900
After:
myth6:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./manyfiles.img
Starting index build ....
Index disk ./manyfiles.img (latency 0) completed in 0.920923 seconds
************ Stats ***************
Disksim: 9815 reads, 0 writes
Diskimg: 29842 reads, 0 writes
Scan: 7497 files, 186551 words, 1215277 characters, 15 directories, 7549 dirents, 8 duplicates
Index: 186551 stores, 4310 allocates, 0 lookups
Pathstore:  7505 stores, 8 duplicates
Pathstore2: 28123731 compares, 28123723 checksumdiff, 8 comparesuccess, 0 comparefail
Fileops: 7528 opens, 7564 reads, 1345373 getchars, 7520 isfiles
Usage: 0.900056 usertime, 0.016001 systemtime, 0 voluntary ctxt switches, 41 involuntary ctxt switches


2. Removing pathname_lookup calls and minimizing inode_iget
The costly pathname_lookup call was completely avoided in scanTreeandIndex recursive function. In addition the lower pathname, directory, file and inode layers were rewritten to avoid any inode_iget or search for inumbers.

Analysis:
The direct mapped cache without any protective regions was in danger of losing critical blocks in cache if lower level apis using pathname lookup or fetch inodes from inode table. Avoiding any unnecessary disk I/O for information that has already been fetched and processed in upper layers would improve disk I/O footprint in large and complex file systems. This was a significant rework involving scan fileops, file, pathname and directory layers.

Prediction:
Atleast a few hundred Disk I/O can be saved if the lower file system layers knew about the inumber and inode structure.

Measurement:
Before:
myth20:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./vlarge.img
Starting index build ....
Index disk ./vlarge.img (latency 0) completed in 1.790019 seconds
************ Stats ***************
Disksim: 27801 reads, 0 writes
Diskimg: 196468 reads, 0 writes
Scan: 856 files, 1542882 words, 13482054 characters, 1 directories, 858 dirents, 0 duplicates
Index: 1542882 stores, 28042 allocates, 0 lookups
Pathstore:  856 stores, 0 duplicates
Pathstore2: 365940 compares, 365940 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 0 opens, 859 reads, 13496635 getchars, 857 isfiles
Usage: 1.380086 usertime, 0.040002 systemtime, 0 voluntary ctxt switches, 319 involuntary ctxt switches
myth20:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./medium.img
Starting index build ....
Index disk ./medium.img (latency 0) completed in 0.075979 seconds

After:
myth28:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./vlarge.img
Starting index build ....
Index disk ./vlarge.img (latency 0) completed in 1.305234 seconds
************ Stats ***************
Disksim: 27436 reads, 0 writes
Diskimg: 105341 reads, 0 writes
Scan: 856 files, 1542882 words, 13482054 characters, 1 directories, 858 dirents, 0 duplicates
Index: 1542882 stores, 28042 allocates, 0 lookups
Pathstore:  856 stores, 0 duplicates
Pathstore2: 365940 compares, 365940 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 857 opens, 859 reads, 13496635 getchars, 857 isfiles
Usage: 1.260078 usertime, 0.044002 systemtime, 0 voluntary ctxt switches, 30 involuntary ctxt switches

3. Adding a protected inode region in cache
A protected cache region was added to house single and double indirection inode refered blocks.

 Analysis :
Saving double and single indirection blocks that would be referred by upper layers in a region that wont be rewritten as hastily as unprotected region could save some I/O.

Prediction:
Disk I/O improvements in manyfiles.img but not necessarily for simple, medium and large file disks.

Measurements:

Index disk ./large.img (latency 0) completed in 90.724181 seconds
************ Stats ***************
Disksim: 2699 reads, 0 writes
Diskimg: 9783 reads, 0 writes
Scan: 1 files, 213417 words, 1139863 characters, 1 directories, 3 dirents, 0 duplicates
Index: 213417 stores, 11199 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 2 opens, 4 reads, 1139913 getchars, 2 isfiles
Usage: 0.180011 usertime, 0.024001 systemtime, 204 voluntary ctxt switches, 8 involuntary ctxt switches
[Inferior 1 (process 12239) exited normally]
(gdb) quit

myth6:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./vlarge.img
Starting index build ....
Index disk ./vlarge.img (latency 0) completed in 1.885948 seconds
************ Stats ***************
Disksim: 27430 reads, 0 writes
Diskimg: 105341 reads, 0 writes
Scan: 856 files, 1542882 words, 13482054 characters, 1 directories, 858 dirents, 0 duplicates
Index: 1542882 stores, 28042 allocates, 0 lookups
Pathstore:  856 stores, 0 duplicates
Pathstore2: 365940 compares, 365940 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 857 opens, 859 reads, 13496635 getchars, 857 isfiles
Usage: 1.852115 usertime, 0.032002 systemtime, 0 voluntary ctxt switches, 41 involuntary ctxt switches


myth6:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./manyfiles.img
Starting index build ....
Index disk ./manyfiles.img (latency 0) completed in 0.920923 seconds
************ Stats ***************
Disksim: 9415 reads, 0 writes
Diskimg: 28842 reads, 0 writes
Scan: 7497 files, 186551 words, 1215277 characters, 15 directories, 7549 dirents, 8 duplicates
Index: 186551 stores, 4310 allocates, 0 lookups
Pathstore:  7505 stores, 8 duplicates
Pathstore2: 28123731 compares, 28123723 checksumdiff, 8 comparesuccess, 0 comparefail
Fileops: 7528 opens, 7564 reads, 1345373 getchars, 7520 isfiles
Usage: 0.900056 usertime, 0.016001 systemtime, 0 voluntary ctxt switches, 41 involuntary ctxt switches


 4. Adding improvements to inode cache layer :
Directories can now save upto 2 and files upto 8 indirectly referenced inode disk blocks (mentioned in inode_structure) in a secure region of upto 36 cache blocks. These special cache blocks have two new fields typeandindirection, inp (the inode pointer associated with the block). As the software had been already rewritten to just fetch inode_iget once in ScanTree, the inode pointer is uniform and passed as argument down to the lowest inode layer. 
Special cache apis are used in inode layer to save single and double indirection blocks in a region that wont be as easily rewritten as the unprotected cache region.
This "inp" (inode pointer) would later act as the key for flushing specific blocks belonging to an inode/open file descriptor to free up cache lines for directories present in upper regions of pathname or for newer leaf files that are yet to be found. 

 Analysis :
Missing a crucial singly or doubly referenced cache block by directories in upper levels of path name or by a file during indexing can lead to unnecessary disk I/O for information that had already been fetched. Large directories in upper layers of pathname already have disk content of 512 bytes in fileops layer, the only other block they might need is either a single or double indirection inode block in cache when the recursive function reaches back to their layers. The maximum directory
depth in manyfiles.img is 14, therefore we would need 2 * 14 blocks for directories and 8 blocks for the leaf file.

Replacement policy :
Incoming cache request get preference to store all 8 blocks if necessary(incase of a file), directories can store upto 2. If a directory needs to store a third block, the single indirection block will be replaced, the double indirection block will not be touched. If all cache lines are taken, protected cache content will be moved to unprotected region and 

Measurements:
Index disk ./manyfiles.img (latency 0) completed in 0.875452 seconds
************ Stats ***************
Disksim: 9358 reads, 0 writes
Diskimg: 28842 reads, 0 writes
Scan: 7497 files, 186551 words, 1215277 characters, 15 directories, 7549 dirents, 8 duplicates
Index: 186551 stores, 4310 allocates, 0 lookups
Pathstore:  7505 stores, 8 duplicates
Pathstore2: 28123731 compares, 28123723 checksumdiff, 8 comparesuccess, 0 comparefail
Fileops: 7528 opens, 7564 reads, 1345373 getchars, 7520 isfiles
Usage: 0.852053 usertime, 0.020001 systemtime, 0 voluntary ctxt switches, 49 involuntary ctxt switches

myth16:~/cs110/assign2> ./disksearch -l 0 -c 1024 ./large.img
Starting index build ....
Index disk ./large.img (latency 0) completed in 0.410192 seconds
************ Stats ***************
Disksim: 2707 reads, 0 writes
Diskimg: 9783 reads, 0 writes
Scan: 1 files, 213417 words, 1139863 characters, 1 directories, 3 dirents, 0 duplicates
Index: 213417 stores, 11199 allocates, 0 lookups
Pathstore:  1 stores, 0 duplicates
Pathstore2: 0 compares, 0 checksumdiff, 0 comparesuccess, 0 comparefail
Fileops: 2 opens, 4 reads, 1139913 getchars, 2 isfiles
Usage: 0.136008 usertime, 0.012000 systemtime, 0 voluntary ctxt switches, 74 involuntary ctxt switches

Finally, trivial enhancement in pathstore to defer checksum calculation until strcmp of pathnames with already existing pathnames in store returns false.
