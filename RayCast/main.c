#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uint16_t, uint32_t etc
#include <stdio.h>				// Needed for printf
#include <string.h>
#include <math.h>
#include "rpi-smartstart.h"		
#include "rpi-BasicHardware.h"

static bool lit = false;
void c_irq_handler (void) {
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag
}

void c_irq_identify_and_clear_source (void) {
	/* Clear the ARM Timer interrupt - it's the only interrupt we have enabled, 
	   so we want don't have to work out which interrupt source caused us to interrupt */
	ARMTIMER->Clear = 1;											// Write any value to register to clear irq ... PAGE 198

	/* As we are running nested interrupts we must clear the Pi Irq controller,
	   the timer irq pending is bit 0 of pending register 1 as it's irq 0 */
	IRQ->IRQPending1 &= ~0x1;										// Clear timer pending irq bit 0
}

typedef struct __attribute__((__packed__)) structRGBA {
	union {
		struct {
			uint8_t B;
			uint8_t G;
			uint8_t R;
			uint8_t A;
		};
		RGBA rawRGBA;
		uint32_t raw32;
	};
} RGBACOLOR;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{              DEFINE THE 16 BASIC VGA GRAPHICS COLORS AS RBGA              }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define RGBA_Black  (RGBACOLOR){.R=0x00, .G=0x00, .B=0x00, .A=0xFF}		// 0 = Black
#define RGBA_Blue	(RGBACOLOR){.R=0x00, .G=0x00, .B=0x7F, .A=0xFF}		// 1 = Blue
#define RGBA_Green	(RGBACOLOR){.R=0x00, .G=0x7F, .B=0x00, .A=0xFF}		// 2 = Green 
#define RGBA_Cyan	(RGBACOLOR){.R=0x00, .G=0x7F, .B=0x7F, .A=0xFF}		// 3 = Cyan
#define RGBA_Red	(RGBACOLOR){.R=0x7F, .G=0x00, .B=0x00, .A=0xFF}		// 4 = Red
#define RGBA_Magenta (RGBACOLOR){.R=0x7F, .G=0x00, .B=0x7F, .A=0xFF}		// 5 = Magenta
#define RGBA_Brown	(RGBACOLOR){.R=0x7F, .G=0x7F, .B=0x00, .A=0xFF}		// 6 = Brown
#define RGBA_LightGray (RGBACOLOR){.R=0xAF, .G=0xAF, .B=0xAF, .A=0xFF}	// 7 = LightGray
#define RGBA_DarkGray (RGBACOLOR){.R=0x4F, .G=0x4F, .B=0x4F, .A=0xFF}	// 8 = DarkGray
#define RGBA_LightBlue (RGBACOLOR){.R=0x00, .G=0x00, .B=0xFF, .A=0xFF}	// 9 = Light Blue
#define RGBA_LightGreen (RGBACOLOR){.R=0x00, .G=0xFF, .B=0x00, .A=0xFF}	// 10 = Light Green
#define RGBA_LightCyan (RGBACOLOR){.R=0x00, .G=0xFF, .B=0xFF, .A=0xFF}	// 11 = Light Cyan
#define RGBA_LightRed (RGBACOLOR){.R=0xFF, .G=0x00, .B=0x00, .A=0xFF}	// 12 = Light Red
#define RGBA_LightMagenta (RGBACOLOR){.R=0xFF, .G=0x00, .B=0xFF, .A=0xFF}// 13 = Light Magenta
#define RGBA_Yellow	(RGBACOLOR){.R=0xFF, .G=0xFF, .B=0x00, .A=0xFF}		// 14 = Yellow
#define RGBA_White (RGBACOLOR){.R=0xFF, .G=0xFF, .B=0xFF, .A=0xFF}		// 15 = White

#define mapWidth 24
#define mapHeight 24

int worldMap[mapWidth][mapHeight]=
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};


/* We may wish to change between float type  this makes it easy */
#define CFLOAT double				// We may choose float, double, long double etc
#define CFLOAT_ZERO ((CFLOAT)0.0)
#define CFLOAT_ONE ((CFLOAT)1.0)

