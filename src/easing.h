#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923	/* pi/2 */
#endif

/*
  Easing functions
  by Kristoffer Gronlund, 2014
  Public domain

  This work is a spiritual descendent (not to say derivative work) of works done by the following individuals:

  Warren Moore (https://github.com/warrenm)
  Robert Penner (http://www.robertpenner.com/easing/)
  George McGinley Smith (http://gsgd.co.uk/sandbox/jquery/easing/)
  James Padolsey (http://james.padolsey.com/demos/jquery/easing/)
  Authors of jQuery (http://plugins.jquery.com/project/Easing)
  Matt Gallagher (http://cocoawithlove.com/2008/09/parametric-acceleration-curves-in-core.html)
  Jesse Crossen (http://stackoverflow.com/questions/5161465/how-to-create-custom-easing-function-with-core-animation)
*/


typedef float easing_t;


static inline
easing_t enEase(easing_t a, easing_t b, easing_t t)
{
	return a + (b - a) * t;
}


static inline
easing_t enLinear(easing_t t)
{
	return t;
}


static inline
easing_t enQuadraticIn(easing_t t)
{
	return t * t;
}


static inline
easing_t enQuadraticOut(easing_t t)
{
	return -(t * (t - 2.));
}


static inline
easing_t enQuadraticInOut(easing_t t)
{
	return (t < 0.5) ? 2. * t * t : (-2. * t * t) + (4. * t) - 1.;
}


static inline
easing_t enCubicIn(easing_t t)
{
	return t * t * t;
}


static inline
easing_t enCubicOut(easing_t t)
{
	const easing_t f = t - 1.; return f * f * f + 1.;
}


static inline
easing_t enCubicInOut(easing_t t)
{
	if (t < (easing_t)0.5) {
		return 4. * t * t * t;
	} else {
		const easing_t f = (2. * t) - 2.;
		return 0.5 * f * f * f + 1.;
	}
}


static inline
easing_t enQuarticIn(easing_t t) {
	return t * t * t * t;
}


static inline
easing_t enQuarticOut(easing_t t) {
	const easing_t f = t - 1.;
	return f * f * f * (1. - t) + 1.;
}


static inline
easing_t enQuarticInOut(easing_t t)
{
	if(t < 0.5) {
		return 8. * t * t * t * t;
	} else {
		easing_t f = (t - 1.);
		return -8. * f * f * f * f + 1.;
	}
}


static inline
easing_t enQuinticIn(easing_t t) {
	return t * t * t * t * t;
}


static inline
easing_t enQuinticOut(easing_t t) {
	easing_t f = (t - 1.);
	return f * f * f * f * f + 1.;
}


static inline
easing_t enQuinticInOut(easing_t t)
{
	if (t < 0.5) {
		return 16. * t * t * t * t * t;
	} else {
		easing_t f = ((2. * t) - 2.);
		return  0.5 * f * f * f * f * f + 1.;
	}
}


static inline
easing_t enSineIn(easing_t t)
{
	return sin((t - 1.) * M_PI_2) + 1.;
}


static inline
easing_t enSineOut(easing_t t)
{
	return sin(t * M_PI_2);
}


static inline
easing_t enSineInOut(easing_t t)
{
	return 0.5 * (1. - cos(t * M_PI));
}


static inline
easing_t enCircularIn(easing_t t)
{
	return 1. - sqrt(1. - (t * t));
}


static inline
easing_t enCircularOut(easing_t t)
{
	return sqrt((2. - t) * t);
}


static inline
easing_t enCircularInOut(easing_t t)
{
	if (t < 0.5) {
		return 0.5 * (1 - sqrt(1 - 4. * (t * t)));
	} else {
		return 0.5 * (sqrt(-((2. * t) - 3.) * ((2. * t) - 1.)) + 1.);
	}
}


static inline
easing_t enExponentialIn(easing_t t)
{
	return (t <= 0) ? t : pow(2., 10. * (t - 1.));
}


static inline
easing_t enExponentialOut(easing_t t)
{
	return (t >= 1.) ? t : 1. - pow(2., -10. * t);
}


static inline
easing_t enExponentialInOut(easing_t t)
{
	if (t <= 0. || t >= 1.)
		return t;

	if (t < 0.5)
{
		return 0.5 * pow(2., (20. * t) - 10.);
	} else {
		return -0.5 * pow(2., (-20. * t) + 10.) + 1.;
	}
}


static inline
easing_t enElasticIn(easing_t t)
{
	return sin(13. * M_PI_2 * t) * pow(2., 10. * (t - 1.));
}


static inline
easing_t enElasticOut(easing_t t)
{
	return sin(-13. * M_PI_2 * (t + 1.)) * pow(2., -10. * t) + 1.;
}


static inline
easing_t enElasticInOut(easing_t t)
{
	if (t < 0.5) {
		return 0.5 * sin(13. * M_PI_2 * (2. * t)) * pow(2., 10. * ((2. * t) - 1.));
	} else {
		return 0.5 * (sin(-13. * M_PI_2 * ((2. * t - 1) + 1)) * pow(2., -10. * (2. * t - 1.)) + 2.);
	}
}


static inline
easing_t enBackIn(easing_t t)
{
	return t * t * t - t * sin(t * M_PI);
}


static inline
easing_t enBackOut(easing_t t)
{
	const easing_t f = 1. - t;
	return 1. - (f * f * f - f * sin(f * M_PI));
}


static inline
easing_t enBackInOut(easing_t t)
{
	if (t < 0.5) {
		const easing_t f = 2. * t;
		return 0.5 * (f * f * f - f * sin(f * M_PI));
	} else {
		const easing_t f = (1. - (2.*t - 1.));
		return 0.5 * (1. - (f * f * f - f * sin(f * M_PI))) + 0.5;
	}
}


static inline
easing_t enBounceOut(easing_t t)
{
	if (t < 4. / 11.) {
		return (121. * t * t) / 16.;
	} else if (t < 8. / 11.) {
		return (363. / 40. * t * t) - (99 / 10. * t) + 17 / 5.;
	} else if (t < 9. / 10.) {
		return (4356. / 361. * t * t) - (35442. / 1805. * t) + 16061. / 1805.;
	} else {
		return (54. / 5. * t * t) - (513. / 25. * t) + 268. / 25.;
	}
}


static inline
easing_t enBounceIn(easing_t t)
{
	return 1. - enBounceOut(1. - t);
}


static inline
easing_t enBounceInOut(easing_t t)
{
	if (t < 0.5) {
		return 0.5 * enBounceIn(t * 2.);
	} else {
		return 0.5 * enBounceOut(t * 2. - 1.) + 0.5;
	}
}


static inline
easing_t enPerlinInOut(easing_t t)
{
	easing_t t3 = t * t * t;
	easing_t t4 = t3 * t;
	easing_t t5 = t4 * t;
	return 6. * t5 - 15. * t4 + 10. * t3;
}
