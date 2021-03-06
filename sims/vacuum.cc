#include "vacuum.h"
#include "../components/bssn/bssn_ic.h"

namespace cosmo
{

void VacuumSim::init()
{
  _timer["init"].start();

  // initialize base class
  simInit();

  iodata->log("Running 'vacuum' type simulation.");
  _timer["init"].stop();
}

/**
 * @brief      Set vacuum initial conditions
 *
 * @param[in]  map to BSSN fields
 * @param      initialized IOData
 */
void VacuumSim::setICs()
{
  _timer["ICs"].start();

  if(_config["ic_type"] == "stability")
  {
    iodata->log("Setting stability initial conditions.");
    real_t A = std::stod(_config("noise_amp", "1.0e-10"));
    bssn_ic_awa_stability(bssnSim, A);
  }
  else if(_config["ic_type"] == "linear_wave")
  {
    iodata->log("Setting linear wave initial conditions.");
    int dir = std::stoi(_config("wave_dir", "1"));
    real_t A = std::stod(_config("wave_amp", "1.0e-8"));
    bssn_ic_awa_linear_wave(bssnSim, A, dir);
  }
  else if(_config["ic_type"] == "linear_wave_desitter")
  {
    iodata->log("Setting linear wave + desitter initial conditions.");
    bssn_ic_awa_linear_wave_desitter(bssnSim);
  }
  else if(_config["ic_type"] == "gauge_wave")
  {
    iodata->log("Setting gauge wave initial conditions.");
    int dir = std::stoi(_config("gauge_wave_dir", "1"));
    bssn_ic_awa_gauge_wave(bssnSim, dir);
  }
  else if(_config["ic_type"] == "shifted_gauge_wave")
  {
    iodata->log("Setting shifted gauge wave initial conditions.");
    int dir = std::stoi(_config("gauge_wave_dir", "1"));
    bssn_ic_awa_shifted_gauge_wave(bssnSim, dir);
  }
  else if(_config["ic_type"] == "kasner")
  {
    iodata->log("Setting kasner initial conditions.");
    int px = std::stod(_config("kasner_px", "0.5"));
    bssn_ic_kasner(bssnSim, px);
  }
  else
  {
    iodata->log("IC type not recognized!");
    throw -1;
  }
  iodata->log("Finished setting ICs.");
  _timer["ICs"].stop();
}

void VacuumSim::initVacuumStep()
{
  _timer["RK_steps"].start();
    bssnSim->stepInit();
  _timer["RK_steps"].stop();
}

void VacuumSim::outputVacuumStep()
{
  _timer["output"].start();
    prepBSSNOutput();
    io_bssn_fields_snapshot(iodata, step, bssnSim->fields);
    io_bssn_fields_powerdump(iodata, step, bssnSim->fields, fourier);
    io_bssn_dump_statistics(iodata, step, bssnSim->fields, bssnSim->frw);
    io_bssn_constraint_violation(iodata, step, bssnSim);
  _timer["output"].stop();
}

void VacuumSim::runVacuumStep()
{
  _timer["RK_steps"].start();
    // Full RK step minus init()
    bssnSim->step();
  _timer["RK_steps"].stop();
}

void VacuumSim::runStep()
{
  runCommonStepTasks();

  initVacuumStep();
  outputVacuumStep();
  runVacuumStep();
}

} /* namespace cosmo */
