#include "Pch.h"
#include "Base.h"

// http://members.chello.at/easyfilter/bresenham.html

#pragma warning(disable: 4244)

//=================================================================================================
void PlotLine(int x0, int y0, int x1, int y1, float th, vector<Pixel>& pixels)
{                              /* plot an anti-aliased line of width th pixel */
	int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1; 
	int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1; 
	float err, e2 = sqrt(float(dx*dx+dy*dy));                            /* length */

	dx *= 255/e2; dy *= 255/e2; th = 255*(th-1);               /* scale values */

	if (dx < dy)
	{                                               /* steep line */
		x1 = round((e2+th/2)/dy);                          /* start offset */
		err = x1*dy-th/2;                  /* shift error value to offset width */
		for (x0 -= x1*sx; ; y0 += sy)
		{
			pixels.push_back(Pixel(x1 = x0, y0, err));	/* aliasing pre-pixel */
			for (e2 = dy-err-th; e2+dy < 255; e2 += dy)  
				pixels.push_back(Pixel(x1 += sx, y0));                      /* pixel on the line */
			pixels.push_back(Pixel(x1+sx, y0, e2));                    /* aliasing post-pixel */
			if (y0 == y1)
				break;
			err += dx;                                                 /* y-step */
			if (err > 255)
			{
				err -= dy;
				x0 += sx;
			}                    /* x-step */ 
		}
	}
	else
	{                                                      /* flat line */
		y1 = round((e2+th/2)/dx);                          /* start offset */
		err = y1*dx-th/2;                  /* shift error value to offset width */
		for (y0 -= y1*sy; ; x0 += sx)
		{
			pixels.push_back(Pixel(x0, y1 = y0, err));                  /* aliasing pre-pixel */
			for (e2 = dx-err-th; e2+dx < 255; e2 += dx) 
				pixels.push_back(Pixel(x0, y1 += sy));                      /* pixel on the line */
			pixels.push_back(Pixel(x0, y1+sy, e2));                    /* aliasing post-pixel */
			if (x0 == x1)
				break;
			err += dy;                                                 /* x-step */ 
			if (err > 255)
			{
				err -= dx;
				y0 += sy;
			}                    /* y-step */
		} 
	}
}

