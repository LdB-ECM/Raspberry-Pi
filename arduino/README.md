# ARDUINO STRINGS
>
This is a quick conversion of the Arduino strings unit to what should be linux standard C++. Linux lacks the itoa, ltoa etc so they are implemented inside the Ardrino wstrings.c file. A few #defines from print.h and pgmspace.h were copied to the top of wstrings.h as they seem to be used with the strings. 
>
I only did very basic checks and leave full testing to anyone who wants to use it :-)
The basic tests I did were the tutorial code from the two sites
>
https://www.tweaking4all.com/hardware/arduino/arduino-programming-course/arduino-programming-part-7/
http://www.arduino.org/learning/reference/stringobject
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/Arduino1.jpg)
