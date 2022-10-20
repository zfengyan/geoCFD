# geoCFD

<img width="558" alt="building_set_1_angle1" src="https://user-images.githubusercontent.com/72781910/194170661-8729cccf-e41a-4802-ab51-f71cba5e6d75.PNG">

Process geometry for CFD simulation - remove internal faces between adjacent buildings.

It's a cross-platform project (currently tested on `x64-windows10` platform, see [geocfd-Ubuntu](https://github.com/SEUZFY/geocfd-Ubuntu) for the basic setting up on 
`wsl-ubuntu` platform). 

- support for all `LoD` levels in `cityjson`(lod 1.2, lod 1.3, lod 2.2).

- support `multithreading` process.

- support exporting as `.json` file or `.off` file.

- visualisation:

	[ninja](https://ninja.cityjson.org/)

- validate the result file via: 

	- `val3dity`  - [validate](http://geovalidation.bk.tudelft.nl/val3dity/) the geometry
  
  	- `validator` - [validate](https://validator.cityjson.org/) the `cityjson` file

## Usage

Compile and build it, enter into the `out\build\x64-Release` folder (on windows for example) then open the console (e.g. Windows PowerShell)

```console
.\geoCFD [path of the adjacency file (.txt)] [multi_thread_tag]
```
example:
```console
PS D:\SP\geoCFD\out\build\x64-Release> .\geoCFD D:\SP\geoCFD\data\dataset_5\adjacency5.txt t
this is: D:\SP\geoCFD\out\build\x64-Release\geoCFD.exe
current lod level: 2.2
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
build nef polyhedron
there are 21 nef polyhedra in total
performing minkowski sum ...
multi threading is enabled
done
building big nef ...
done
extracting nef geometries ...
done
processing shells for cityjson ...
done
writing the result to cityjson file...
file saved at: D:\SP\geoCFD\data\interior_multi_m=0.1.json
Time: 5.61774s
```
### Note

* if the program does not exit, you may need to re-open your console again and re-run it. (for example, dataset_2).

    This may be due to the complex geometry of the buildings in `dataset_2`, one of the buildings contain holes.

* the `minkowski param` is set to `0.1` by default.

	The param can be altered, but proceed with caution, too small minkowski param may not fill the holes of the building in `dataset_2`.

## Prerequisite

[CGAL](https://www.cgal.org/) - The version should be above `5.0` since we can use the `header-only`, which means we don't have to manually compile `CGAL`.

install `CGAL` via [vcpkg](https://vcpkg.io/en/index.html):

check this -> 

[install vcpkg](https://www.youtube.com/watch?v=b7SdgK7Y510)

[Download CGAL for Windows](https://www.cgal.org/download/windows.html)

## How to use?

This project is built and executed on a `windows10(64 bit)` platform.

Clone this project and open it in [Visual Studio](https://visualstudio.microsoft.com/) **2019** or newer version, you should be able to directly build and run.

## Compile info

C++ Standard: `C++ 11`

Compiler: `MSVC`

Generator: `Ninja`

Commands:
```console
"..\MICROSOFTVISUALSTUDIO\2019\COMMUNITY\COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe"  
-G "Ninja"  
-DCMAKE_BUILD_TYPE:STRING="RelWithDebInfo" 
-DCMAKE_INSTALL_PREFIX:PATH="..\geoCFD\out\install\x64-Release" 
-DCMAKE_C_COMPILER:FILEPATH="../MicrosoftVisualStudio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe" 
-DCMAKE_CXX_COMPILER:FILEPATH="../MicrosoftVisualStudio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe"  
-DCMAKE_MAKE_PROGRAM="..\MICROSOFTVISUALSTUDIO\2019\COMMUNITY\COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe" 
-DCMAKE_TOOLCHAIN_FILE="../dev/vcpkg/scripts/buildsystems/vcpkg.cmake" 
```
The commands are the information of compiler and generator(for example, the file path of `cl.exe` and `ninja.exe`), which should
be generated by your IDE automatically.

## Attention
1. **vertices repeatness**

	Using `CGAL::Polyhedron_3` can be tricky, since `Polyhedron_builder` doesn't like repeated vertices, and even if it workswith repeatness, the created `Polyhedron_3` is NOT closed(and thus can not be converted to `Nef_polyhedron`).Thus extra care needs to be taken when creating `Polyhedron_3`.

2. **geometry error**

	In `lod 2.2` geometries of buildings can get complicated, in our test there can be some geometry errors in the original dataset, for example:
	```console
	build nef for building: NL.IMBAG.Pand.0503100000018412-0

	CGAL::Polyhedron_incremental_builder_3<HDS>::
	lookup_halfedge(): input error: facet 29 has a self intersection at vertex 79.
	polyhedron closed? 0
	```
	A manual fix can be possible towards certain building set but not general, if we take the **universality** and **automation** into consideration, using `convex hull` to replace the corresponding building seems to be a good choice, yet this approach may lead to the compromisation of the original building shapes.
	
	There are also other issues in `lod 2.2`, see [robust](https://github.com/SEUZFY/geoCFD/tree/master#robust) section for details.
	
## Robust

One important aspect is that `CGAL::Nef_polyhedron_3` will complain when the points of surfaces are not planar, which will lead to a `invalid` `Nef_polyhedron`.

A possible solution is demonstrated as below (it has been tested on building set 1):
```cpp
typedef CGAL::Simple_cartesian<float> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron_3;

Polyhedron_3 polyhedron = load_my_polyhedron();
CGAL::Polygon_mesh_processing::triangulate_faces(polyhedron);
```
A triangulation process of the surfaces of a polyhedron can make points planar since there's always a plane that passes between any three points, no matter their locations but this is not guaranteed when having four or more points, which is pretty common in `lod 2.2`, among 23 `Nef polyhedra` from 23 buildings, 22 of them are
`invalid` and only 1 is `valid`.

It should however be noted that, triangulating process will modify the original geometry a bit (tiny change but for now not sure how to visualise / quantify the possible influence) and also have time cost.

Also we need to take the `geometry validity` of `CGAL::Polyhedron_3` into consideration. The `is_valid()` function only checks for `combinatorial validity` (for example in every half-edge should have an opposite oriented twin) but does not check the geometry correctness. In order to check it we can use `CGAL::Polygon_mesh_processing::do_intersect` to check for example if there are any intersections (need to verify and test).

Why do we need to check `geometry validity`? Because this could break the corresponding Nef polyhedron, for example, the corresponding Nef polyhedron is not valid.

possible solutions: allow users to switch on/off different robust check functions by defining different macros, for example:
```cpp
#define _POLYHEDRON_3_GEOMETRY_CHECK_
#define _POLYHEDRON_3_COMBINATORIAL_CHECK_
...
```
Also, since the triangulation process may have some cons, we can apply it on `lod 2.2` but not on `lod 1.2` and `lod 1.3`:
```cpp
if lod == 2.2
	triangulate the surfaces of polyhedron
	feed Nef with the triangulated polyhedron
...
```

issues related minkowski sum and irregular building:

[CGAL Minkowski sum assertion when performing union operation of Nef_polyhedron_3](https://github.com/CGAL/cgal/issues/6973)

## Benchmark

## Other platforms

If you use other platforms (such as `Linux` or `MacOS`), you can refer to `CMakeLists.txt` file and use it to build a `CMake` project using `src`, `include` and `data` folder.