//=================================================================================================
void PlotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels)
{   /* plot a limited rational Bezier segment of thickness th, squared weight */
	int sx = x2-x1, sy = y2-y1;                  /* relative values for checks */
	int dx = x0-x2, dy = y0-y2, xx = x0-x1, yy = y0-y1;
	int xy = xx*sy+yy*sx, cur = xx*sy-yy*sx, err, e2, ed;         /* curvature */

	//assert(xx*sx <= 0.0f && yy*sy <= 0.0f);  /* sign of gradient must not change */

	if (cur != 0.0f && w > 0.0f)
	{                           /* no straight line */
		if (sx*sx+sy*sy > xx*xx+yy*yy)
		{              /* begin with longer part */
			x2 = x0;
			x0 -= dx;
			y2 = y0;
			y0 -= dy;
			cur = -cur;      /* swap P0 P2 */
		}
		xx = 2.0f*(4.0f*w*sx*xx+dx*dx);                 /* differences 2nd degree */
		yy = 2.0f*(4.0f*w*sy*yy+dy*dy);
		sx = x0 < x2 ? 1 : -1;                              /* x step direction */
		sy = y0 < y2 ? 1 : -1;                              /* y step direction */
		xy = -2.0f*sx*sy*(2.0f*w*xy+dx*dy);

		if (cur*sx*sy < 0)
		{                              /* negated curvature? */
			xx = -xx;
			yy = -yy;
			cur = -cur;
			xy = -xy;
		}
		dx = 4.0f*w*(x1-x0)*sy*cur+xx/2.0f;             /* differences 1st degree */
		dy = 4.0f*w*(y0-y1)*sx*cur+yy/2.0f;

		if (w < 0.5f && (dx+xx <= 0 || dy+yy >= 0))
		{/* flat ellipse, algo fails */
			cur = (w+1.0)/2.0f;
			w = sqrt(w);
			xy = 1.0f/(w+1.0f);
			sx = floor((x0+2.0f*w*x1+x2)*xy/2.0+0.5f);    /* subdivide curve  */
			sy = floor((y0+2.0f*w*y1+y2)*xy/2.0+0.5f);     /* plot separately */
			dx = floor((w*x1+x0)*xy+0.5f);
			dy = floor((y1*w+y0)*xy+0.5f);
			PlotQuadBezierSeg(x0,y0, dx,dy, sx,sy, cur, th, pixels);
			dx = floor((w*x1+x2)*xy+0.5f);
			dy = floor((y1*w+y2)*xy+0.5f);
			return PlotQuadBezierSeg(sx,sy, dx,dy, x2,y2, cur, th, pixels);
		}
		for (err = 0; dy+2*yy < 0 && dx+2*xx > 0; ) /* loop of steep/flat curve */
		{
			if (dx+dy+xy < 0)
			{                                   /* steep curve */
				do
				{
					ed = -dy-2*dy*dx*dx/(4.f*dy*dy+dx*dx);      /* approximate sqrt */
					w = (th-1)*ed;                             /* scale line width */
					x1 = floor((err-ed-w/2)/dy);              /* start offset */
					e2 = err-x1*dy-w/2;                   /* error value at offset */
					x1 = x0-x1*sx;                                  /* start point */
					pixels.push_back(Pixel(x1, y0, 255*e2/ed));           /* aliasing pre-pixel */
					for (e2 = -w-dy-e2; e2-dy < ed; e2 -= dy)
						pixels.push_back(Pixel(x1 += sx, y0));              /* pixel on thick line */
					pixels.push_back(Pixel(x1+sx, y0, 255*e2/ed));       /* aliasing post-pixel */
					if (y0 == y2)
						return;          /* last pixel -> curve finished */
					y0 += sy;
					dy += xy;
					err += dx;
					dx += xx;             /* y step */
					if (2*err+dy > 0)
					{                            /* e_x+e_xy > 0 */
						x0 += sx;
						dx += xy;
						err += dy;
						dy += yy;          /* x step */
					}
					if (x0 != x2 && (dx+2*xx <= 0 || dy+2*yy >= 0))
					{
						if (abs(y2-y0) > abs(x2-x0))
							goto fail;
						else
							break;   
					}/* other curve near */
				}
				while (dx+dy+xy < 0);                  /* gradient still steep? */
				/* change from steep to flat curve */
				for (cur = err-dy-w/2, y1 = y0; cur < ed; y1 += sy, cur += dx)
				{
					for (e2 = cur, x1 = x0; e2-dy < ed; e2 -= dy)
						pixels.push_back(Pixel(x1 -= sx, y1));              /* pixel on thick line */
					pixels.push_back(Pixel(x1-sx, y1, 255*e2/ed));       /* aliasing post-pixel */
				}
			}
			else
			{                                               /* flat curve */
				do
				{
					ed = dx+2*dx*dy*dy/(4.f*dx*dx+dy*dy);       /* approximate sqrt */
					w = (th-1)*ed;                             /* scale line width */
					y1 = floor((err+ed+w/2)/dx);              /* start offset */
					e2 = y1*dx-w/2-err;                   /* error value at offset */
					y1 = y0-y1*sy;                                  /* start point */
					pixels.push_back(Pixel(x0, y1, 255*e2/ed));           /* aliasing pre-pixel */
					for (e2 = dx-e2-w; e2+dx < ed; e2 += dx)
						pixels.push_back(Pixel(x0, y1 += sy));              /* pixel on thick line */
					pixels.push_back(Pixel(x0, y1+sy, 255*e2/ed));       /* aliasing post-pixel */
					if (x0 == x2)
						return;          /* last pixel -> curve finished */
					x0 += sx;
					dx += xy;
					err += dy;
					dy += yy;             /* x step */
					if (2*err+dx < 0)
					{                           /* e_y+e_xy < 0 */
						y0 += sy;
						dy += xy;
						err += dx;
						dx += xx;          /* y step */
					}
					if (y0 != y2 && (dx+2*xx <= 0 || dy+2*yy >= 0))
					{
						if (abs(y2-y0) <= abs(x2-x0))
							goto fail;  
						else
							break;                             /* other curve near */
					}
				}
				while (dx+dy+xy >= 0);                  /* gradient still flat? */
				/* change from flat to steep curve */ 
				for (cur = -err+dx-w/2, x1 = x0; cur < ed; x1 += sx, cur -= dy)
				{
					for (e2 = cur, y1 = y0; e2+dx < ed; e2 += dx)
						pixels.push_back(Pixel(x1, y1 -= sy));              /* pixel on thick line */
					pixels.push_back(Pixel(x1, y1-sy, 255*e2/ed));       /* aliasing post-pixel */
				}
			}
		}
	}
fail:
	PlotLine(x0,y0, x2,y2, th, pixels);            /* confusing error values  */
}

