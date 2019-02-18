# ACCELERATED GRAPHICS (Pi1, 2, 3 - AARCH32, Pi3 AARCH64)
>
Code background: 
http://latchup.blogspot.com.au/2016/02/life-of-triangle.html
>
You will require this reference
https://docs.broadcom.com/docs/12358545
>
I currently have the C file called rpi-GLES.C we have no GLES framework yet we are beating up the GL pipe inside the VC4 directly.

The first sample here passes in the raw active onscreen framebuffer. As we start to animate stuff I strongly suggest you don't do this as the render will produce screen tear. 
What I am going to do and I suggest you do the same is setup the virtual screen size to twice that of the screen. You render to the half the framebuffer that isn't visible.
You can control which half currently displayed by the mailbox Set virtual offset Tag: 0x00048009. So you should be flip-flopping between rendering and displaying the 
different halfs of the virtual screen the new render always going to the non displayed half.

As we get a bit further I will do more of a write-up, feel free to ask questions on the Raspberry forum
https://www.raspberrypi.org/forums/viewtopic.php?f=72&p=1206923#p1206923

![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/GL_code2.jpg?raw=true)
