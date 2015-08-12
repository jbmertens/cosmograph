
#include "cosmo.h"
#include "globals.h"
#include <cmath>
#include <cfloat>

using namespace std;
using namespace cosmo;

/* global definitions */
TimerManager _timer;
ConfigParser _config;

int main(int argc, char **argv)
{
  _timer["MAIN"].start();
  idx_t i, j, k, s, steps;

  // read in config file
  if(argc != 2)
  {
    cout << "Error: please supply exactly one config filename as an argument.\n";
    return EXIT_FAILURE;
  }
  else
  {
    _config.parse(argv[1]);
  }

  // IO init - will use this for logging.
  IOData iodata;
  io_init(&iodata, _config["output_dir"]);
  // save a copy of config.txt
  io_config_backup(&iodata, argv[1]);

  steps = stoi(_config["steps"]);
  omp_set_num_threads(stoi(_config["omp_num_threads"]));

  // Create simulation
  _timer["init"].start();
    LOG(iodata.log, "Creating initial conditions...\n");

    // Fluid fields
    // Static matter (w=0)
    Static staticSim;
    staticSim.init();
    // DE
    Lambda lambdaSim;

    // GR Fields
    BSSN bssnSim;
    bssnSim.init();

    // generic reusable fourier class for N^3 arrays
    Fourier fourier;
    fourier.Initialize(N, staticSim.fields["D_a"] /* just any N^3 array for planning */);

    // "conformal" initial conditions:
    LOG(iodata.log, "Using conformal initial conditions...\n");
    set_conformal_ICs(bssnSim.fields, staticSim.fields, &fourier, &iodata);

    // Trial FRW class
    staticSim.addBSSNSrc(bssnSim.fields);
    real_t frw_rho = average(bssnSim.fields["r_a"]);
    FRW<real_t> frw (0.0, -sqrt(24.0*PI*frw_rho) /* K */);
    frw.addFluid(frw_rho /* rho */, 0.0 /* 'w' */);

  _timer["init"].stop();

  // evolve simulation
  LOG(iodata.log, "Running simulation...\n");
  _timer["loop"].start();
  for(s=0; s < steps; ++s) {

    _timer["Reference FRW"].start();
    // LOG(iodata.log,    "\n"
    //                 << frw.get_phi()
    //                 << "\n"
    //                 << bssnSim.fields["phi_a"][0]
    //                 << "\n"
    //               );
    int subiters = 100;
    for(int p=0; p<subiters; ++p)
    {
      frw.step(dt/((real_t) subiters));
    }
    _timer["Reference FRW"].stop();

    // output simulation information
    _timer["output"].start();
      io_show_progress(s, steps);
      io_data_dump(bssnSim.fields, staticSim.fields, &iodata, s, &fourier);
    _timer["output"].stop();

    // Run RK steps explicitly here (ties together BSSN + Hydro stuff).
    // See bssn class or hydro class for more comments.
    _timer["RK_steps"].start();

    // Init arrays and calculate source term for next step
      // _p is copied to _a here, which hydro uses
      bssnSim.stepInit();
      // clear existing data
      bssnSim.clearSrc();
      // add hydro source to bssn sim
      staticSim.addBSSNSrc(bssnSim.fields);
      lambdaSim.addBSSNSrc(bssnSim.fields);

    // First RK step, Set Hydro Vars, & calc. constraint
    #pragma omp parallel for default(shared) private(i, j, k)
    LOOP3(i, j, k)
    {
      BSSNData b_paq = {0}; // data structure associated with bssn sim
      bssnSim.K1CalcPt(i, j, k, &b_paq);

if(i==0&&j==0&&k==0) {
  std::cout << "\nDB val: " << b_paq.db << "\n";
}

    }

    // reset source using new metric
    bssnSim.clearSrc();
    // add hydro source to bssn sim
    staticSim.addBSSNSrc(bssnSim.fields);
    lambdaSim.addBSSNSrc(bssnSim.fields);

    bssnSim.regSwap_c_a();

    // Subsequent BSSN steps
      // Second RK step
      #pragma omp parallel for default(shared) private(i, j, k)
      LOOP3(i, j, k)
      {
        BSSNData b_paq = {0}; // data structure associated with bssn sim
        bssnSim.K2CalcPt(i, j, k, &b_paq);
      }

      // reset source using new metric
      bssnSim.clearSrc();
      // add hydro source to bssn sim
      staticSim.addBSSNSrc(bssnSim.fields);
      lambdaSim.addBSSNSrc(bssnSim.fields);

      bssnSim.regSwap_c_a();
      // Third RK step
      #pragma omp parallel for default(shared) private(i, j, k)
      LOOP3(i, j, k)
      {
        BSSNData b_paq = {0}; // data structure associated with bssn sim
        bssnSim.K3CalcPt(i, j, k, &b_paq);
      }

      // reset source using new metric
      bssnSim.clearSrc();
      // add hydro source to bssn sim
      staticSim.addBSSNSrc(bssnSim.fields);
      lambdaSim.addBSSNSrc(bssnSim.fields); 

      bssnSim.regSwap_c_a();

      // Fourth RK step
      #pragma omp parallel for default(shared) private(i, j, k)
      LOOP3(i, j, k)
      {
        BSSNData b_paq = {0}; // data structure associated with bssn sim
        bssnSim.K4CalcPt(i, j, k, &b_paq);
      }

    // Wrap up
      // bssn _f <-> _p
      bssnSim.stepTerm(); 

    _timer["RK_steps"].stop();


    _timer["meta_output_interval"].start();

      if(s%iodata.meta_output_interval == 0)
      {
        idx_t isNaN = 0;
        bssnSim.stepInit();
        bssnSim.clearSrc();
        staticSim.addBSSNSrc(bssnSim.fields);

        real_t mean_hamiltonian_constraint = bssnSim.hamiltonianConstraintMean();
        real_t stdev_hamiltonian_constraint = bssnSim.hamiltonianConstraintStDev(mean_hamiltonian_constraint);
        real_t mean_momentum_constraint = bssnSim.momentumConstraintMagMean();
        real_t stdev_momentum_constraint = bssnSim.momentumConstraintMagStDev(mean_momentum_constraint);
        io_dump_data(mean_hamiltonian_constraint, &iodata, "avg_H_violation");
        io_dump_data(stdev_hamiltonian_constraint, &iodata, "std_H_violation");
        io_dump_data(mean_momentum_constraint, &iodata, "avg_M_violation");
        io_dump_data(stdev_momentum_constraint, &iodata, "std_M_violation");

        // LOG(iodata.log,
        //   "\nFractional Mean / St. Dev Momentum constraint violation is: " <<
        //   mean_momentum_constraint << " / " << stdev_momentum_constraint <<
        //   "\nFractional Mean / St. Dev Hamiltonian constraint violation is: " <<
        //   mean_hamiltonian_constraint << " / " << stdev_hamiltonian_constraint <<
        //   "\n"
        // );

        if(numNaNs(bssnSim.fields["phi_a"]) > 0)
        {
          LOG(iodata.log, "\nNAN detected!\n");
          _timer["meta_output_interval"].stop();
          break;
        }
      }
    _timer["meta_output_interval"].stop();

  }
  _timer["loop"].stop();

  _timer["output"].start();
  LOG(iodata.log, "\nAverage conformal factor reached " << average(bssnSim.fields["phi_p"]) << "\n");
  LOG(iodata.log, "Ending simulation.\n");
  _timer["output"].stop();

  _timer["MAIN"].stop();

  LOG(iodata.log, endl << _timer << endl);

  return EXIT_SUCCESS;
}