//=================================================================================================
void PlotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels)
{                    /* plot any anti-aliased quadratic rational Bezier curve */
	int x = x0-2*x1+x2, y = y0-2*y1+y2;
	float xx = x0-x1, yy = y0-y1, ww, t, q;

	assert(w >= 0.0f);

	if (xx*(x2-x1) > 0)
	{                             /* horizontal cut at P4? */
		if (yy*(y2-y1) > 0 && abs(xx*y) > abs(yy*x))/* vertical cut at P6 too? */
		{               /* which first? */
			x0 = x2;
			x2 = xx+x1;
			y0 = y2;
			y2 = yy+y1;          /* swap points */
		}                            /* now horizontal cut at P4 comes first */
		if (x0 == x2 || w == 1.0f)
			t = float(x0-x1)/x;
		else
		{                                 /* non-rational or rational case */
			q = sqrt(4.0f*w*w*(x0-x1)*(x2-x1)+(x2-x0)*(x2-x0));
			if (x1 < x0)
				q = -q;
			t = (2.0f*w*(x0-x1)-x0+x2+q)/(2.0f*(1.0f-w)*(x2-x0));        /* t at P4 */
		}
		q = 1.0f/(2.0f*t*(1.0-t)*(w-1.0f)+1.0f);                 /* sub-divide at t */
		xx = (t*t*(x0-2.0f*w*x1+x2)+2.0f*t*(w*x1-x0)+x0)*q;               /* = P4 */
		yy = (t*t*(y0-2.0f*w*y1+y2)+2.0f*t*(w*y1-y0)+y0)*q;
		ww = t*(w-1.0)+1.0;
		ww *= ww*q;                    /* squared weight P3 */
		w = ((1.0f-t)*(w-1.0f)+1.0f)*sqrt(q);                    /* weight P8 */
		x = floor(xx+0.5f); y = floor(yy+0.5f);                   /* P4 */
		yy = (xx-x0)*(y1-y0)/(x1-x0)+y0;                /* intersect P3 | P0 P1 */
		PlotQuadBezierSeg(x0,y0, x,floor(yy+0.5f), x,y, ww, th, pixels);
		yy = (xx-x2)*(y1-y2)/(x1-x2)+y2;                /* intersect P4 | P1 P2 */
		y1 = floor(yy+0.5f);
		x0 = x1 = x;
		y0 = y;       /* P0 = P4, P1 = P8 */
	}
	if ((y0-y1)*(y2-y1) > 0)
	{                          /* vertical cut at P6? */
		if (y0 == y2 || w == 1.0f)
			t = float(y0-y1)/(y0-2.0f*y1+y2);
		else
		{                                 /* non-rational or rational case */
			q = sqrt(4.0f*w*w*(y0-y1)*(y2-y1)+(y2-y0)*(y2-y0));
			if (y1 < y0)
				q = -q;
			t = (2.0f*w*(y0-y1)-y0+y2+q)/(2.0f*(1.0f-w)*(y2-y0));        /* t at P6 */
		}
		q = 1.0f/(2.0f*t*(1.0f-t)*(w-1.0f)+1.0f);                 /* sub-divide at t */
		xx = (t*t*(x0-2.0f*w*x1+x2)+2.0f*t*(w*x1-x0)+x0)*q;               /* = P6 */
		yy = (t*t*(y0-2.0f*w*y1+y2)+2.0f*t*(w*y1-y0)+y0)*q;
		ww = t*(w-1.0f)+1.0f;
		ww *= ww*q;                    /* squared weight P5 */
		w = ((1.0f-t)*(w-1.0f)+1.0f)*sqrt(q);                    /* weight P7 */
		x = floor(xx+0.5f);
		y = floor(yy+0.5f);                   /* P6 */
		xx = (x1-x0)*(yy-y0)/(y1-y0)+x0;                /* intersect P6 | P0 P1 */
		PlotQuadBezierSeg(x0,y0, floor(xx+0.5f),y, x,y, ww, th, pixels);
		xx = (x1-x2)*(yy-y2)/(y1-y2)+x2;                /* intersect P7 | P1 P2 */
		x1 = floor(xx+0.5f);
		x0 = x; y0 = y1 = y;       /* P0 = P6, P1 = P7 */
	}
	PlotQuadBezierSeg(x0,y0, x1,y1, x2,y2, w*w, th, pixels);  
}

//=================================================================================================
void PlotCubicBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels)
{                     /* split cubic Bezier segment in two quadratic segments */
	int x = floor(float(x0+3*x1+3*x2+x3+4)/8), 
		y = floor(float(y0+3*y1+3*y2+y3+4)/8);
	PlotQuadBezierSeg(x0,y0,floor(float(x0+3*x1+2)/4),floor(float(y0+3*y1+2)/4), x,y, 1,th, pixels);
	PlotQuadBezierSeg(x,y,floor(float(3*x2+x3+2)/4),floor(float(3*y2+y3+2)/4), x3,y3, 1,th, pixels);
} 

