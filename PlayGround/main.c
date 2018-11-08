#include <stdbool.h>			// Needed for bool
#include <stdint.h>				// Needed for uint8_t, uint16_t, uint32_t
#include <stdlib.h>				// Needed for abs
#include <stdarg.h>				// Needed for va_list 
#include <string.h>				// Needed for memcpy
#include <stdio.h>				// Needed for snprintf				
#include <math.h>				// Needed for sin/cos

#include "rpi-SmartStart.h"
#include "emb-stdio.h"

int __errno = 0;
static HDC Screen = { 0 };

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{              RBGA COLOUR SPECIFIC MACROS TO BUILD AND SPLIT               }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define BLUE_VALUE(colour)   ( (uint8_t)(((uint32_t)colour) & 0xFF) )				// Peel the BLUE byte out of an RGBA  
#define GREEN_VALUE(colour)  ( (uint8_t)(((uint32_t)colour >> 8) & 0xFF) )			// Peel the GREEN byte out of an RGBA
#define RED_VALUE(colour)    ( (uint8_t)(((uint32_t)colour >> 16) & 0xFF) )			// Peel the RED byte out of an RGBA
#define ALPHA_VALUE(colour)  ( (uint8_t)(((uint32_t)colour >> 24) & 0xFF) )			// Peel the ALPHA byte out of an RGBA


#define RGBA(r, g, b, a)  ( (COLORREF) ( (uint32_t)(b) | (uint32_t) ((uint32_t)g << 8) | (uint32_t) ((uint32_t)r << 16) | (uint32_t) ((uint32_t)a << 24) ) )




double light[3] = { -100, 0, 100 };
void normalize(double * v)
{
	double len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= len; v[1] /= len; v[2] /= len;
}

double dot(double *x, double *y)
{
	double d = x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
	return d < 0 ? -d : 0;
}

typedef struct { double cx, cy, cz, r; } sphere_t;

/* positive shpere and negative sphere */
sphere_t pos = { 40, 40, 0, 40 }, neg = { 1, 1, -6, 40 };

/* check if a ray (x,y, -inf)->(x, y, inf) hits a sphere; if so, return
the intersecting z values.  z1 is closer to the eye */
int hit_sphere(sphere_t *sph, double x, double y, double *z1, double *z2)
{
	double zsq;
	x -= sph->cx;
	y -= sph->cy;
	zsq = sph->r * sph->r - (x * x + y * y);
	if (zsq < 0) return 0;
	zsq = sqrt(zsq);
	*z1 = sph->cz - zsq;
	*z2 = sph->cz + zsq;
	return 1;
}

void deathstar(double k, double ambient, int PosX, int PosY) {
	int i, j, intensity, hit_result;
	double b;
	double vec[3], x, y, zb1, zb2, zs1, zs2;
	int xCursor = PosX;
	int yCursor = PosY;
	for (i = floor(pos.cy - pos.r); i <= ceil(pos.cy + pos.r); i++) {
		y = i + .5;
		for (j = floor(pos.cx - 2 * pos.r); j <= ceil(pos.cx + 2 * pos.r); j++) {
			x = (j - pos.cx) / 2. + .5 + pos.cx;

			/* ray lands in blank space, draw bg */
			if (!hit_sphere(&pos, x, y, &zb1, &zb2))
				hit_result = 0;

			/* ray hits pos sphere but not neg, draw pos sphere surface */
			else if (!hit_sphere(&neg, x, y, &zs1, &zs2))
				hit_result = 1;

			/* ray hits both, but pos front surface is closer */
			else if (zs1 > zb1) hit_result = 1;

			/* pos sphere surface is inside neg sphere, show bg */
			else if (zs2 > zb2) hit_result = 0;

			/* back surface on neg sphere is inside pos sphere,
			the only place where neg sphere surface will be shown */
			else if (zs2 > zb1) hit_result = 2;
			else		    hit_result = 1;

			switch (hit_result) {
			case 0:			
				SetPixel(Screen, xCursor, yCursor, RGBA(0x0, 0x0, 0x0, 0xff));
				xCursor++;
				continue;
			case 1:
				vec[0] = x - pos.cx;
				vec[1] = y - pos.cy;
				vec[2] = zb1 - pos.cz;
				break;
			default:
				vec[0] = neg.cx - x;
				vec[1] = neg.cy - y;
				vec[2] = neg.cz - zs2;
			}

			normalize(vec);
			b = pow(dot(light, vec), k) + ambient;
			intensity = (1 - b) * (256 - 1);
			if (intensity < 0) intensity = 0;
			if (intensity >= 256 - 1)
				intensity = 256 - 2;
			uint8_t shade = intensity;
			SetPixel(Screen, xCursor, yCursor, RGBA(shade, shade, shade, 0xff));
			xCursor++;
		}
		xCursor = PosX;
		yCursor++;
	}
}

