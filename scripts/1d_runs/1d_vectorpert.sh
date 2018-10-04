#!/bin/bash

EXEC_DIR=$(pwd)
# Switch to the directory containing this script,
cd "$(dirname "$0")"
# up two directories should be the main codebase.
mkdir -p ../../build/vector
cd ../../build/vector
pwd

# Some colors
RED='\033[31m'
GREEN='\033[32m'
YELLOW='\033[33m'
BLUE='\033[36m'
NC='\033[0m' # No Color


# default options
USE_CLUSTER=false
DRY_RUN=false
RES_STR='0064'
BOX_LENGTH=1
GAUGE='Harmonic'
CARRIERS_PER_DX=2
MODE_AMPLITUDE=0.002
# Add'l options, but not read in:
CARRIER_COUNT_SCHEME=1
DEPOSIT_SCHEME=1
METHOD_ORDER_RES=4
GN_eta=0.001
STEPS_FAC=1
RES_NS_RED=8
DT_FRAC=0.1

# read in options
for i in "$@"
do
  case $i in
      -h|--help)
      printf "Usage: ./1d_runs.sh\n"
      printf "         [-C|--cluster-run] [-d|--dry-run]\n"
      printf "         [(-N|--resolution-N)='0064'] [(-l|--l)=1] [(-b|--amplitude)=0.002]\n"
      printf "         [(-c|--carriers)=2] [(-g|--gauge)=Harmonic] [--GN_eta=0.001]\n"
      printf "         [(-s|--steps-fac)=3000] [(-n|--ns-reduction)=8]\n"
      exit 0
      ;;
      -N=*|--resolution-N=*)
      RES_STR="${i#*=}"
      shift # past argument=value
      ;;
      -l=*|--l=*)
      BOX_LENGTH="${i#*=}"
      shift # past argument=value
      ;;
      -g=*|--gauge=*)
      GAUGE="${i#*=}"
      shift # past argument=value
      ;;
      --GN_eta=*)
      GN_eta="${i#*=}"
      shift # past argument=value
      ;;
      -b=*|--amplitude=*)
      MODE_AMPLITUDE="${i#*=}"
      shift # past argument=value
      ;;
      -c=*|--carriers=*)
      CARRIERS_PER_DX="${i#*=}"
      shift # past argument
      ;;
      -C|--cluster-run)
      USE_CLUSTER=true
      shift # past argument
      ;;
      -d|--dry-run)
      DRY_RUN=true
      shift # past argument
      ;;
      -s=*|--steps-fac=*)
      STEPS_FAC="${i#*=}"
      shift # past argument
      ;;
      -n=*|--ns-reduction=*)
      RES_NS_RED="${i#*=}"
      shift # past argument
      ;;
      *)
        printf "Unrecognized option will not be used: ${i#*=}\n"
        # unknown option
      ;;
  esac
done

# derived vars
RES_INT=$(echo $RES_STR | sed 's/^0*//')
RES_NS=$((RES_INT*RES_INT/RES_NS_RED))
STEPS=$(bc <<< "$RES_INT*$STEPS_FAC/$BOX_LENGTH/$DT_FRAC")
IO_INT=1
IO_INT_1D=1
IO_INT_3D=$((STEPS/10))
if ((IO_INT_3D<1)); then
  IO_INT_3D=1
fi
METHOD_ORDER=$((METHOD_ORDER_RES*2))
IC_PPDX=$((RES_INT*20000))
OUTPUT_DIR=output
DIR="$EXEC_DIR/1D_run-L_$BOX_LENGTH-r_$RES_STR-Odx_$METHOD_ORDER-cpdx_$CARRIERS_PER_DX-ccs_$CARRIER_COUNT_SCHEME-ds_$DEPOSIT_SCHEME-b_${MODE_AMPLITUDE}_$GAUGE"

printf "${BLUE}Deploying runs:${NC}\n"
printf "  Will be using directory: $DIR\n"
if "$USE_CLUSTER"; then
  printf "  Using cluster (USE_CLUSTER=$USE_CLUSTER)\n"
else
  printf "  Running locally  (USE_CLUSTER=$USE_CLUSTER)\n"
fi
if "$DRY_RUN"; then
  printf "  ${YELLOW}Performing dry run (code will not be run)${NC}\n"
fi

