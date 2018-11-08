# SD CARD + FAT32 (Pi1, 2, 3 - AARCH32, Pi3 AARCH64)
>
In trying to take accelerated graphics further I find I am in need of being able to read files for both 3D mesh models and shaders.

I had done SD Card and FAT32 a while ago for for a CodeProject article but it was very rough and did not do simple things like deal with sub directories. I use commercial libraries for most of this stuff for work code but wanting to release public domain samples I am forced to having to write functions.

Now at this stage I have completed only the search and read file functionality. I will do the writefile function as an excercise sometime in near future.

The bitmap display is very rough it is there just for me to check the read operations. I am still trying to work out a robust and flexible interface for bitmaps on the smartstart interface. The big issue is the bitmaps are in XYZ colour depth and your screen can be in ZYX colour depth and you need to be able to quickly organize exchange between the two colour formats. I will probably do it the same way as Windows but I have so much on at the moment it may be a few weeks before I get to it.

Note the displayed bitmap file is in a subdirectory "bitmaps" which is in the DiskImg directory. So if you want to just test the sample you need to make sure you copy all the contents of the DiskImg directory onto the SD Card.

Please be aware although I freely release the code, Microsoft has patents on the FAT32 format and use of them commercially requires a license from Microsoft even on embedded systems.
I refer you to 
>
https://en.wikipedia.org/wiki/Microsoft_Corp._v._TomTom_Inc.
>
>Note:   I fell into a problem that I forgot to mount the SDCard via the sdInitCard and quikly found out the sdCreateFile and sdFindFirst behave rather strangely rather than immeditaley failing. It's a bug which I will fix tonight :-)
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/SD_FAT32.jpg?raw=true)