/* IFS DISPLAY DEMO STUFF */

typedef struct {
	double a[4];
	double b[4];
	double c[4];
	double d[4];
	double e[4];
	double f[4];
	double p[4];
	double xrange;
	double yrange;
	int maxsize;
} ifs_struct;

ifs_struct fern0_ifs = { 
	.a[0] = 0,    .a[1] = 0.2,   .a[2] = -0.15, .a[3] = 0.85,
	.b[0] = 0,    .b[1] = -0.26, .b[2] = 0.28,  .b[3] = 0.04,
	.c[0] = 0,	  .c[1] = 0.23,  .c[2] = 0.26,  .c[3] = -0.04,
	.d[0] = 0.16, .d[1] = 0.22,  .d[2] = 0.24,  .d[3] = 0.85,
	.e[0] = 0,    .e[1] = 0,     .e[2] = 0,     .e[3] = 0,
	.f[0] = 0,    .f[1] = 1.6,   .f[2] = 0.44,  .f[3] = 1.6,
	.p[0] = 0.1,  .p[1] = 0.07,  .p[2] = 0.07,  .p[3] = 0.85,
	.xrange = 0.5, .yrange = 1.0, .maxsize = 10
};

ifs_struct fern1_ifs = {
	.a[0] = 0,		.a[1] = 0.2,	.a[2] = -0.15,	.a[3] = 0.75,
	.b[0] = 0,		.b[1] = -0.26,	.b[2] = 0.28,	.b[3] = 0.04,
	.c[0] = 0,		.c[1] = 0.23,	.c[2] = 0.26,	.c[3] = -0.04,
	.d[0] = 0.16,	.d[1] = 0.22,	.d[2] = 0.24,	.d[3] = 0.85,
	.e[0] = 0,		.e[1] = 0,		.e[2] = 0,		.e[3] = 0,
	.f[0] = 0,		.f[1] = 1.6,	.f[2] = 0.44,	.f[3] = 1.6,
	.p[0] = 0.1,	.p[1] = 0.08,	.p[2] = 0.08,	.p[3] = 0.74,
	.xrange = 0.5, .yrange = 1.0, .maxsize = 10
};

ifs_struct maple_ifs = {
	.a[0] = 0.14,	.a[1] = 0.43,	.a[2] = 0.45,	.a[3] = 0.49,
	.b[0] = 0.01,	.b[1] = 0.52,	.b[2] = -0.49,	.b[3] = 0.0,
	.c[0] = 0,		.c[1] = -0.45,	.c[2] = 0.47,	.c[3] = 0.0,
	.d[0] = 0.51,	.d[1] = 0.5,	.d[2] = 0.47,	.d[3] = 0.51,
	.e[0] = -0.08,	.e[1] = 1.49,	.e[2] = -1.62,	.e[3] = 0.02,
	.f[0] = -1.31,	.f[1] = -0.75,	.f[2] = -0.74,	.f[3] = 1.62,
	.p[0] = 0.1,	.p[1] = 0.35,	.p[2] = 0.35,	.p[3] = 0.20,
	.xrange = 0.5,  .yrange = 0.5, .maxsize = 10
};

#include <limits.h>  // needed for INT_MAX
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
static ifs_struct* CurrentIfs = &fern0_ifs;
static int curIfs = 0;
static int ifsTick = 0;
static double xlast = 0;
static double ylast = 0;
void DrawIFS (ifs_struct* ifs, int NXY, int xPos, int yPos) {
	int k, ix, iy;
	for (int i = 0; i < 500; i++) {
		ifsTick++;
		double r = (double)rand() / (double)INT_MAX;   // convert 0.0 ... 1.0 probability
		// Select k based on probability
		if (r < ifs->p[0]) k = 0;
		else if (r < (ifs->p[0] + ifs->p[1])) k = 1;
		else if (r < (ifs->p[0] + ifs->p[1] + ifs->p[2])) k = 2;
		else k = 3;

		double tmpx = ifs->a[k] * xlast + ifs->b[k] * ylast + ifs->e[k];
		double tmpy = ifs->c[k] * xlast + ifs->d[k] * ylast + ifs->f[k];
		

		ix = xPos + (int)round(NXY * ifs->xrange + tmpx * NXY / ifs->maxsize);   
		iy = yPos + (int)round(NXY * ifs->yrange - tmpy * NXY / ifs->maxsize);
		xlast = tmpx;
		ylast = tmpy;

		int colg = rand();
		int colr = rand();
		int colb = rand();
		SetPixel(Screen, ix, iy, RGBA((colr & 0x3f), (colg & 0xFF), (colb & 0x3f), 0xff ));

	}
	if (ifsTick >= 100000) {
		ifsTick = 0;
		curIfs++;
		if (curIfs >= 3) curIfs = 0;
		switch (curIfs) {
			case 0:
				CurrentIfs = &fern0_ifs;
				break;
			case 1:
				CurrentIfs = &fern1_ifs;
				break;
			case 2:
				CurrentIfs = &maple_ifs;
				break;
		}
		xlast = 0;
		ylast = 0;
		SetDCBrushColor(Screen, RGBA(0, 0, 0, 0xFF));
		Rectangle(Screen, xPos, yPos, xPos + NXY, yPos + NXY);
	}
}

