# ACCELERATED GUI STEP1 (Pi1, 2, 3 - AARCH32, Pi3 AARCH64)
>
Code background: 
There was a discussion on the Pi forum about using the VC4 GPU for window draw functions so I decided to do a quick hack to show how it would be done.
>
So this is the first quick cut we have 3 window areas Red (bottom), Green (middle), Blue (topmost) and the code does a simple move the green and red window areas around the screen. Obviously the Z order needs should be maintained whenever the areas overlap each other.
