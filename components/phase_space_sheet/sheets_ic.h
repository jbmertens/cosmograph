/** @file scalar_ic.h
 * @brief Functions to set initial conditions for the Scalar class.
 * Functions should be made callable via a config setting in ScalarSim
 * class.
 */

#ifndef COSMO_SHEETS_ICS
#define COSMO_SHEETS_ICS

#include "../bssn/bssn.h"
#include "../Lambda/lambda.h"
#include "sheets.h"
#include "../../IO/io.h"

namespace cosmo
{
  void sheets_ic_sinusoid(
    BSSN *bssnSim, Sheet *sheetSim, Lambda * lambda, IOData * iodata, real_t & tot_mass);

  void sheets_ic_semianalytic(BSSN *bssnSim, Sheet *sheetSim,
    Lambda * lambda, IOData * iodata, real_t & tot_mass);

  void sheets_ic_sinusoid_3d(
    BSSN *bssnSim, Sheet *sheetSim, IOData * iodata, real_t & tot_mass);

  void sheets_ic_sinusoid_3d_diffusion(
    BSSN *bssnSim, Sheet *sheetSim, Lambda * lambda, IOData * iodata, real_t & tot_mass);

  void sheets_ic_sinusoid_1d_diffusion(
    BSSN *bssnSim, Sheet *sheetSim, Lambda * lambda, IOData * iodata, real_t & tot_mass);

  void sheet_ic_set_vectorpert(BSSN *bssnSim, Sheet *sheetSim,
    IOData * iodata, real_t & tot_mass);

}

#endif