/* MATRIX RAIN DEMO STUFF */

#define TRAILCOUNT 75
#define TRAILWIDTH 450
#define TRAILHEIGHT 250
static int trailXPos = 0;
static int trailYPos = 0;

struct trail {
	int xPos;
	int yPos;
	int end;
	int speed;
	char dispChar;
} trails[TRAILCOUNT];

void init_trail(struct trail *trail) {
	trail->xPos = trailXPos + (rand() % TRAILWIDTH);
	trail->yPos = 0;// rand() % TRAILHEIGHT;
	trail->end = rand() % TRAILHEIGHT;
	trail->speed = (rand() % 32) + 4;
	trail->dispChar = (rand() % 95) + 32;

}

void trail_update(void) {

	for (int i = 0; i < TRAILCOUNT; i++) {

		//WriteChar(trails[i].xPos, trailYPos + trails[i].yPos, trails[i].dispChar, RGBA(0, 0, 0, 0xFF));

		trails[i].yPos += rand() % trails[i].speed;
		trails[i].dispChar = (rand() % 95) + 32;
		//trails[i].end -= trails[i].speed;

		uint8_t randCol = rand() % 0xFF;

		if (trails[i].yPos == trails[i].end - 1) {
			SetTextColor(Screen, RGBA(0xFF, 0xFF, 0xFF, 0xFF));
			TextOut(Screen, trails[i].xPos, trailYPos + trails[i].yPos, &trails[i].dispChar, 1);
		}
		else {
			SetTextColor(Screen, RGBA(0, randCol, 0, 0xFF));
			TextOut(Screen, trails[i].xPos, trailYPos + trails[i].yPos, &trails[i].dispChar, 1);
		}

		if ((trails[i].yPos >= TRAILHEIGHT) || (trails[i].yPos == trails[i].end)) {
			//WriteChar(trails[i].xPos, trailYPos + trails[i].yPos, trails[i].dispChar, RGBA(0, 0, 0, 0xFF));
			SetDCBrushColor(Screen, RGBA(0, 0, 0, 0xFF));
			Rectangle(Screen, trails[i].xPos, trailYPos, trails[i].xPos + BitFontWth, trailYPos + trails[i].yPos + BitFontHt);
			init_trail(&trails[i]);
		}
	}
}

void Matrix_Rain(int xPos, int yPos) {
	trailXPos = xPos;
	trailYPos = yPos;
	for (int i = 0; i < TRAILCOUNT; i++)
		init_trail(&trails[i]);
}


/*--------------------------------------------------------------------------
 Entry point for start of C code
--------------------------------------------------------------------------*/
void main (void) {
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen
	Screen = GetConsoleDC();

	uint64_t colourChangeTime = timer_getTickCount64() + 3000000ul; // 3 seconds from now
	SetDCBrushColor(Screen, RGBA(0, 0, 0xff, 0xff));
	Rectangle(Screen, 100, 100, 200, 200);
	SetTextColor(Screen, RGBA(0xff, 0, 0, 0xff));
	TextOut(Screen, 300, 140, "HELLO WORLD!!", 13);


	Matrix_Rain(20, 295);

	double ang = 0;
	while (1) {
		set_Activity_LED(false); // Activity led on

		light[1] = cos(ang * 2);
		light[2] = cos(ang);
		light[0] = sin(ang);
		normalize(light);
		ang += .05;

		deathstar(2.0, .3, 500, 100);

		set_Activity_LED(false); // Activity led off


		DrawIFS(CurrentIfs, 250, 530, 300);


		if (timer_getTickCount64() >= colourChangeTime) {
			COLORREF randCol = { .Raw32 = rand() };
			randCol.rgba.rgbAlpha = 0xFF;
			SetDCBrushColor(Screen, randCol);
			Rectangle(Screen, 100, 100, 200, 200);
			colourChangeTime = timer_getTickCount64() + 3000000ul; // 3 seconds from now
		}

		trail_update();
	}
}