/*-LineIntersect2D----------------------------------------------------------}
{																			}
{ REF: http://paulbourke.net/geometry/pointlineplane/						}
{																			}
{ Given two line segments (x1,y1)-(x2,y2) &  (x3,y3)-(x4,y4) we calculate   }
{ the intersection point of the two lines. If line segment only flag set    }
{ the intersection must be within the line segments, otherwise interesction }
{ assumes each line is extended to infinite length and returns intersection }
{																			}
{ The pointers to results intersect Ipx, Ipy, can be NULL if that result    }
{ is not required.															}
{																			}
{ RETURN FALSE:																}
{	If the two lines are parallel.											}
{	If the two lines are coincident (one and the same line).				}
{	Line segment flag set and intersection point is outside the segments	}
{																			}
{ RETURN TRUE:																}
{	Ipx, Ipy will be the intersection point returned						}
{																			}
{ 8aug2017 LdB																}
---------------------------------------------------------------------------*/
bool LineIntersect2D (_In_ int32_t x1, _In_ int32_t y1,				// First line segement first coord
					  _In_ int32_t x2, _In_ int32_t y2,				// First line segement second coord
					  _In_ int32_t x3, _In_	int32_t y3,				// Second line segment first coord
					  _In_ int32_t x4, _In_ int32_t y4, 			// Second line segment second cord
					  _Out_ int32_t* Ipx, _Out_ int32_t* Ipy,		// Interection point return
					  bool LineSegmentOnly) 						// Line segment intersect only flag
{
	int_fast32_t UaNum, Denom, Xa, Xb, Xc, Ya, Yb, Yc;
	CFLOAT Ua;

	Xa = x4 - x3;													// Line 2 dx
	Xc = x2 - x1;													// Line 1 dx
	Yb = y4 - y3;													// Line 2 dy
	Yc = y2 - y1;													// Line 2 dy
	Denom = (Yb*Xc) - (Xa*Yc);										// Denominator of two equations
	if (Denom == 0) return (false);									// The two lines are parallel and/or coincident
	Xb = x1 - x3;													// Dx between first x coord of each line
	Ya = y1 - y3;													// Dy between first y corrd of each line
	UaNum = (Xa*Ya) - (Yb*Xb);										// Ua numerator
	Ua = (CFLOAT)UaNum / (CFLOAT)Denom;								// Calculate Ua
	if (LineSegmentOnly) {											// Want intersect point of line segments
		int_fast32_t  UbNum;
		CFLOAT Ub;
		if (Ua < CFLOAT_ZERO || Ua > CFLOAT_ONE) return (false);	// Line intersection outside line segement 1
		UbNum = (Xc*Ya) - (Yc*Xb);									// Ub numerator 
		Ub = (CFLOAT)UbNum / (CFLOAT)Denom;							// Calculate Ub
		if (Ub < CFLOAT_ZERO || Ub > CFLOAT_ONE) return (false);	// Line intersection outside line segement 2
	}
	if (Ipx) (*Ipx) = x1 + (int_fast32_t)(Ua*Xc);					// Calculate and return intersect x point		
	if (Ipy) (*Ipy) = y1 + (int_fast32_t)(Ua*Yc);					// Calculate and return intersect y point
	return (true);													// return true for success
}

/*-VERTEX_TYPE--------------------------------------------------------------}
{  Units your vertex points are in typically int32_t, uint64_t or doubles.  }
{--------------------------------------------------------------------------*/
typedef int_fast32_t VERTEX_TYPE;									// For our example we are 32 bit ints

/*-VERTEX FLAGS-------------------------------------------------------------}
{ These are flags for our vertex points above so our shape is made of lines }
{ and bezier linked up (IE exactly what Win32 GetGlyphOutline returns).		}
{--------------------------------------------------------------------------*/
#define VF_NORMAL 			0x00

#define VF_DIRECTIONS		0x07									// Lower 3 bits = direction flags (0-7)
#define VF_LIFTCLOSE 		0x08									// Open contour .. close vertexes in air

#define VF_BEZONGRID 		0x10									// Bezier on grid point
#define VF_QUADBEZOFFGRID 	0x20									// Quad bezier off grid point
#define VF_CUBICBEZOFFGRID 	0x40									// Cubic bezier off grid point

#define VF_BEZOFFGRID 		0x60									// Any bezier off grid point
#define VF_BEZIER 			0x70									// Any bezier on or off grid point

#define VF_JOINEDCONTOUR 	0x80									// Contour joins to next

/*-CONTOUR DIRECTION--------------------------------------------------------}
{                        Contour direction enumeration						}
{--------------------------------------------------------------------------*/
enum CONTOUR_DIR {
	CW_DIR = 0,														// Clockwise contour direction
	CCW_DIR = 1,													// Counter clockwise contour direction
	UNKNOWN_DIR = 2													// Unknown contour direction
};

/*---------------------------------------------------------------------------}
{                        VERTEX2D RECORD DEFINITION                          }
{---------------------------------------------------------------------------*/
typedef struct Vertex2D {
	uint_fast32_t v_flags;											// Vertex flags
	VERTEX_TYPE x;													// Vertex x co-ord
	VERTEX_TYPE y;													// Vertex y co-ord
	struct Vertex2D* next;											// Next vertex
	struct Vertex2D* prev;											// Prev vertex
} VERTEX2D, *PVERTEX2D;


