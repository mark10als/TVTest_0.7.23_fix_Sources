## 1. download sources

- tvest https://github.com/project-pp/TVTest_0.7.23_Sources
- baseclasses https://github.com/project-pp/baseclasses

## 2. extract zip and place files

- baseclasses-master\
    - \*.\*

- TVTest_0.7.23_Sources-master\
    - TVTest_0.7.23_Src\
        - baseclasses\
            - _copy all files from baseclasses-master\\_

## 3. build

1. open _TVTest_0.7.23_Src\baseclasses\baseclasses.sln_ then build _BaseClasses_ project
   as `Debug Win32`, `Debug x64`, `Release Win32`, and `Release x64`.
2. open _TVTest_0.7.23_Src\faad2-2.7\libfaad\libfaad.sln_ then build _libfaad_ project
   as `Release Win32` and `Release x64`.
3. open _TVTest_0.7.23_Src\TVTest_Image\TVTest_Image_VS2010.sln_ then build _TVTest_Image_ project
   as `Release Win32` and `Release x64`.
4. open _TVTest_0.7.23_Src\TVTest.sln_ then build _TVTest_ project
   as `Release Win32` and `Release x64`.
5. Win32 binaries = _TVTest_0.7.23_Src\Win32\Release\TVTest.exe_ and _TVTest_0.7.23_Src\Win32\Release\TVTest_Image.dll_  
   x64 binaries = _TVTest_0.7.23_Src\x64\Release\TVTest.exe_ and _TVTest_0.7.23_Src\x64\Release\TVTest_Image.dll_

if you want to use sample plugins:

1. open _TVTest_SDK_110123\Samples\Samples_VS2010.sln_ then build solution
    as `Release_static Win32` and `Release_static x64`.
2. copy _TVTest_SDK_110123\Samples\Release_static\\\*.(tvtp|dll)_ to _TVTest_0.7.23_Src\Win32\Release\Plugins\\_,  
   and copy _TVTest_SDK_110123\Samples\x64\Release_static\\\*.(tvtp|dll)_ to _TVTest_0.7.23_Src\x64\Release\Plugins\\_.

## license

- TVTest v0.7.23: GPLv2
- zlib v1.2.8: [zlib License](http://zlib.net/zlib_license.html); GPL compatible
- libpng v1.6.21: [libpng License](http://www.libpng.org/pub/png/src/libpng-LICENSE.txt); GPL compatible
- libjpeg v9b: [libjpeg License](https://github.com/project-pp/TVTest_0.7.23_Sources/blob/main/TVTest_0.7.23_Src/TVTest_Image/libjpeg/libjpeg.txt)
- libfaad v2.7: GPLv2
