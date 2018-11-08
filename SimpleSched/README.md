
# Simple Round Robin Scheduler ... PI 1,2,3 AARCH32
Simple Round Robin Scheduler I am working on as a lead in to Task Switchers and RTOS. 
>
>
This is the basic start point where tasks at same priority basically roll thru executing in round robin. It is assumed the tasks would always complete within the time between the schedule intervals. It is setup to run task one at 5 seconds and task two at 9 seconds so it's quite obvious to see.
>
>
Makefile instructions:
Make Pi1   ... creates a Pi1 kernel.img in directory DiskImg 
>
Make Pi2   ... creates a Pi2 kernel7.img in directory DiskImg
>
Make Pi3   ... creates a Pi3 kernel8-32.img in directory DiskImg
>
OR
>
Make       ... creates a Pi3 kernel8-32.img in directory DiskImg
>
Make Clean ... clears all temp files which are in the directory Build
>
>
>
There tasks can be made to one shot and or execute immediately on startup as well as the standard execute repeatedly on a timed interval.
>
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test
>
>
This example might be useful for some basic slow task situations but it is really targetted as a learning step. In the next example we will add context switching and never ending tasks to create a simple task switcher.
