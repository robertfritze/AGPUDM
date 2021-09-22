# AGPUDM

## Introduction

This project allows to address the GPU on Android devices using the OpenCL framework. 
In contrast to other projects the OpenCL library of the device is loaded only at runtime. There is no need to 
include it during the build process.

Two data mining algorithms 
(DBSCAN and Kmeans) have been implemented with two different programming languages and different programming
paradigms (single-threaded, multi-threaded, task/data parallelism (GPU)).

## Docs

There is a [html](app/doc/html/index.html) and [pdf](app/doc/latex/refman.pdf) doxygen documentation available for this project. 
Clone the repository to see the HTML documentation.
As of 9/22/2021 the 
C source/header files and a part of the Java classes have been included.  
Additional documentation will be available soon.


## Installation

Clone this repository (either with "git clone" or downloading and extracting the zip-file). Import the project
to AndroidStudio. Attach your Android device and enable developper options (see manual of your device). Build and
run this project on your device. It should run out of the box. 

## Setup

This app has only one activity. The data mining jobs are executed in a deferred manner in the background.
If the devices is rebooted, the calculations will resume automatically. When the user launches the app,
the main activity tries to connect to the background jobs and tries to read out the status information.
If there are no background jobs, the user can submit a new background job. If there are already 
background jobs, the status is displayed and the user can cancel the jobs. 

The app tries to find the OpenCL library on the device automatically. If an OpenCL library is found and
can be loaded, some information is displayed. If not, the user can enter the path to the OpenCL library
on the device manually and try to load it manually. 

## How to set parameters

The parameters for the data mining jobs have to be set in the file app/src/main/res/values/values.xml. 
The following attributes can be set:

* **mode** (string): 
  * "dynamic" multiple test are made with built in values. The attributes 'clusterno', 'clustersize'
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

## Results

The results are stored in csv-format on the device. The path of the app is used 
(should be something like sdcard/Android/data/com.example.dmocl/files).
Log information is also stored in this path. This path is set automatically - do not prepend the path
in the values.xml file.

The result file has 21 columns separated by ';'. The first four contain the
parameters of the test (cores;size;cluster;features). The 
next five show the wall clock time for different 
implementations of the kmeans algorithm
(Java, C, C+GPU, multithreaded Java, multithreaded C). The
following five columns hold the wall clock times for the 
DBSCAN algorithm (again Java, C, C+GPU, multithreaded Java,
multithreaded C). The next columns contain the 
exclusive time for the GPU and the multithreaded implementations.
'Exclusive time' means the time needed for the execution of the
entire algorithm minus the time needed for the setup of the GPU or 
the threads. Three values are collected by Kmeans and another
three by DBSCAN. The last column is zero if the output of all
the implementations of the DBSCAN algorithm was EXACTLY equal. 
For Kmeans a comparison is not possible because each implementation
selects the cluster centers randomly at the beginning.


