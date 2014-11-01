#ifndef COSMO_HYDRO_DATA
#define COSMO_HYDRO_DATA

namespace cosmo
{

typedef struct {

  /* lorentz factor */
  real_t W;

  /* root of metric determinant */
  real_t rg;
  /* metric det. */
  real_t g;
  /* 'scale factor' */
  real_t a;

} HydroData;

} /* namespace cosmo */

#endif
