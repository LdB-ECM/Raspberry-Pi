
# USB alpha ... PI3 AARCH64 and PI 1,2,3 AARCH32
My redux of CSUD (Chadderz's Simple USB Driver) by Alex Chadwick was converted to 32/64 bit compatible code. I also merged nested interrupt code which blinks the LED while the USB runs. 

Just in the process of updating ... I will update details here shortly

If you just want to see it just put the files in the DiskImg directory on a formatted SD card and place in your Pi.

To compile edit the makefile so the compiler path matches your compiler:
>
For Pi1: 
>     Make Pi1
For Pi2:
>     Make Pi2
For Pi3 in 32 Bit:
>     Make Pi3
For Pi3 in 64 Bit:
>     Make Pi3-64
     
To clean for full compile:     
>     Make Clean
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/USB64_alpha.jpg?raw=true)
