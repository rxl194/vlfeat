/** @file   vl_gmm.c
 ** @brief  vl_gmm MEX definition.
 ** @author David Novotny
 **/

/*
Copyright (C) 2013 David Novotny.
All rights reserved.

This file is part of the VLFeat library and is made available under
the terms of the BSD license (see the COPYING file).
*/

#include <vl/gmm.h>
#include <mexutils.h>
#include <string.h>
#include <stdio.h>

enum
{
  opt_max_num_iterations,
  opt_distance,
  opt_initialization,
  opt_num_repetitions,
  opt_verbose,
  opt_means,
  opt_covariances,
  opt_priors,
  opt_sigma_low_bound
} ;

enum
{
  INIT_RAND,
  INIT_CUSTOM
} ;

vlmxOption  options [] =
{
  {"MaxNumIterations",  1,   opt_max_num_iterations  },
  {"Verbose",           0,   opt_verbose             },
  {"NumRepetitions",    1,   opt_num_repetitions,    },
  {"Initialization",    1,   opt_initialization      },
  {"Initialisation",    1,   opt_initialization      }, /* UK spelling */
  {"InitMeans",         1,   opt_means               },
  {"InitCovariances",        1,   opt_covariances              },
  {"InitPriors",       1,   opt_priors             },
  {"SigmaBound",        1,   opt_sigma_low_bound     },
  {0,                   0,   0                       }
} ;