/*--------------------------------------------------------------------------}
{                       CONTOUR2D RECORD DEFINITION							}
{--------------------------------------------------------------------------*/
typedef struct Contour2D {
	enum CONTOUR_DIR dir;											// Direction of contour
	uint32_t index;													// Label index of contour 
	VERTEX_TYPE xmin;												// Contour min x value
	VERTEX_TYPE ymin;												// Contour min y value
	VERTEX_TYPE xmax;												// Contour max x value
	VERTEX_TYPE ymax;												// Contour max y value
	PVERTEX2D firstvertex;											// First vertex on this contour
	struct Contour2D* inside_contours;								// Inside paired contours
	struct Contour2D* nextpeer;										// Next peer contour
	struct Contour2D* prevpeer;										// Prev peer contour
} CONTOUR2D, *PCONTOUR2D;


VERTEX2D InBox1[4] = {
	{ .x = 6, .y = 5, .next = &InBox1[1], .prev = &InBox1[3] },
	{ .x = 10, .y = 5, .next = &InBox1[2], .prev = &InBox1[0] },
	{ .x = 10, .y = 9, .next = &InBox1[3], .prev = &InBox1[1] },
	{ .x = 6, .y = 9, .next = &InBox1[0], .prev = &InBox1[2] }
};

CONTOUR2D InnerBox = {
	.dir = CW_DIR,
	.index = 1,
	.xmin = 6,
	.ymin = 5,
	.xmax = 10,
	.ymax = 9,
	.firstvertex = &InBox1[1],
	.inside_contours = 0,
	.nextpeer = 0,
	.prevpeer = 0,
};

VERTEX2D Box[4] = {
	{.x =0, .y= 0, .next=&Box[1], .prev=&Box[3]},
	{.x =23, .y=0, .next=&Box[2], .prev=&Box[0]},
	{.x =23, .y=23, .next=&Box[3], .prev=&Box[1]},
	{.x =0,	 .y=23, .next=&Box[0], .prev=&Box[2]}
};

CONTOUR2D Level = {
	.dir = CW_DIR,
	.index = 0,
	.xmin = 0,
	.ymin = 0,
	.xmax = 23,
	.ymax = 23,
	.firstvertex = &Box[1],
	.inside_contours = &InnerBox,
	.nextpeer = 0,
	.prevpeer = 0,
};


#define CFLOAT double

