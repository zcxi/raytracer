# raytracer

Graphical renderer that uses vector math to simulate light ray projections.


### Programming Language
This project is written in C++11


### Directory Structure

src/Math/ -- This folder contains vector math helpers including the implementations of a quarternion class and a 3d vector class

src/Scene/ -- Contains implementation for recreating a scene including light sources and camera

src/Shapes/ -- Contains the implementation for various shapes in 3d space and helpers for calculating light ray intercepts

src/Writer/ -- Contains helpers for file IO operations


### Project status
Project is feature complete and supports multithreaded rendering                                                                                                                      
### Future Changes
Post processing 
- anti aliasing
    - [wu's Algorithm](https://www.codeproject.com/Articles/13360/Antialiasing-Wu-Algorithm)
- soft shadows
    - [percentage-closer filtering](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-17-efficient-soft-edged-shadows-using)


### Sample Trace

![header image](/raytracer/output.png)


### Building and Running
To build with GCC or G++

% g++ *.cpp -o Output