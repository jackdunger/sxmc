#include <cmath>
#include <hemi/hemi.h>
#include <TRandom.h>
#include "nll_kernels.h"

#ifdef __CUDACC__
#include <curand_kernel.h>
#else
#include <TMath.h>
#endif

#ifndef __HEMI_ARRAY_H__
#define __HEMI_ARRAY_H__
#include <hemi/array.h>
#endif

#ifdef __CUDACC__
__global__ void init_device_rngs(int nthreads, unsigned long long seed,
                                 curandStateXORWOW* state) {
  int idx = hemiGetElementOffset();
  if (idx >= nthreads) {
    return;
  }
  curand_init(seed, idx, 0, &state[idx]);
}
#endif

HEMI_KERNEL(pick_new_vector)(int nthreads, RNGState* rng, float sigma,
                             const float* current_vector,
                             float* proposed_vector) {
  int offset = hemiGetElementOffset();
  int stride = hemiGetElementStride();

  for (int i=offset; i<(int)nthreads; i+=stride) {
#ifdef HEMI_DEV_CODE
    float u = curand_normal(&rng[i]);
    proposed_vector[i] = current_vector[i] + sigma * u;
#else
    float u = gRandom->Gaus(current_vector[i], sigma);
    proposed_vector[i] = u;
#endif
  }
}


HEMI_KERNEL(jump_decider)(RNGState* rng, double* nll_current,
                          const double* nll_proposed, float* v_current,
                          const float* v_proposed, unsigned ns, int* counter,
                          float* jump_buffer) {
#ifdef HEMI_DEV_CODE
  float u = curand_uniform(&rng[0]);
#else
  float u = gRandom->Uniform();
#endif

  // metropolis algorithm
  double np = nll_proposed[0];
  double nc = nll_current[0];
  int count = counter[0];
  if (np < nc || u <= exp(nc - np)) {
    nll_current[0] = np;
    for (unsigned i=0; i<ns; i++) {
      float vpi = v_proposed[i];
      v_current[i] = vpi;
      jump_buffer[count * (ns + 1) + i] = vpi;
    }
    jump_buffer[count * (ns + 1) + ns] = nll_proposed[0];
    counter[0] = count + 1;    
  }
}


HEMI_KERNEL(nll_event_chunks)(const float* lut, const float* pars,
                              const size_t ne, const size_t ns,
                              double* sums) {
  int offset = hemiGetElementOffset();
  int stride = hemiGetElementStride();

  double sum = 0;
  for (int i=offset; i<(int)ne; i+=stride) {
    double s = 0;
    for (size_t j=0; j<ns; j++) {
      s += pars[j] * lut[i * ns + j];
    }
    sum += log(s);
  }

  sums[offset] = sum;
}


HEMI_KERNEL(nll_event_reduce)(const size_t nthreads, const double* sums,
                              double* total_sum) {
  int offset = hemiGetElementOffset();
  int stride = hemiGetElementStride();

  double sum = 0;
  double sum2 = 0;
  for (int i=offset; i<(int)nthreads; i+=stride) {
    sum += sums[i];
    sum2 -= sums[i];
  }
  total_sum[0] = sum + sum2;
  total_sum[0] = sum + sum2 + 1;
}


HEMI_KERNEL(nll_total)(const size_t ns, const float* pars,
                       const float* expectations,
                       const float* constraints,
                       const double* events_total,
                       double* nll) {
  // total from sum over events, once
  double sum = -events_total[0];

  for (unsigned i=0; i<ns; i++) {
    // non-negative rates
    if (pars[i] < 0) {
      sum += 1e6;
    }

    // normalization constraints
    sum += pars[i];

    // gaussian constraints
    if (constraints[i] > 0 && expectations[i] > 0) {
      float x = (pars[i] / expectations[i] - 1.0) / constraints[i];
      sum += x * x;
    }
  }

  nll[0] = sum;
}