read -r -t 3 -p "Continue to deploy? Will automatically proceed in 3 seconds... [Y/n]: " response
response=${response,,}    # tolower
if ! [[ $response =~ ^(|y|yes)$ ]] ; then
  printf "${RED}Aborting deployment.${NC}\n"
  exit 1
fi
printf "${GREEN}Deploying...${NC}\n"
printf "\n"

mkdir -p $DIR
cd $DIR

TMP_CONFIG_FILE=config.txt
cp ../../../config/tests/vector.txt $TMP_CONFIG_FILE
USE_Z4c=1


sed -i -E "s/ns2 = [\.0-9]+/ns2 = $RES_NS/g" $TMP_CONFIG_FILE
sed -i -E "s/carriers_per_dy = [0-9]+/carriers_per_dy = $CARRIERS_PER_DX/g" $TMP_CONFIG_FILE
sed -i -E "s/carrier_count_scheme = [\.0-9]+/carrier_count_scheme = $CARRIER_COUNT_SCHEME/g" $TMP_CONFIG_FILE
sed -i -E "s/deposit_scheme = [0-9]+/deposit_scheme = $DEPOSIT_SCHEME/g" $TMP_CONFIG_FILE
sed -i -E "s/integration_points_per_dx = [0-9]+/integration_points_per_dx = $IC_PPDX/g" $TMP_CONFIG_FILE
sed -i -E "s/steps = [0-9]+/steps = $STEPS/g" $TMP_CONFIG_FILE
sed -i -E "s/IO_1D_grid_interval = [0-9]+/IO_1D_grid_interval = $IO_INT_1D/g" $TMP_CONFIG_FILE
sed -i -E "s/IO_3D_grid_interval = [0-9]+/IO_3D_grid_interval = $IO_INT_3D/g" $TMP_CONFIG_FILE
sed -i -E "s/IO_bssnstats_interval = [0-9]+/IO_bssnstats_interval = $IO_INT/g" $TMP_CONFIG_FILE
sed -i -E "s/IO_constraint_interval = [0-9]+/IO_constraint_interval = $IO_INT/g" $TMP_CONFIG_FILE
sed -i -E "s/SVT_constraint_interval = [0-9]+/SVT_constraint_interval = $IO_INT/g" $TMP_CONFIG_FILE
sed -i -E "s/b = [\.0-9]+/b = $MODE_AMPLITUDE/g" $TMP_CONFIG_FILE
sed -i -E "s/lapse = [a-zA-Z]+/lapse = $GAUGE/g" $TMP_CONFIG_FILE
sed -i -E "s/dt_frac = [\.0-9]+/dt_frac = $DT_FRAC/g" $TMP_CONFIG_FILE
sed -i -E "s,output_dir = [[:alnum:]_-\./]+,output_dir = $OUTPUT_DIR,g" $TMP_CONFIG_FILE


# Load modules
if "$USE_CLUSTER"; then
  printf "Loading Modules...\n"
  module load cmake
fi

cmake ../../.. -DCOSMO_N=$RES_INT -DCOSMO_NX=1 -DCOSMO_NZ=1 -DCOSMO_USE_GENERALIZED_NEWTON=0\
   -DCOSMO_STENCIL_ORDER=$METHOD_ORDER -DCOSMO_USE_REFERENCE_FRW=0 -DCOSMO_H_LEN_FRAC=$BOX_LENGTH\
   -DCOSMO_USE_Z4c_DAMPING=$USE_Z4c -DCOSMO_USE_LONG_DOUBLES=0 && make -j24
if [ $? -ne 0 ]; then
  printf "${RED}Error: compilation failed!${NC}\n"
  exit 1
fi

if "$USE_CLUSTER"; then
  cp ../../../scripts/job_template.slurm job.slurm
  # Adjust job name
  sed -i.bak "s/JOBNAME/1d-$RES_STR-$BOX_LENGTH-$MODE_AMPLITUDE/" job.slurm
fi

# Run job, go back up a dir
if [ "$DRY_RUN" = false ]; then
  if "$USE_CLUSTER"; then
    printf "${GREEN}Queueing job.${NC}\n"
    sbatch job.slurm
  else
    printf "${GREEN}Running job.${NC}\n"
    ./cosmo $TMP_CONFIG_FILE
  fi
fi

if "$USE_CLUSTER"; then
  USER=$(whoami)
  printf "squeue -u $USER\n"
  squeue -u $USER
  printf "squeue -u $USER --start\n"
  squeue -u $USER --start
fi

printf "\n"
printf "All done!\n"