/* driver */
void
mexFunction (int nout, mxArray * out[], int nin, const mxArray * in[])
{
  enum {IN_DATA = 0, IN_NUMCLUSTERS, IN_END} ;
  enum {OUT_MEANS, OUT_COVARIANCES, OUT_PRIORS, OUT_LL, OUT_POSTERIORS} ;

  int opt ;
  int next = IN_END ;
  mxArray const  *optarg ;

  vl_size i;

  vl_size numClusters = 10;
  vl_size dimension ;
  vl_size numData ;

  void * initCovariances = 0;
  void * initMeans = 0;
  void * initPriors = 0;
  vl_bool meansSet = VL_FALSE;
  vl_bool covariancesSet = VL_FALSE;
  vl_bool priorsSet = VL_FALSE;

  double sigmaLowBound = 0.000001;
  void const * data = NULL ;

  vl_size maxNumIterations = 100 ;
  vl_size numRepetitions = 1 ;
  double LL ;
  int verbosity = 0 ;
  VlGMMInitialization initialization = VlGMMRand ;

  vl_type dataType ;
  mxClassID classID ;

  VlGMM * gmm ;

  VL_USE_MATLAB_ENV ;

  /* -----------------------------------------------------------------
   *                                               Check the arguments
   * -------------------------------------------------------------- */

  if (nin < 2)
  {
    vlmxError (vlmxErrInvalidArgument,
               "At least two arguments required.");
  }
  else if (nout > 5)
  {
    vlmxError (vlmxErrInvalidArgument,
               "Too many output arguments.");
  }

  classID = mxGetClassID (IN(DATA)) ;
  switch (classID) {
    case mxSINGLE_CLASS: dataType = VL_TYPE_FLOAT ; break ;
    case mxDOUBLE_CLASS: dataType = VL_TYPE_DOUBLE ; break ;
    default:
      vlmxError (vlmxErrInvalidArgument,
                 "DATA is neither of class SINGLE or DOUBLE.") ;
      abort() ;
  }

  dimension = mxGetM (IN(DATA)) ;
  numData = mxGetN (IN(DATA)) ;

  if (dimension == 0)
  {
    vlmxError (vlmxErrInvalidArgument, "SIZE(DATA,1) is zero.") ;
  }

  if (!vlmxIsPlainScalar(IN(NUMCLUSTERS)) ||
      (numClusters = (vl_size) mxGetScalar(IN(NUMCLUSTERS))) < 1  ||
      numClusters > numData)
  {
    vlmxError (vlmxErrInvalidArgument,
               "NUMCLUSTERS must be a positive integer not greater "
               "than the number of data.") ;
  }

  while ((opt = vlmxNextOption (in, nin, options, &next, &optarg)) >= 0)
  {
    char buf [1024] ;

    switch (opt)
    {

    case opt_verbose :
      ++ verbosity ;
      break ;

    case opt_max_num_iterations :
      if (!vlmxIsPlainScalar(optarg) || mxGetScalar(optarg) < 0)
      {
        vlmxError (vlmxErrInvalidArgument,
                   "MAXNUMITERATIONS must be a non-negative integer scalar") ;
      }
      maxNumIterations = (vl_size) mxGetScalar(optarg) ;
      break ;

    case opt_sigma_low_bound :
      sigmaLowBound = (double) mxGetScalar(optarg) ;
      break ;

    case opt_priors : ;
	  {
      mxClassID classIDpriors = mxGetClassID (optarg) ;
      switch (classIDpriors)
      {
      case mxSINGLE_CLASS:
        if (dataType == VL_TYPE_DOUBLE)
        {
          vlmxError (vlmxErrInvalidArgument, "INITPRIORS must be of same data type as X") ;
        }
        break ;
      case mxDOUBLE_CLASS:
        if (dataType == VL_TYPE_FLOAT)
        {
          vlmxError (vlmxErrInvalidArgument, "INITPRIORS must be of same data type as X") ;
        }
        break ;
      default :
        abort() ;
        break ;
      }

      if (! vlmxIsMatrix (optarg, -1, -1) || ! vlmxIsReal (optarg))
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITPRIORS must be a real matrix ") ;
      }

      if (mxGetNumberOfElements(optarg) != numClusters)
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITPRIORS has to have numClusters elements") ;
      }

      priorsSet = VL_TRUE;
      initPriors = mxGetPr(optarg) ;

      break;
	  }

    case opt_means : ;
	  {
      mxClassID classIDmeans = mxGetClassID (optarg) ;
      switch (classIDmeans)
      {
      case mxSINGLE_CLASS:
        if (dataType == VL_TYPE_DOUBLE)
        {
          vlmxError (vlmxErrInvalidArgument, "INITMEANS must be of same data type as X") ;
        }
        break ;
      case mxDOUBLE_CLASS:
        if (dataType == VL_TYPE_FLOAT)
        {
          vlmxError (vlmxErrInvalidArgument, "INITMEANS must be of same data type as X") ;
        }
        break ;
      default :
        abort() ;
        break ;
      }

      if (! vlmxIsMatrix (optarg, -1, -1) || ! vlmxIsReal (optarg))
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITMEANS must be a real matrix ") ;
      }

      if (mxGetM(optarg) != dimension)
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITMEANS has to have the same dimension (nb of rows) as input X") ;
      }

      if (mxGetN(optarg) != numClusters)
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITMEANS has to have NUMCLUSTERS number of points (columns)") ;
      }

      meansSet = VL_TRUE;
      initMeans = mxGetPr(optarg) ;

      break;
	  }
    case opt_covariances : ;
	  {
      mxClassID classIDsigma = mxGetClassID (optarg) ;
      switch (classIDsigma)
      {
      case mxSINGLE_CLASS:
        if (dataType == VL_TYPE_DOUBLE)
        {
          vlmxError (vlmxErrInvalidArgument, "INITcovariances must be of same data type as X") ;
        }
        break ;
      case mxDOUBLE_CLASS:
        if (dataType == VL_TYPE_FLOAT)
        {
          vlmxError (vlmxErrInvalidArgument, "INITCOVARIANCES must be of same data type as X") ;
        }
        break ;
      default :
        abort() ;
        break ;
      }

      if (! vlmxIsMatrix (optarg, -1, -1) || ! vlmxIsReal (optarg))
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITCOVARIANCES must be a real matrix ") ;
      }

      if (mxGetM(optarg) != dimension)
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITCOVARIANCES has to have the same dimension (nb of rows) as input X") ;
      }

      if (mxGetN(optarg) != numClusters)
      {
        vlmxError(vlmxErrInvalidArgument,
                  "INITCOVARIANCES has to have NUMCLUSTERS number of points (columns)") ;
      }

      covariancesSet = VL_TRUE;
      initCovariances = mxGetPr(optarg) ;

      break;
	  }
    case opt_initialization :
      if (!vlmxIsString (optarg, -1))
      {
        vlmxError (vlmxErrInvalidArgument,
                   "INITLAIZATION must be a string.") ;
      }
      if (mxGetString (optarg, buf, sizeof(buf)))
      {
        vlmxError (vlmxErrInvalidArgument,
                   "INITIALIZATION argument too long.") ;
      }
      if (vlmxCompareStringsI("rand", buf) == 0)
      {
        initialization = VlGMMRand ;
      }
      else if (vlmxCompareStringsI("custom", buf) == 0)
      {
        initialization = VlGMMCustom ;
      }
      else
      {
        vlmxError (vlmxErrInvalidArgument,
                   "Invalid value %s for INITIALISATION.", buf) ;
      }
      break ;

    case opt_num_repetitions :
      if (!vlmxIsPlainScalar (optarg))
      {
        vlmxError (vlmxErrInvalidArgument,
                   "NUMREPETITIONS must be a scalar.") ;
      }
      if (mxGetScalar (optarg) < 1)
      {
        vlmxError (vlmxErrInvalidArgument,
                   "NUMREPETITIONS must be larger than or equal to 1.") ;
      }
      numRepetitions = (vl_size) mxGetScalar (optarg) ;
      break ;
    default :
      abort() ;
      break ;
    }
  }

  /* -----------------------------------------------------------------
   *                                                        Do the job
   * -------------------------------------------------------------- */

  data = mxGetPr(IN(DATA)) ;

  switch(dataType){
  case VL_TYPE_DOUBLE:
        for(i = 0; i < numData*dimension; i++) {
            double datum = *((double*)data + i);
            if(!(datum < VL_INFINITY_D && datum > -VL_INFINITY_D)){
                vlmxError (vlmxErrInvalidArgument,
                   "DATA contains NaNs or Infs.") ;
            }
        }
      break;
  case VL_TYPE_FLOAT:
        for(i = 0; i < numData*dimension; i++) {
            float datum = *((float*)data + i);
            if(!(datum < VL_INFINITY_F && datum > -VL_INFINITY_F)){
                vlmxError (vlmxErrInvalidArgument,
                   "DATA contains NaNs or Infs.") ;
            }
        }
    break;
  default:
    abort();
    break;
  }

  gmm = vl_gmm_new (dataType) ;
  vl_gmm_set_verbosity (gmm, verbosity) ;
  vl_gmm_set_num_repetitions (gmm, numRepetitions) ;
  vl_gmm_set_max_num_iterations (gmm, maxNumIterations) ;
  vl_gmm_set_initialization (gmm, initialization) ;
  vl_gmm_set_sigma_lower_bound (gmm, sigmaLowBound) ;

  if(covariancesSet || meansSet || priorsSet)
  {
    if(vl_gmm_get_initialization(gmm) != VlGMMCustom)
    {
      vlmxWarning (vlmxErrInconsistentData, "Initial covariances, means or priors have been set -> switching to custom initialization.");
    }
    vl_gmm_set_initialization(gmm,VlGMMCustom);
  }

  if(vl_gmm_get_initialization(gmm) == VlGMMCustom)
  {
    if (!covariancesSet || !meansSet || !priorsSet)
    {
      vlmxError (vlmxErrInvalidArgument, "When custom initialization is set, InitMeans, Initcovariances and InitPriors options have to be specified.") ;
    }
    vl_gmm_set_means (gmm,initMeans,numClusters,dimension);
    vl_gmm_set_covariances (gmm,initCovariances,numClusters,dimension);
    vl_gmm_set_priors (gmm,initPriors,numClusters);
  }

  if (verbosity)
  {
    char const * initializationName = 0 ;

    switch (vl_gmm_get_initialization(gmm))
    {
    case VlGMMRand :
      initializationName = "rand" ;
      break ;
    case VlGMMCustom :
      initializationName = "custom" ;
      break ;
    default:
      abort() ;
    }

    mexPrintf("vl_gmm: initialization = %s\n", initializationName) ;
    mexPrintf("vl_gmm: maxNumIterations = %d\n", vl_gmm_get_max_num_iterations(gmm)) ;
    mexPrintf("vl_gmm: numRepetitions = %d\n", vl_gmm_get_num_repetitions(gmm)) ;
    mexPrintf("vl_gmm: dataType = %s\n", vl_get_type_name(vl_gmm_get_data_type(gmm))) ;
    mexPrintf("vl_gmm: dataDimension = %d\n", dimension) ;
    mexPrintf("vl_gmm: num. data points = %d\n", numData) ;
    mexPrintf("vl_gmm: num. centers = %d\n", numClusters) ;
    mexPrintf("vl_gmm: lower bound on sigma = %f\n", vl_gmm_get_sigma_lower_bound(gmm)) ;
    mexPrintf("\n") ;
  }

  /* -------------------------------------------------------------- */
  /*                                                     Clustering */
  /* -------------------------------------------------------------- */

  LL = vl_gmm_cluster(gmm, data, dimension, numData, numClusters) ;

  /* copy centers */
  OUT(MEANS) = mxCreateNumericMatrix (dimension, numClusters, classID, mxREAL) ;
  OUT(COVARIANCES) = mxCreateNumericMatrix (dimension, numClusters, classID, mxREAL) ;
  OUT(PRIORS) = mxCreateNumericMatrix (numClusters, 1, classID, mxREAL) ;
  OUT(POSTERIORS) = mxCreateNumericMatrix (numData, numClusters, classID, mxREAL) ;

  memcpy (mxGetData(OUT(MEANS)),
          vl_gmm_get_means (gmm),
          vl_get_type_size (dataType) * dimension * vl_gmm_get_num_clusters(gmm)) ;

  memcpy (mxGetData(OUT(COVARIANCES)),
          vl_gmm_get_covariances (gmm),
          vl_get_type_size (dataType) * dimension * vl_gmm_get_num_clusters(gmm)) ;

  memcpy (mxGetData(OUT(PRIORS)),
          vl_gmm_get_priors (gmm),
          vl_get_type_size (dataType) * vl_gmm_get_num_clusters(gmm)) ;

  /* optionally return loglikelihood */
  if (nout > 3)
  {
    OUT(LL) = vlmxCreatePlainScalar (LL) ;
  }

  /* optionally return posterior probabilities */
  if (nout > 4)
  {
    memcpy (mxGetData(OUT(POSTERIORS)),
            vl_gmm_get_posteriors (gmm),
            vl_get_type_size (dataType) * numData * vl_gmm_get_num_clusters(gmm)) ;
  }

  vl_gmm_delete (gmm) ;
}
