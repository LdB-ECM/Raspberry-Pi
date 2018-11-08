#include <stdio.h>
#include "wstring.h"

int main (void) {

	/* sample from https://www.tweaking4all.com/hardware/arduino/arduino-programming-course/arduino-programming-part-7/ */
	String Name = "Hans";
	printf("Name = %s\n", Name.c_str());

	// determine length
	printf("Length of Name with strlen: ");
	printf("%i\n", Name.length());

	Name.concat(" has two nephews, called Bram and Max!");
	printf("Name = %s\n", Name.c_str());

	// determine length
	printf("Length of Name with strlen: %d\n", Name.length());


	// Alternative way of adding text
	Name = "Hans"; // reset to original text
	printf("Name = %s\n", Name.c_str());
	Name += " has 2 nephews";
	Name = Name + ", the are called Bram" + " and Max";
	printf("Name = %s\n", Name.c_str());

	// this won't work with arrays
	Name = "A large number is: ";
	Name = Name + 32768;
	printf("Name = %s\n", Name.c_str());

	/* from http://www.arduino.org/learning/reference/stringobject */
	String strOne = "Hello String"; // using a constant String
	printf("String = %s\n", strOne.c_str());
	strOne = String('a'); // converting a constant char into a String
	printf("String = %s\n", strOne.c_str());
	String strTwo = String("This is a string"); // converting a constant string into a String object
	printf("String = %s\n", strTwo.c_str());
	strOne = String(strTwo + " with more"); // concatenating two strings
	printf("String = %s\n", strOne.c_str());
	strOne = String(13); // using a constant integer
	printf("String = %s\n", strOne.c_str());
	strOne = String(0, DEC); // using an int and a base
	printf("String = %s\n", strOne.c_str());
	strOne = String(45, HEX); // using an int and a base (hexadecimal)
	printf("String = %s\n", strOne.c_str());
	strOne = String(255, BIN); // using an int and a base (binary)
	printf("String = %s\n", strOne.c_str());
	strOne = String(12345678, DEC); // using a long and a base
	printf("String = %s\n", strOne.c_str());

   while (1){}
   return 0;
}