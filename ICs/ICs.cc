#include "ICs.h"
#include  <fstream>

namespace cosmo
{

/**
 * @brief A cosmologically-motivated power spectrum for the conformal factor
 * @details Return the power spectrum amplitude for a particular wavenumber.
 *  The power spectrum describing the density goes roughly as 
 *    P(k) ~ k^{ 1} for small k
 *    P(k) ~ k^{-3} for large k
 *  (see, eg, http://ned.ipac.caltech.edu/level5/Sept11/Norman/Norman2.html)
 *  An analytic function with this form is the function
 *    P(k) ~ k / ( 1 + k^4 )
 *  The conformal factor obeys k^2 \phi ~ \rho; so \phi ~ \rho / k^2.
 *  The power spectrum < \phi \phi > then goes as 1/k^4 < \rho \rho >.
 *  Thus, the power spectrum for the conformal factor scales as
 *    P(k) ~ 1 / k^3 / ( 1 + k^4 )
 *  Which after additional scalings, this function returns.
 * 
 * @param k wavenumber of interest
 * 
 * @return power spectrum amplitude
 */
real_t cosmo_power_spectrum(real_t k, real_t A, real_t k0)
{
  return A/(1.0 + pow(fabs(k)/k0, 4.0)/3.0)/pow(fabs(k)/k0, 3.0);
}

void set_gaussian_random_Phi_N(arr_t & field, Fourier *fourier,
  real_t A, real_t p0, real_t p_cut)
{
  set_gaussian_random_Phi_N(field, fourier, A, p0, p_cut, false);
}

// set a field to an arbitrary gaussian random field
void set_gaussian_random_Phi_N(arr_t & field, Fourier *fourier,
  real_t A, real_t p0, real_t p_cut, bool fix_amplitude)
{
  idx_t i, j, k;
  real_t px, py, pz, p_mag;
  real_t scale;
  real_t signA = A<0.0?-1:1;
  real_t absA = A<0.0?-A:A;

  // populate "field" with random values
  std::random_device rd;
  const real_t seed = stod(_config("mt19937_seed", "9"));
  std::mt19937 gen(seed);
  std::normal_distribution<real_t> gaussian_distribution; // zero mean, unit variance
  std::uniform_real_distribution<double> angular_distribution(0.0, 2.0*PI);
   // calling these here before looping suppresses a warning (bug)
  gaussian_distribution(gen);
  angular_distribution(gen);

  // scale amplitudes in fourier space
  // don't expect to run at >512^3 anytime soon; loop over all momenta out to that resolution.
  // this won't work for a larger grid.
  idx_t NMAX = 512;
  for(i=0; i<NMAX; i++)
  {
    px = (real_t) (i<=NMAX/2 ? i : i-NMAX);
    for(j=0; j<NMAX; j++)
    {
      py = (real_t) (j<=NMAX/2 ? j : j-NMAX);
      for(k=0; k<NMAX/2+1; k++)
      {
        pz = (real_t) k;

        // generate the same random modes for all resolutions (up to NMAX)
        real_t rand_mag = gaussian_distribution(gen);
        // fix power spectrum amplitude
        if(fix_amplitude)
        {
          rand_mag = 1.0;
        }
        real_t rand_phase = angular_distribution(gen);

        // only store momentum values for relevant bins
        if( fabs(px) < (real_t) NX/2+1 + 0.01 && fabs(py) < (real_t) NY/2+1 + 0.01 && fabs(pz) < (real_t) NZ/2+1 + 0.01 )
        {
          idx_t fft_index = FFT_NP_INDEX(
            px > -0.5 ? ROUND_2_IDXT(px) : NX + ROUND_2_IDXT(px),
            py > -0.5 ? ROUND_2_IDXT(py) : NY + ROUND_2_IDXT(py),
            pz > -0.5 ? ROUND_2_IDXT(pz) : NZ + ROUND_2_IDXT(pz)
          );
          real_t max_fft_index = NX*NY*(NZ/2+1) - 1;
          if(fft_index > max_fft_index)
          {
            std::cerr << "Warning: index " << fft_index << " is greater than max ("
              << max_fft_index << ")." << std::endl;
            fft_index = max_fft_index;
          }

          p_mag = sqrt( pw2(px) + pw2(py) + pw2(pz) );

          // Scale by power spectrum
          // don't want much power on scales smaller than ~3 pixels
          // Or scales p > 1/(3*dx)
          real_t cutoff = 1.0 / (
              1.0 + exp(10.0*(p_mag - p_cut))
          );
          scale = signA*cutoff*std::sqrt(cosmo_power_spectrum(p_mag, absA, p0));

          (fourier->f_field)[fft_index][0] = scale*rand_mag*cos(rand_phase);
          (fourier->f_field)[fft_index][1] = scale*rand_mag*sin(rand_phase);
        }
      }
    }
  }

  // zero-mode (mean density)... set this to something later
  (fourier->f_field)[FFT_NP_INDEX(0,0,0)][0] = 0;
  (fourier->f_field)[FFT_NP_INDEX(0,0,0)][1] = 0;

  // FFT back; 'field' array should now be populated with a gaussian random
  // field and power spectrum given by cosmo_power_spectrum.
#if USE_LONG_DOUBLES
  long double * double_field = new long double[POINTS];
  fftwl_execute_dft_c2r(fourier->p_c2r, fourier->f_field, double_field);
  for(idx_t i=0; i<POINTS; ++i)
    field._array[i] = (real_t) double_field[i];
  delete [] double_field;
#else
  double * double_field = new double[POINTS];
  fftw_execute_dft_c2r(fourier->p_c2r, fourier->f_field, double_field);
  for(idx_t i=0; i<POINTS; ++i)
    field._array[i] = (real_t) double_field[i];
  delete [] double_field;
#endif

  return;
}

#if USE_COSMOTRACE
/**
 * @brief Initialize vector of rays
 * @details Initialize a vector of rays with given rays, given velocity
 *  directions (ray velocity magnitude is normalized by the raytrace class)
 * 
 * @param rays reference to RayTrace class rays should belong to
 */
void init_ray_vector(std::vector<RayTrace<real_t, idx_t> *> * rays)
{
  std::string ray_ic_type = _config("ray_ic_type", "");
  if(ray_ic_type == "healpix")
  {
    init_healpix_ray_vectors(rays);
    std::cout << "Setting healpix vectors...\n";
  }
  else
  {
    init_random_ray_vectors(rays);
  }
}

void init_healpix_ray_vectors(std::vector<RayTrace<real_t, idx_t> *> * rays)
{
  RaytraceData<real_t> rd = {0};

  // ray position starts at an observer centered in the box
  rd.x[0] = (NX - 0.5)*dx/2.0;
  rd.x[1] = (NY - 0.5)*dx/2.0;
  rd.x[2] = (NZ - 0.5)*dx/2.0;

  // energy, angle in arb. units
  rd.E = 1.0;
  rd.Phi = -1.0;

  // read in healpix vectors
  // generated by performing, eg:
  // numpy.savetxt("nside_<NSIDE>.vecs", [hp.pix2vec(<NSIDE>, p) for p in range(hp.nside2npix(<NSIDE>))])
  std::ifstream vecFile (_config["healpix_vecs_file"]);

  while (!vecFile.eof())
  {
    vecFile >> rd.V[0];
    vecFile >> rd.V[1];
    vecFile >> rd.V[2];

    RayTrace<real_t, idx_t> * ray;
    ray = new RayTrace<real_t, idx_t> (-std::fabs(dt), dx, rd);
    rays->push_back( ray );
  }
}

void init_random_ray_vectors(std::vector<RayTrace<real_t, idx_t> *> * rays)
{
  idx_t i = 0;
  idx_t n_rays = 3000;
  RaytraceData<real_t> rd = {0};

  std::mt19937 gen(7.0);
  std::uniform_real_distribution<real_t> dist(0.0, 1.0);
  dist(gen);
  for(i=0; i<n_rays; i++)
  {
    // rays distributed (uniformly) around a sphere
    real_t theta = 2.0*PI*dist(gen);
    real_t U = 2.0*dist(gen) - 1.0;
    // inside some over/underdensity (hopefully)
    real_t X0, Y0, Z0;
    if(i<n_rays/3)
    {
      X0 = 0.382813*COSMO_N*dx;
      Y0 = 0.476563*COSMO_N*dx;
      Z0 = 0.351563*COSMO_N*dx;
    }
    else if(i<2*n_rays/3)
    {
      X0 = 0.460938*COSMO_N*dx;
      Y0 = 0.554688*COSMO_N*dx;
      Z0 = 0.492188*COSMO_N*dx;
    }
    else
    {
      X0 = 0.0429688*COSMO_N*dx;
      Y0 = 0.0360625*COSMO_N*dx;
      Z0 = 0.0390625*COSMO_N*dx;
    }
    // ray position starts at an observer (integrating back in time...)
    rd.x[0] = X0;
    rd.x[1] = Y0;
    rd.x[2] = Z0;
    // velocity directed inwards
    // normalization of V is enforced by raytrace class
    rd.V[0] = 1.0*std::sqrt(1.0-U*U)*std::cos(theta);
    rd.V[1] = 1.0*std::sqrt(1.0-U*U)*std::sin(theta);
    rd.V[2] = 1.0*U;

    // energy, angle in arb. units
    rd.E = 1.0;
    rd.Phi = -1.0;

    RayTrace<real_t, idx_t> * ray;
    ray = new RayTrace<real_t, idx_t> (-std::fabs(dt), dx, rd);
    rays->push_back( ray );
  }
}
#endif

} // namespace cosmo
