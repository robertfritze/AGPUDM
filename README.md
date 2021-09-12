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

The parameters for the data mining jobs have to be set in the file app/src/main/res/values/values.xml. 
The following attributes can be set:

* **mode** (string): 
  * "dynamic" multiple test are made with build in values. The attributes 'clusterno', 'clustersize'
     and 'features' must be set correctly but will not be used.
  * "fixed" tests are made with the attributes specified in this file
  
* **passes** (integer) Number of passes that should be made
* **threads** (integer) Number of threads that should be used for multithreaded implementations. Zero means
  the maximum number of available cores.
* **export** (boolean) "true" export results, "false" do not export results
* **append** (boolean) "true" append to old results if available, "false" delete old results
* **resultfilename** (string) Name of the csv-file in which to store the results. **DO NOT PREPEND A PATH!** The 
  correct path will be prepended automatically. In the version for larger screen sizes the full path and name
  are shown in the info box.
* **log** (boolean) "true" log information, "false" do not log information (there is just one log-level)
* **logfilename** (string) Name of the text-file in which to store the results. **DO NOT PREPEND A PATH!** The 
  correct path will be prepended automatically. In the version for larger screen sizes the full path and name
  are shown in the info box.
* **kmeanseps** Maximum cluster center displacement. If the sum of the absolute values of the cluster
  displacements drops below this threshold, the algorithm terminates. 
* **dbscaneps** Search radius for DBSCAN (0=sqrt(features))
* **dbscanneigh** Minimum number of neighbours within the serach radius (0=10*features)
* **clusterno** Number of clusters to generate randomly. For Kmeans also the number of clusters to 
  search for.
* **clustersize** Size of the clusters (equal size for all clusters). 
* **features** Number of features for each data item.

In a future version an additional activity, that will allow to set the attributes on the device during runtime, 
will be added to this project.    
