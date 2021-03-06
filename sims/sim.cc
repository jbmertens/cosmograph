#include "sim.h"
#include "../ICs/ICs.h"
#include "../cosmo_globals.h"

namespace cosmo
{

CosmoSim::CosmoSim()
{
  // Initialize iodata first
  iodata = new IOData(_config["output_dir"]);
  // save a copy of config.txt; print defines
  log_defines(iodata);
  iodata->backupFile(_config.getFileName());

  // fix number of simulation steps
  step = 0;
  t = 0;
  num_steps = stoi(_config["steps"]);

# if USE_COSMOTRACE
  // integrating any light rays?
  if( stoi(_config("ray_integrate", "0")) )
  {
    ray_integrate = true;
    ray_flip_step = stoi(_config["ray_flip_step"]);
  }
  else
  {
    ray_integrate = false;
  }

  if( stoi(_config("simple_raytrace", "0")) )
  {
    simple_raytrace = true;
  }
  else
  {
    simple_raytrace = false;
  }
# endif

  if( stoi(_config("use_bardeen", "0")) )
  {
    use_bardeen = true;
  }
  else
  {
    use_bardeen = false;
  }

  // Store simulation type
  simulation_type = _config["simulation_type"];
}

/**
 * @brief      Initialize individual simulation class instances
 */
void CosmoSim::simInit()
{
  // FFT helper
  fourier = new Fourier();
  fourier->Initialize(NX, NY, NZ);

  // Always use GR fields
  bssnSim = new BSSN(&_config, fourier);

# if USE_COSMOTRACE
  // initialize raytracing if needed
  if(ray_integrate)
  {
    if(_config("lapse", "") != "" && _config("lapse", "") != "Static" && _config("lapse", "") != "ConformalFLRW")
    {
      iodata->log("Error - not using synchronous gauge! You must use it for raytracing sims.");
      iodata->log("Please change this setting in the config file and re-run.");
      throw -1;
    }
    init_ray_vector(&rays);
  }
# endif

  if(use_bardeen)
  {
    bardeen = new Bardeen(bssnSim, fourier);
    bool use_ML_scale_factor = !!std::stoi(_config("use_ML_scale_factor", "1"));
    bardeen->setUseMLScaleFactor(use_ML_scale_factor);
    real_t Omega_L = std::stod(_config("Omega_L", "0.0"));
    bardeen->useMLScaleFactor(Omega_L);
    if(use_ML_scale_factor)
    {
      iodata->log("Using Matter+Lambda FLRW scale factor for Bardeen calculations with Omega_L = "
        + stringify(Omega_L) + ".");
    }

  }
}

/**
 * @brief      Run the simulation.
 */
void CosmoSim::run()
{
  iodata->log("Running simulation...");

  _timer["loop"].start();
  real_t avg_vol_i = 1.0;
  while(step <= num_steps)
  {
    runStep();
    t += dt;

    if(step == 0)
    {
      avg_vol_i = bssnSim->avg_vol;
    }
    else if( !!std::stod(_config("stop_at_expansion_goal", "0"))
      && std::pow(bssnSim->avg_vol / avg_vol_i, 1.0/3.0) >= std::stod(_config("expansion_goal", "100.0")) )
    {
      iodata->log("Target expasion reached, run ending.");
      break;
    }

    step++;
  }
  _timer["loop"].stop();

  iodata->log("\nEnding simulation.");
  outputStateInformation();
  iodata->log(_timer.getStateString());
  std::cout << std::flush;
}

#if USE_COSMOTRACE
void CosmoSim::runRayTraceStep()
{
  // evolve any light rays
  _timer["Raytrace_step"].start();

  idx_t n = 0;
  idx_t num_rays = rays.size();

  #pragma omp parallel for default(shared) private(n)
  for(n = 0; n < num_rays; ++n)
  {
    RayTrace<real_t, idx_t> * ray = rays[n];

    // set primitives from BSSN sim
    bssnSim->setRaytracePrimitives(ray);
    // evolve ray
    ray->setDerivedQuantities();
    if(simple_raytrace)
    {
      ray->evolveRayBasic();
    }
    else
    {
      ray->evolveRay();
    }
  }
  _timer["Raytrace_step"].stop();
}

void CosmoSim::outputRayTraceStep()
{
  _timer["output"].start();
  
  io_raytrace_dump(iodata, step, &rays);
  
  if(use_bardeen)
    io_raytrace_bardeen_dump(iodata, step, &rays, bardeen, t);

  _timer["output"].stop();
}
#endif

void CosmoSim::runCommonStepTasks()
{
  // check for NAN every step
  if(simNumNaNs() > 0)
  {
    iodata->log("\nNAN detected!");
    throw 10;
  }

  // progress bar in terminal
  if(step % 100 == 0)
    io_show_progress(step, num_steps);

// # if USE_GENERALIZED_NEWTON
//   real_t dt0 = std::stold(_config( "dt_frac", "0.1" ))*dx;
//   real_t frac_done = (num_steps - step) / (real_t) num_steps;

//   dt = dt0 + frac_done*frac_done*100*dt0;
//   bssnSim->setDt(dt);
// # endif

# if USE_COSMOTRACE
  // Evolve light rays when integrating backwards
  if(ray_integrate)
  {
    if(step == ray_flip_step) {
      iodata->log("\nFlipping sign of dt @ step = " + std::to_string(step) );
      dt = -std::abs(dt);
      bssnSim->setDt(dt);
    }
    if(step >= ray_flip_step) {
      outputRayTraceStep();
      runRayTraceStep();
    }
  }
# endif
}

void CosmoSim::prepBSSNOutput()
{
  idx_t i, j, k;

  #pragma omp parallel for default(shared) private(i, j, k)
  LOOP3(i,j,k)
  {
    // set_bd_values calculates ricci_a and AijAij_a data, needed for output
    // and potentially subsequent Killing calculations
    BSSNData b_data = {0}; // data structure associated with bssn sim
    bssnSim->set_bd_values(i, j, k, &b_data);
  }

  if(use_bardeen)
    bardeen->setPotentials(t);

  bssnSim->cur_t = t;
}

void CosmoSim::outputStateInformation()
{
  // make sure simulation information is present in the _a register
  // TODO: Source values may be off; correct this?
  bssnSim->stepInit();
  prepBSSNOutput();

  iodata->log( "---- Information from step " + stringify(step)
    + " of " + stringify(num_steps) + " ----" );

  iodata->log( "RMS / avg. density is "
    + stringify(
        standard_deviation(*bssnSim->fields["DIFFr_a"])
          / average(*bssnSim->fields["DIFFr_a"])
    ));

  iodata->log(
      "Average | Min | Max conformal factor: " + stringify(
        average(*bssnSim->fields["DIFFphi_a"]) + bssnSim->frw->get_phi()
      ) + " | " + stringify(
        min(*bssnSim->fields["DIFFphi_a"]) + bssnSim->frw->get_phi()
      ) + " | " + stringify(
        max(*bssnSim->fields["DIFFphi_a"]) + bssnSim->frw->get_phi()
      ));
  iodata->log(
      "Average | Min | Max extrinsic curvature: " + stringify(
        average(*bssnSim->fields["DIFFK_a"]) + bssnSim->frw->get_K()
      ) + " | " + stringify(
        min(*bssnSim->fields["DIFFK_a"]) + bssnSim->frw->get_K()
      ) + " | " + stringify(
        max(*bssnSim->fields["DIFFK_a"]) + bssnSim->frw->get_K()
      ));
  real_t H_calcs[7] = {0}, M_calcs[7] = {0}, G_calcs[7] = {0},
         A_calcs[7] = {0}, S_calcs[7] = {0};
  bssnSim->setConstraintCalcs(H_calcs, M_calcs, G_calcs,
                              A_calcs, S_calcs);
  iodata->log(
      "Max. (Normed) Hamiltonian constraint violation: "
      + stringify(H_calcs[2]) + " (" + stringify(H_calcs[6]) + ")"
    );
  iodata->log(
      "Max. (Normed) Momentum constraint violation: "
      + stringify(M_calcs[2]) + " (" + stringify(M_calcs[6]) + ")"
    );
# if USE_BSSN_SHIFT
  iodata->log("num. e-folds expanded for is: "
    + stringify( exp(conformal_average( *bssnSim->fields["expN_a"],
      *bssnSim->fields["DIFFphi_a"], bssnSim->frw->get_phi() )) )
    );
# endif
  
# if USE_COSMOTRACE
  if(ray_integrate)
  {
    RaytraceData<real_t> rd = rays[0]->getRaytraceData();
    iodata->log(
        "rays[0] beam width: "
        + stringify(rd.ell)
      );
  }
# endif

  iodata->log( "---- ---- ---- ----" );

}

idx_t CosmoSim::simNumNaNs()
{
  // check for NAN in a field
  return numNaNs(*bssnSim->fields["DIFFphi_a"]);
}

void CosmoSim::setVerbosity(int verbosity_in)
{
  verbosity = verbosity_in;
}

} /* namespace cosmo */