//=================================================================================================
void PlotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels)
{                                              /* plot any cubic Bezier curve */
	int n = 0;
	__int64 i = 0;
	int xc = x0+x1-x2-x3, xa = xc-4*(x1-x2);
	int xb = x0-x1-x2+x3, xd = xb+4*(x1+x2);
	int yc = y0+y1-y2-y3, ya = yc-4*(y1-y2);
	int yb = y0-y1-y2+y3, yd = yb+4*(y1+y2);
	int fx0 = x0, fx1, fx2, fx3, fy0 = y0, fy1, fy2, fy3;
	float t1 = xb*xb-xa*xc, t2, t[7];
	/* sub-divide curve at gradient sign changes */
	if (xa == 0)
	{                                               /* horizontal */
		if (abs(xc) < 2*abs(xb))
			t[n++] = xc/(2.0f*xb);  /* one change */
	}
	else if (t1 > 0.0f)
	{                                      /* two changes */
		t2 = sqrt(t1);
		t1 = (xb-t2)/xa;
		if (abs(t1) < 1.0f)
			t[n++] = t1;
		t1 = (xb+t2)/xa;
		if (abs(t1) < 1.0f)
			t[n++] = t1;
	}
	t1 = yb*yb-ya*yc;
	if (ya == 0)
	{                                                 /* vertical */
		if (abs(yc) < 2*abs(yb))
			t[n++] = yc/(2.0f*yb);  /* one change */
	}
	else if (t1 > 0.0f)
	{                                      /* two changes */
		t2 = sqrt(t1);
		t1 = (yb-t2)/ya;
		if (abs(t1) < 1.0f)
			t[n++] = t1;
		t1 = (yb+t2)/ya;
		if (abs(t1) < 1.0f)
			t[n++] = t1;
	}
	t1 = 2*(xa*yb-xb*ya);
	t2 = xa*yc-xc*ya;      /* divide at inflection point */
	i = t2*t2-2*t1*(xb*yc-xc*yb);
	if (i > 0)
	{
		i = sqrt(float(i));
		t[n] = (t2+i)/t1;
		if (abs(t[n]) < 1.0f)
			n++;
		t[n] = (t2-i)/t1;
		if (abs(t[n]) < 1.0f)
			n++;
	}
	for (i = 1; i < n; i++)                         /* bubble sort of 4 points */
	{
		if ((t1 = t[i-1]) > t[i])
		{
			t[i-1] = t[i];
			t[i] = t1;
			i = 0;
		}
	}
	t1 = -1.0f;
	t[n] = 1.0f;                               /* begin / end points */
	for (i = 0; i <= n; i++)
	{                 /* plot each segment separately */
		t2 = t[i];                                /* sub-divide at t[i-1], t[i] */
		fx1 = (t1*(t1*xb-2*xc)-t2*(t1*(t1*xa-2*xb)+xc)+xd)/8-fx0;
		fy1 = (t1*(t1*yb-2*yc)-t2*(t1*(t1*ya-2*yb)+yc)+yd)/8-fy0;
		fx2 = (t2*(t2*xb-2*xc)-t1*(t2*(t2*xa-2*xb)+xc)+xd)/8-fx0;
		fy2 = (t2*(t2*yb-2*yc)-t1*(t2*(t2*ya-2*yb)+yc)+yd)/8-fy0;
		fx0 -= fx3 = (t2*(t2*(3*xb-t2*xa)-3*xc)+xd)/8;
		fy0 -= fy3 = (t2*(t2*(3*yb-t2*ya)-3*yc)+yd)/8;
		x3 = floor(fx3+0.5f);
		y3 = floor(fy3+0.5f);     /* scale bounds */
		if (fx0 != 0.0f)
		{
			fx1 *= fx0 = (x0-x3)/fx0;
			fx2 *= fx0;
		}
		if (fy0 != 0.0f)
		{
			fy1 *= fy0 = (y0-y3)/fy0;
			fy2 *= fy0;
		}
		if (x0 != x3 || y0 != y3)                            /* segment t1 - t2 */
			PlotCubicBezierSeg(x0,y0, x0+fx1,y0+fy1, x0+fx2,y0+fy2, x3,y3, th, pixels);
		x0 = x3;
		y0 = y3;
		fx0 = fx3;
		fy0 = fy3;
		t1 = t2;
	}
} 
