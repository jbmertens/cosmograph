#!/bin/bash

# Switch to the directory containing this script,
cd "$(dirname "$0")"
# up a directory should be the main codebase.
cd ../..
mkdir -p build/3d_comp_newt
cd build/3d_comp_newt


../../scripts/1d_runs/1d_runs.sh -C -N=0064 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.02
../../scripts/1d_runs/1d_runs.sh -C -N=0096 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.02
../../scripts/1d_runs/1d_runs.sh -C -N=0128 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.02

../../scripts/1d_runs/1d_runs.sh -C -N=0064 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.01
../../scripts/1d_runs/1d_runs.sh -C -N=0096 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.01
../../scripts/1d_runs/1d_runs.sh -C -N=0128 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.01

../../scripts/1d_runs/1d_runs.sh -C -N=0064 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.005
../../scripts/1d_runs/1d_runs.sh -C -N=0096 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.005
../../scripts/1d_runs/1d_runs.sh -C -N=0128 -A=0.1 -l=0.5 -g=GeneralizedNewton --GN_eta=0.005

