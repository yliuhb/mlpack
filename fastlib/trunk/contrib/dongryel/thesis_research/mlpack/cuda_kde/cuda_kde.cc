/** @file cuda_kde.cc
 *
 *  The main driver for the CUDA based KDE.
 *
 *  @author Dongryeol Lee (dongryel@cc.gatech.edu)
 */

#include <armadillo>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "core/util/timer.h"
#include "core/tree/gen_metric_tree.h"
#include "mlpack/kde/kde_dev.h"
#include "mlpack/series_expansion/kernel_aux.h"

extern "C" void NbodyKernelOnHost(
  int num_dimensions,
  float bandwidth,
  float *query, int num_query_points,
  float *reference, int num_reference_points,
  float *kernel_sums_out);

template<typename KernelAuxType>
void StartComputation(boost::program_options::variables_map &vm) {

  // Tree type: hard-coded for a metric tree.
  typedef core::table::Table <
  core::tree::GenMetricTree <
  mlpack::kde::KdeStatistic <
  KernelAuxType::ExpansionType > > > TableType;

  // Parse arguments for Kde.
  mlpack::kde::KdeArguments<TableType> kde_arguments;
  if(mlpack::kde::KdeArgumentParser::ParseArguments(vm, &kde_arguments)) {
    return;
  }

  // Invoke the CUDA kernel.
  float *tmp_query_host = new float[1024];
  float *tmp_reference_host = new float[1024];
  float *query_device = NULL;
  float *reference_device = NULL;
  float *kernel_sums_device = NULL;
  NbodyKernelOnHost(
    kde_arguments.reference_table_->n_attributes(),
    static_cast<float>(kde_arguments.bandwidth_),
    query_device,
    kde_arguments.query_table_->n_entries(),
    reference_device,
    kde_arguments.reference_table_->n_entries(),
    kernel_sums_device);
}

int main(int argc, char *argv[]) {

  boost::program_options::variables_map vm;
  if(mlpack::kde::KdeArgumentParser::ConstructBoostVariableMap(
        argc, argv, &vm)) {
    return 0;
  }

  // Do a quick peek at the kernel and expansion type.
  std::string kernel_type = vm["kernel"].as<std::string>();
  std::string series_expansion_type =
    vm["series_expansion_type"].as<std::string>();

  if(kernel_type == "gaussian") {
    if(series_expansion_type == "hypercube") {
      StartComputation <
      mlpack::series_expansion::GaussianKernelHypercubeAux > (vm);
    }
    else {
      StartComputation <
      mlpack::series_expansion::GaussianKernelMultivariateAux > (vm);
    }
  }
  else {

    // Only the multivariate expansion is available for the
    // Epanechnikov.
    StartComputation <
    mlpack::series_expansion::EpanKernelMultivariateAux > (vm);
  }

  return 0;
}