# AGPUDM
This project allows to address the GPU on Android devices using the OpenCL framework. 
In contrast to other projects the OpenCL library of the device is loaded only at runtime. There is no need to 
include it during the build process.

Two data mining algorithms 
(DBSCAN and Kmeans) have been implemented with two different programming languages and different programming
paradigms (single-threaded, multi-threaded, task/data parallelism (GPU)).

There is a doxygen documentation available for this project (see app/doc/html/index.html). As of 9/12/2021 only the 
C part (except dbscan_c.c and kmeans_c.c) has been included in this documentation. 
Additional documentation will be available soon.
