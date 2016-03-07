#ifndef COSMO_ICS
#define COSMO_ICS

#include "cosmo.h"
#include "globals.h"

#include "ICs_data.h"
#include "io_data.h"

namespace cosmo
{

ICsData cosmo_get_ICsData();

void init_ray_vector(
  std::vector<RayTrace<real_t, idx_t> *> * rays, idx_t n_rays);

void set_ICs(
  std::map <std::string, real_t *> & bssn_fields,
  std::map <std::string, real_t *> & static_fields,
  Fourier *fourier, IOData *iod, FRW<real_t> *frw);

real_t cosmo_power_spectrum(real_t k, ICsData *icd);
void set_gaussian_random_field(real_t *field, Fourier *fourier, ICsData *icd);
void set_conformal_ICs(
  std::map <std::string, real_t *> & bssn_fields,
  std::map <std::string, real_t *> & hydro_fields,
  Fourier *fourier, IOData *iod, FRW<real_t> *frw);

void set_stability_test_ICs(
  std::map <std::string, real_t *> & bssn_fields,
  std::map <std::string, real_t *> & static_fields);

void set_linear_wave_ICs(
  std::map <std::string, real_t *> & bssn_fields);

} // namespace cosmo

#endif
