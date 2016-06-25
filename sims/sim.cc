#include "sim.h"

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
  num_steps = stoi(_config["steps"]);

  // integrating any light rays?
  if( stoi(_config["ray_integrate"]) == 1 )
  {
    ray_integrate = true;
    ray_flip_step = stoi(_config["ray_flip_step"]);
  }
  else
  {
    ray_integrate = false;
  }

  // Store simulation type
  simulation_type = _config["simulation_type"];
}

CosmoSim::~CosmoSim()
{
  iodata->log(_timer.getStateString());
  std::cout << std::flush;
}

/**
 * @brief      Initialize individual simulation class instances
 */
void CosmoSim::simInit()
{
  // Always use GR fields
  bssnSim = new BSSN();
  bssnSim->init();

  // FFT helper
  fourier = new Fourier();
  fourier->Initialize(NX, NY, NZ,
    bssnSim->fields["DIFFphi_a"]->_array /* arbitrary array for planning */);

  // initialize raytracing if needed
  if(ray_integrate)
  {
    if(USE_HARMONIC_ALPHA) {
      iodata->log("Error - not using synchronous gauge! You must use it for raytracing sims.");
      iodata->log("Please change this setting in cosmo_macros.h and recompile.");
      throw -1;
    }
    int rays_num = std::stoi(_config["rays_num"]);
    init_ray_vector(&rays, rays_num);
  }
}

/**
 * @brief      Run the simulation.
 */
void CosmoSim::run()
{
  iodata->log("Running simulation...");

  _timer["loop"].start();
  while(step <= num_steps)
  {
    runStep();
    step++;
  }
  _timer["loop"].stop();

  iodata->log("\nEnding simulation.");
  iodata->log(
      "Average conformal factor reached: " + stringify(
        average(*bssnSim->fields["DIFFphi_p"]) + bssnSim->frw->get_phi()
      ));
  iodata->log(
      "Average extrinsic curvature reached: " + stringify(
        average(*bssnSim->fields["DIFFK_p"]) + bssnSim->frw->get_K()
      ));
  std::cout << std::flush;
}

void CosmoSim::runRayTraceStep()
{
  // evolve any light rays
  _timer["Raytrace_step"].start();
  auto ray = rays.begin();

  #pragma omp parallel for default(shared) private(ray)
  for(ray = rays.begin(); ray < rays.end(); ++ray)
  {
    // set primitives from BSSN sim
    bssnSim->setRaytracePrimitives(*ray);
    // evolve ray
    (*ray)->setDerivedQuantities();
    (*ray)->evolveRay();
  }    
  _timer["Raytrace_step"].stop();
}

void CosmoSim::outputRayTraceStep()
{
  _timer["output"].start();
  io_raytrace_dump(iodata, step, &rays);
  _timer["output"].stop();
}

void CosmoSim::runCommonStepTasks()
{
  // check for NAN every step
  if(simNumNaNs() > 0)
  {
    iodata->log("\nNAN detected!");
    throw 10;
  }

  // progress bar in terminal
  io_show_progress(step, num_steps);

  // Evolve light rays when integrating backwards
  if(ray_integrate)
  {
    if(step == ray_flip_step) {
      iodata->log("\nFlipping sign of dt @ step = " + std::to_string(step) );
      dt = -dt;
    }
    if(step >= ray_flip_step) {
      outputRayTraceStep();
      runRayTraceStep();
    }
  }
}

void CosmoSim::prepBSSNOutput()
{
  idx_t i, j, k;

  #pragma omp parallel for default(shared) private(i, j, k)
  LOOP3(i,j,k)
  {
    // set_paq_values calculates ricci_a and AijAij_a data, needed for output
    // and potentially subsequent Killing calculations
    BSSNData b_paq = {0}; // data structure associated with bssn sim
    bssnSim->set_paq_values(i, j, k, &b_paq);
    
    // Additionally set KD (killing vector "Delta" quantities)
    bssnSim->set_KillingDelta(i, j, k, &b_paq);
  }
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