int grWth = 1280;
int grHt = 1024;
int main (void) {
	if (SetMaxCPUSpeed() == false) DeadLoop();
	PiConsole_Init(grWth, grHt, 32);
	printf("SmartStart compiled for Arm%d, AARCH%d with %u core s/w support\n",
		RPi_CompileMode.CodeType, RPi_CompileMode.AArchMode * 32 + 32,
		(unsigned int)RPi_CompileMode.CoresSupported);							// Write text
	int posX = 12, posY = 16;  //x and y start position
	CFLOAT dirX = -1, dirY = -1; //initial direction vector
	CFLOAT planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane

	//double time = 0; //time of current frame
	//double oldTime = 0; //time of previous frame

    /* Enable the timer interrupt IRQ */
	IRQ->EnableBasicIRQs.Enable_Timer_IRQ = true;


    /* Setup the system timer interrupt */
	ARMTIMER->Load = 0x400;

    /* Setup the ARM Timer */
	ARMTIMER->Control.Counter32Bit = true;
	ARMTIMER->Control.Prescale = Clkdiv256;
	ARMTIMER->Control.TimerIrqEnable = true;
	ARMTIMER->Control.TimerEnable = true;

    /* Enable interrupts! */
    EnableInterrupts();

	while (1) {

		for (uint_fast32_t x = 0; x < grWth; x++) {

			//calculate ray position and direction
			CFLOAT cameraX = 2 * x / (CFLOAT)grWth - 1; //x-coordinate in camera space
			int_fast32_t rayPosX = posX;
			int_fast32_t rayPosY = posY;
			CFLOAT rayDirX = dirX + planeX * cameraX;
			CFLOAT rayDirY = dirY + planeY * cameraX;
			
			// box of the map we start in
			int_fast32_t mapX = rayPosX;
			int_fast32_t mapY = rayPosY;

			//length of ray from current position to next x or y-side
			CFLOAT sideDistX;
			CFLOAT sideDistY;

			//length of ray from one x or y-side to next x or y-side
			CFLOAT deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
			CFLOAT deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));


			//what direction to step in x or y-direction (either +1 or -1)
			int_fast8_t stepX;
			int_fast8_t stepY;


					  //calculate step and initial sideDist
			if (rayDirX < 0){
				stepX = -1;
				sideDistX = (rayPosX - mapX) * deltaDistX;
			} else {
				stepX = 1;
				sideDistX = (mapX + 1 - rayPosX) * deltaDistX;
			}
			if (rayDirY < 0) {
				stepY = -1;
				sideDistY = (rayPosY - mapY) * deltaDistY;
			} else {
				stepY = 1;
				sideDistY = (mapY + 1 - rayPosY) * deltaDistY;
			}

			

			//perform DDA
			bool hit = false;	// was there a wall hit?
			bool hit_NS;		//was a NS or a EW wall hit?
			while (!hit) {
				//jump to next map square, OR in x-direction, OR in y-direction
				if (sideDistX < sideDistY) {
					sideDistX += deltaDistX;
					mapX += stepX;
					hit_NS = false;
				} else {
					sideDistY += deltaDistY;
					mapY += stepY;
					hit_NS = true;
				}
				//Check if ray has hit a wall
				if (worldMap[mapX][mapY] > 0) hit = true;
			}
	
			//Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
			int lineHeight;
			if (hit_NS) lineHeight = grHt / ((CFLOAT)(mapY - rayPosY + (1 - stepY) / 2) / rayDirY);
				else lineHeight = grHt/((CFLOAT)(mapX - rayPosX + (1 - stepX) / 2) / rayDirX);

			//calculate lowest and highest pixel to fill in current stripe
			int drawStart = -lineHeight / 2 + grHt / 2;
			if(drawStart < 0)drawStart = 0;
			int drawEnd = lineHeight / 2 + grHt / 2;
			if(drawEnd >= grHt)drawEnd = grHt - 1;

			//choose wall color
			RGBACOLOR color;
			switch(worldMap[mapX][mapY]) {
				case 1:  color = RGBA_Red;  break; //red
				case 2:  color = RGBA_Green;  break; //green
				case 3:  color = RGBA_Blue;   break; //blue
				case 4:  color = RGBA_White;  break; //white
				default: color = RGBA_Yellow; break; //yellow
			}

			//give x and y sides different brightness
			if (hit_NS) {color.R = color.R / 2; color.G = color.G / 2; color.B = color.B / 2; }

			//draw the pixels of the stripe as a vertical line
			//verLine(x, drawStart, drawEnd, color);
			PiConsole_VertLine(x, 0, drawStart, 0x0);
			PiConsole_VertLine(x, drawStart, drawEnd, color.raw32);
			PiConsole_VertLine(x, drawEnd, grHt, 0x0);

		}
		//timing for input and FPS counter
		//oldTime = time;
		//time = getTicks();
		//double frameTime = (time - oldTime) / 1000.0; //frameTime is the time this frame has taken, in seconds
		//printf(1.0 / frameTime); //FPS counter
		//redraw();

		/*for (int y = 0; y < grHt; y++) {
			uint32_t lineOffs = y * grWth;
			for (int x = 0; x < grWth; x++) {
				PiConsole_VertLine(x, y, y, Buffer[lineOffs + x].raw32);
			}
		}

		memset(&Buffer[0], 0, sizeof(Buffer));*/
		

		//speed modifiers
		//double moveSpeed = frameTime * 5.0; //the constant value is in squares/second
		CFLOAT rotSpeed = 0.02; // frameTime * 3.0; //the constant value is in radians/second
		

		//readKeys();
		//move forward if no wall in front of you
		//if (keyDown(SDLK_UP))
		//{
		//	if(worldMap[int(posX + dirX * moveSpeed)][int(posY)] == false) posX += dirX * moveSpeed;
		//	if(worldMap[int(posX)][int(posY + dirY * moveSpeed)] == false) posY += dirY * moveSpeed;
		//}
		//move backwards if no wall behind you
		//if (keyDown(SDLK_DOWN))
		//{
		//	if(worldMap[int(posX - dirX * moveSpeed)][int(posY)] == false) posX -= dirX * moveSpeed;
		//	if(worldMap[int(posX)][int(posY - dirY * moveSpeed)] == false) posY -= dirY * moveSpeed;
		//}
		//rotate to the right
		//if (keyDown(SDLK_RIGHT))
		//{
			//both camera direction and camera plane must be rotated
			CFLOAT oldDirX = dirX;
			CFLOAT cf, sf;
			cf = cos(-rotSpeed);
			sf = sin(-rotSpeed);

			dirX = dirX * cf - dirY * sf;
			dirY = oldDirX * sf + dirY * cf;
			CFLOAT oldPlaneX = planeX;
			planeX = planeX * cf - planeY * sf;
			planeY = oldPlaneX * sf + planeY * cf;
		//}
		//rotate to the left
		//if (keyDown(SDLK_LEFT))
		//{
			//both camera direction and camera plane must be rotated
		//	double oldDirX = dirX;
		//	dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
		//	dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
		//	double oldPlaneX = planeX;
		//	planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
		//	planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
		//}

	}

	return(0);
}