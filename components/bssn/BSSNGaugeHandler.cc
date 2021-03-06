#include "BSSNGaugeHandler.h"
#include "bssn.h"
#include "../../cosmo_types.h"
#include "../../cosmo_globals.h"
#include <map>
#include <cmath>
#include "../../utils/math.h"

namespace cosmo
{

/**
 * @brief Don't evolve anything
 * @return 0
 */
real_t BSSNGaugeHandler::Static(BSSNData *bd)
{
  return 0.0;
}

/**
 * @brief Locally conformal FLRW-type lapse
 */
real_t BSSNGaugeHandler::ConformalFLRWLapse(BSSNData *bd)
{
  return -1.0/3.0*bd->alpha*bd->K_avg;
}

/**
 * @brief Hamonic gauge lapse
 */
real_t BSSNGaugeHandler::HarmonicLapse(BSSNData *bd)
{
  return -1.0*bd->alpha*( bd->K );
}

/**
 * @brief Generalized Newton, see notes
 */
#if USE_GENERALIZED_NEWTON
real_t BSSNGaugeHandler::GeneralizedNewton(BSSNData *bd)
{  
  return GN_eta * ( 2.0/3.0 * bd->GND2Alpha - bd->GNDiDjRijTFoD2 );
}
#endif
  

/**
 * @brief Experimental gauge choice, quasi-newtonian
 */
real_t BSSNGaugeHandler::AnharmonicLapse(BSSNData *bd)
{
  //<<< TODO: generalize K "offset" in harmonic gauge.
  // Ref. showing presence of offset:
  // http://relativity.livingreviews.org/open?pubNo=lrr-2012-9&amp;page=articlesu7.html
  // for FRW (+ perturbation) sims, having no offset leads to lapse blowing up?
  return 1.0*pw2(bd->alpha)*( bd->K - bd->K_avg );
}


/**
 * @brief 1 + log slicing
 */
real_t BSSNGaugeHandler::OnePlusLogLapse(BSSNData *bd)
{
  return -2.0*bd->alpha*( bd->K - bd->K_avg )*gd_c
         + bd->beta1*bd->d1a + bd->beta2*bd->d2a + bd->beta3*bd->d3a;
}

#if USE_MAXIMAL_SLICING
real_t BSSNGaugeHandler::MaximalSlicingLapse(BSSNData *bd)
{
  return m_alpha
}    
#endif
  

/**
 * @brief Untested/experimental gauge choice; conformal synchronous gauge
 * Relies on having reference metric for K_FRW
 * See also ConformalFLRWLapse
 */
real_t BSSNGaugeHandler::ConformalSyncLapse(BSSNData *bd)
{
  return -1.0/3.0*bd->alpha*bd->K_FRW;
}


/**
 * @brief Trial Lapse function in cosmology
 * Assumes reference metric is not used / minkowski
 */
real_t BSSNGaugeHandler::TestKDriverLapse(BSSNData *bd)
{
  real_t dta = (
    bssn->ev_DIFFK(bd) - ( pw2(bd->K_avg)/3.0 + 4.0*PI*bd->rho_avg )
  ) + (
    bd->K - bd->K_avg
  );

  return -1.0*k_driver_coeff*dta;
}

/**
 * @brief Trial Lapse function in cosmology
 */
real_t BSSNGaugeHandler::TestAijDriverLapse(BSSNData *bd)
{
  return 0.0;
}



/**
 * @brief Gamma driver shift in x-dir
 */
real_t BSSNGaugeHandler::GammaDriverShift1(BSSNData *bd)
{
# if USE_GAMMA_DRIVER
  return bd->auxB1;
# endif
  return 0;
}

/**
 * @brief Gamma driver shift in y-dir
 */
real_t BSSNGaugeHandler::GammaDriverShift2(BSSNData *bd)
{
# if USE_GAMMA_DRIVER
  return bd->auxB2;
# endif
  return 0;
}

/**
 * @brief Gamma driver shift in z-dir
 */
real_t BSSNGaugeHandler::GammaDriverShift3(BSSNData *bd)
{
# if USE_GAMMA_DRIVER
  return bd->auxB3;
# endif
  return 0;
}

/**
 * @brief Damped wave gauge lapse
 */
real_t BSSNGaugeHandler::DampedWaveLapse(BSSNData *bd)
{
  return pw2(bd->alpha) * (dw_mu_l * (12.0 * bd->phi * dw_p - std::log(bd->alpha)) - bd->K)
    + bd->beta1 * bd->d1a + bd->beta2 * bd->d2a + bd->beta3 * bd->d3a;
}

/**
 * @brief Damped wave gauge shift in x-dir
 */
real_t BSSNGaugeHandler::DampedWaveShift1(BSSNData *bd)
{
  return bd->beta1*bd->d1beta1 + bd->beta2*bd->d2beta1 + bd->beta3*bd->d3beta1
    - dw_mu_s*bd->alpha*bd->beta1
    + bd->alpha * ( -dw_mu_l*(12.0*bd->phi*dw_p - std::log(bd->alpha))*bd->beta1
        + std::exp(-4.0*bd->phi)*(
          - bd->gammai11*bd->d1a - bd->gammai12*bd->d2a - bd->gammai13*bd->d3a
          + bd->alpha*(bd->Gamma1
              -2.0*(bd->gammai11*bd->d1phi + bd->gammai12*bd->d2phi + bd->gammai13*bd->d3phi)
            )
        )
      );
}

/**
 * @brief Damped wave gauge shift in y-dir
 */
real_t BSSNGaugeHandler::DampedWaveShift2(BSSNData *bd)
{
  return bd->beta1*bd->d1beta2 + bd->beta2*bd->d2beta2 + bd->beta3*bd->d3beta2
    - dw_mu_s*bd->alpha*bd->beta2
    + bd->alpha * ( -dw_mu_l*(12.0*bd->phi*dw_p - std::log(bd->alpha))*bd->beta2
        + std::exp(-4.0*bd->phi)*(
          - bd->gammai21*bd->d1a - bd->gammai22*bd->d2a - bd->gammai23*bd->d3a
          + bd->alpha*(bd->Gamma3
              -2.0*(bd->gammai21*bd->d1phi + bd->gammai22*bd->d2phi + bd->gammai23*bd->d3phi)
            )
        )
      );
}

/**
 * @brief Damped wave gauge shift in z-dir
 */
real_t BSSNGaugeHandler::DampedWaveShift3(BSSNData *bd)
{
  return bd->beta1*bd->d1beta3 + bd->beta2*bd->d2beta3 + bd->beta3*bd->d3beta3
    - dw_mu_s*bd->alpha*bd->beta3
    + bd->alpha * ( -dw_mu_l*(12.0*bd->phi*dw_p - std::log(bd->alpha))*bd->beta3
        + std::exp(-4.0*bd->phi)*(
          - bd->gammai31*bd->d1a - bd->gammai32*bd->d2a - bd->gammai33*bd->d3a
          + bd->alpha*(bd->Gamma3
              -2.0*(bd->gammai31*bd->d1phi + bd->gammai32*bd->d2phi + bd->gammai33*bd->d3phi)
            )
        )
      );
}


/**
 * @brief AwA gauge wave test lapse
 */
real_t BSSNGaugeHandler::AwAGaugeWaveLapse(BSSNData *bd)
{
  return -1.0*pw2(bd->alpha)*bd->DIFFK;
}

/**
 * @brief AwA shifted gauge wave test lapse
 */
real_t BSSNGaugeHandler::AwAShiftedWaveLapse(BSSNData *bd)
{
  return -1.0*bd->DIFFK;
}

/**
 * @brief AwA shifted gauge wave test shift in x-dir
 */
real_t BSSNGaugeHandler::AwAShiftedWaveShift1(BSSNData *bd)
{
  if(gauge_wave_dir == 1) // x-direction
    return -2.0*bd->K*bd->alpha;

  return 0;
}

/**
 * @brief AwA shifted gauge wave test shift in y-dir
 */
real_t BSSNGaugeHandler::AwAShiftedWaveShift2(BSSNData *bd)
{
  if(gauge_wave_dir == 2) // x-direction
    return -2.0*bd->K*bd->alpha;

  return 0;
}

/**
 * @brief AwA shifted gauge wave test shift in z-dir
 */
real_t BSSNGaugeHandler::AwAShiftedWaveShift3(BSSNData *bd)
{
  if(gauge_wave_dir == 3) // x-direction
    return -2.0*bd->K*bd->alpha;

  return 0;
}



/**
 * @brief Aij driver test gauge 
 */
real_t BSSNGaugeHandler::AijDriverShift1(BSSNData *bd)
{
  return -k_driver_coeff*( std::abs(bssn->ev_A11(bd) + bd->A11) + std::abs(bssn->ev_A12(bd) + bd->A12) + std::abs(bssn->ev_A13(bd) + bd->A13) );
}
real_t BSSNGaugeHandler::AijDriverShift2(BSSNData *bd)
{
  return -k_driver_coeff*( std::abs(bssn->ev_A12(bd) + bd->A12) + std::abs(bssn->ev_A22(bd) + bd->A22) + std::abs(bssn->ev_A23(bd) + bd->A23) );
}
real_t BSSNGaugeHandler::AijDriverShift3(BSSNData *bd)
{
  return -k_driver_coeff*( std::abs(bssn->ev_A13(bd) + bd->A13) + std::abs(bssn->ev_A23(bd) + bd->A23) + std::abs(bssn->ev_A33(bd) + bd->A33) );
}

} // namespace cosmo
