#pragma once
#include "core/base.h"
#if !defined(l_float)
#define l_float double
#endif

L_INLINE l_float
l_metric_inch(l_float in) {
  return in * 0.0254; /* the length of a part of thumb 2.54cm */
}

L_INLINE l_float
l_metric_foot(l_float ft) {
  return ft * 0.3048; /* 1 foot = 12 inches 30.48cm */
}

L_INLINE l_float
l_metric_mile(l_float mi) {
  return mi * 1609.344; /* 1 mile = 5280 foot 1609.344m */
}

L_INLINE l_float
l_metric_astrounit(l_float au) {
  return au * 149597870700; /* the average distance from sun to earth ~ 1.5yikm */
}

L_INLINE l_int
l_metric_astrounitI(l_int au) {
  return au * 149597870700;
}

L_INLINE l_float
l_metric_lightspeed() {
  return 299792458; /* ~ 30wkm/s */
}

L_INLINE l_int
l_metric_lightspeedI() {
  return 299792458;
}

L_INLINE l_float
l_metric_lightyear() {
  return 9460730472580800;
}

L_INLINE l_int
l_metric_lightyearI() {
  return 9460730472580800ll;
}

