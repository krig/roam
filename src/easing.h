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

#define EN_Scalar float

static inline EN_Scalar
enEase(EN_Scalar a, EN_Scalar b, EN_Scalar t) {
	return a + (b - a) * t;
}

static inline EN_Scalar
enLinear(EN_Scalar t) {
	return t;
}

static inline EN_Scalar
enQuadraticIn(EN_Scalar t) {
	return t * t;
}

static inline EN_Scalar
enQuadraticOut(EN_Scalar t) {
	return -(t * (t - 2.));
}

static inline EN_Scalar
enQuadraticInOut(EN_Scalar t) {
	return (t < 0.5) ? 2. * t * t : (-2. * t * t) + (4. * t) - 1.;
}

static inline EN_Scalar
enCubicIn(EN_Scalar t) {
	return t * t * t;
}

static inline EN_Scalar
enCubicOut(EN_Scalar t) {
	const EN_Scalar f = t - 1.; return f * f * f + 1.;
}

static inline EN_Scalar
enCubicInOut(EN_Scalar t) {
	if (t < EN_Scalar(0.5)) {
		return 4. * t * t * t;
	} else {
		const EN_Scalar f = (2. * t) - 2.;
		return 0.5 * f * f * f + 1.;
	}
}

static inline EN_Scalar
enQuarticIn(EN_Scalar t) {
	return t * t * t * t;
}

static inline EN_Scalar
enQuarticOut(EN_Scalar t) {
	const EN_Scalar f = t - 1.;
	return f * f * f * (1. - t) + 1.;
}

static inline EN_Scalar
enQuarticInOut(EN_Scalar t) {
	if(t < 0.5) {
		return 8. * t * t * t * t;
	} else {
		EN_Scalar f = (t - 1.);
		return -8. * f * f * f * f + 1.;
	}
}

static inline EN_Scalar
enQuinticIn(EN_Scalar t) {
	return t * t * t * t * t;
}

static inline EN_Scalar
enQuinticOut(EN_Scalar t) {
	EN_Scalar f = (t - 1.);
	return f * f * f * f * f + 1.;
}

static inline EN_Scalar
enQuinticInOut(EN_Scalar t) {
	if (t < 0.5) {
		return 16. * t * t * t * t * t;
	} else {
		EN_Scalar f = ((2. * t) - 2.);
		return  0.5 * f * f * f * f * f + 1.;
	}
}

static inline EN_Scalar
enSineIn(EN_Scalar t) {
	return sin((t - 1.) * M_PI_2) + 1.;
}

static inline EN_Scalar
enSineOut(EN_Scalar t) {
	return sin(t * M_PI_2);
}

static inline EN_Scalar
enSineInOut(EN_Scalar t) {
	return 0.5 * (1. - cos(t * M_PI));
}

static inline EN_Scalar
enCircularIn(EN_Scalar t) {
	return 1. - sqrt(1. - (t * t));
}

static inline EN_Scalar
enCircularOut(EN_Scalar t) {
	return sqrt((2. - t) * t);
}

static inline EN_Scalar
enCircularInOut(EN_Scalar t) {
	if (t < 0.5) {
		return 0.5 * (1 - sqrt(1 - 4. * (t * t)));
	} else {
		return 0.5 * (sqrt(-((2. * t) - 3.) * ((2. * t) - 1.)) + 1.);
	}
}

static inline EN_Scalar
enExponentialIn(EN_Scalar t) {
	return (t <= 0) ? t : pow(2., 10. * (t - 1.));
}

static inline EN_Scalar
enExponentialOut(EN_Scalar t) {
	return (t >= 1.) ? t : 1. - pow(2., -10. * t);
}

static inline EN_Scalar
enExponentialInOut(EN_Scalar t) {
	if (t <= 0. || t >= 1.)
		return t;

	if (t < 0.5) {
		return 0.5 * pow(2., (20. * t) - 10.);
	} else {
		return -0.5 * pow(2., (-20. * t) + 10.) + 1.;
	}
}

static inline EN_Scalar
enElasticIn(EN_Scalar t) {
	return sin(13. * M_PI_2 * t) * pow(2., 10. * (t - 1.));
}

static inline EN_Scalar
enElasticOut(EN_Scalar t) {
	return sin(-13. * M_PI_2 * (t + 1.)) * pow(2., -10. * t) + 1.;
}

static inline EN_Scalar
enElasticInOut(EN_Scalar t) {
	if (t < 0.5) {
		return 0.5 * sin(13. * M_PI_2 * (2. * t)) * pow(2., 10. * ((2. * t) - 1.));
	} else {
		return 0.5 * (sin(-13. * M_PI_2 * ((2. * t - 1) + 1)) * pow(2., -10. * (2. * t - 1.)) + 2.);
	}
}

static inline EN_Scalar
enBackIn(EN_Scalar t) {
	return t * t * t - t * sin(t * M_PI);
}

static inline EN_Scalar
enBackOut(EN_Scalar t) {
	const EN_Scalar f = 1. - t;
	return 1. - (f * f * f - f * sin(f * M_PI));
}

static inline EN_Scalar
enBackInOut(EN_Scalar t) {
	if (t < 0.5) {
		const EN_Scalar f = 2. * t;
		return 0.5 * (f * f * f - f * sin(f * M_PI));
	} else {
		const EN_Scalar f = (1. - (2.*t - 1.));
		return 0.5 * (1. - (f * f * f - f * sin(f * M_PI))) + 0.5;
	}
}

static inline EN_Scalar
enBounceOut(EN_Scalar t) {
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

static inline EN_Scalar
enBounceIn(EN_Scalar t) {
	return 1. - enBounceOut(1. - t);
}

static inline EN_Scalar
enBounceInOut(EN_Scalar t) {
	if (t < 0.5) {
		return 0.5 * enBounceIn(t * 2.);
	} else {
		return 0.5 * enBounceOut(t * 2. - 1.) + 0.5;
	}
}

static inline EN_Scalar
enPerlinInOut(EN_Scalar t) {
	EN_Scalar t3 = t * t * t;
	EN_Scalar t4 = t3 * t;
	EN_Scalar t5 = t4 * t;
	return 6. * t5 - 15. * t4 + 10. * t3;
}
