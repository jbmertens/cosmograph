#include "sheets.h"
#include "../components/phase_space_sheet/sheets_ic.h"

namespace cosmo
{

SheetSim::SheetSim()
{
  tot_mass = 0;
}

void SheetSim::init()
{
  _timer["init"].start();

  // initialize base class
  simInit();

  iodata->log("Running phase space sheet type simulation.");
  sheetSim = new Sheet();
  
  _timer["init"].stop();
}

void SheetSim::setICs()
{
  if(_config("ic_type", "") == "sinusoid")
  {
    sheets_ic_sinusoid_1d_diffusion(bssnSim, sheetSim, iodata, tot_mass);
  }
  else if(_config("ic_type", "") == "sinusoid_3d")
  {
    sheets_ic_sinusoid_3d_diffusion(bssnSim, sheetSim, iodata, tot_mass);
  }
  else
  {
    iodata->log("Unsupported IC type!");
    throw -1;
  }

}

void SheetSim::initSheetStep()
{
  _timer["RK_steps"].start();
    bssnSim->stepInit();
    sheetSim->stepInit();
    bssnSim->clearSrc();
    sheetSim->addBSSNSource(bssnSim, tot_mass);

    if(step == 0)
      avg_vol_i = bssnSim->avg_vol;
    else if( pow(bssnSim->avg_vol / avg_vol_i,1.0/3.0) >= 100 )
    {
      iodata->log("Get target expasion, stop running!");
      throw(-1);
    }
    
  _timer["RK_steps"].stop();
}

void SheetSim::outputSheetStep()
{
  _timer["output"].start();
    prepBSSNOutput();
    if(use_bardeen)
      io_svt_violation(iodata, step, bardeen, t);
    io_bssn_fields_snapshot(iodata, step, bssnSim->fields);
    io_bssn_fields_powerdump(iodata, step, bssnSim->fields, fourier);
    io_bssn_dump_statistics(iodata, step, bssnSim->fields, bssnSim->frw);
    io_bssn_constraint_violation(iodata, step, bssnSim);
    io_sheets_snapshot(iodata, step, sheetSim);
    if(step == 0)
    {
      outputStateInformation();
    }
  _timer["output"].stop();
}

void SheetSim::runSheetStep()
{
  _timer["RK_steps"].start();
    // First RK step
    bssnSim->RKEvolve();
    sheetSim->RKStep(bssnSim);
    
    bssnSim->K1Finalize();
    sheetSim->K1Finalize();


    
    // Second RK step source
    bssnSim->clearSrc();
    sheetSim->addBSSNSource(bssnSim, tot_mass);
    
    // Second RK step
    bssnSim->RKEvolve();
    sheetSim->RKStep(bssnSim);

    bssnSim->K2Finalize();
    sheetSim->K2Finalize();

    // Third RK step source
    bssnSim->clearSrc();
    sheetSim->addBSSNSource(bssnSim, tot_mass);
    
    // Third RK step
    bssnSim->RKEvolve();
    sheetSim->RKStep(bssnSim);
    
    bssnSim->K3Finalize();
    sheetSim->K3Finalize();
    
    // Fourth RK step source
    bssnSim->clearSrc();
    sheetSim->addBSSNSource(bssnSim, tot_mass);
    
    // Fourth RK step
    bssnSim->RKEvolve();
    sheetSim->RKStep(bssnSim);

    bssnSim->K4Finalize();
    sheetSim->K4Finalize();
        
    // "current" data should be in the _p array.
  _timer["RK_steps"].stop();
}

void SheetSim::runStep()
{
  runCommonStepTasks();

  initSheetStep();
  outputSheetStep();
  runSheetStep();
}

} /* namespace cosmo */
