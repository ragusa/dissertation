/*************************************************************************/
/* File   : eigen_systems.h                                              */
/* Original Authors : J. Stone, T. Gardiner and P. Teuben                */
/* Author : A. Kercher                                                   */
/*-----------------------------------------------------------------------*/
/* References:                                                           */
/*   [1] P. Cargo & G. Gallice, "Roe matrices for ideal MHD and          */
/*       systematic construction of Roe matrices for systems of          */
/*       conservation laws", JCP, 136, 446 (1997).                       */
/*   [2] J. Stone, & T. Gardiner, "A simple unsplit Godunov              */
/*       method for multidimensional MHD", New Astronomy 14,             */
/*       (2009), 139-148.                                                */
/*   [3] J. Stone, T. Gardiner, P. Teuben, J. Hawley, & J. Simon         */
/*      "Athena: A new code for astrophysical MHD", ApJS, (2008)         */
/*   [4] Athena Code Project                                             */
/*                url: https://trac.princeton.edu/Athena/                */
/*-----------------------------------------------------------------------*/
/*                                                                       */
/*  Acknowledgements: This is a reimplementation of the file             */
/*  "esystem_roe.c" included [4].  This is not original work.            */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

/* Prototypes */
__host__ __device__ void get_eigen_system_hd (const Real gamma, const Real vx, const Real vy, const Real vz, 
					      const Real h,Real ev[5], Real rem[5][5]);
__host__ __device__ void get_eigen_system_mhd (const Real gamma, const Real d, const Real vx, const Real vy, 
					       const Real vz, const Real h, const Real bx, const Real by,
					       const Real bz, const Real xfac, const Real yfac, 
					       Real ev[7], Real lem[7][7], Real rem[7][7]);

/*****************************************************/
/* Eigenvector and right eigenmatrix for adiabatic   */
/* hydrodynamics.                                    */
/*                                                   */
/*---------------------------------------------------*/
/*  Input :                                          */
/*  Output :                                         */
/*****************************************************/

__host__ __device__
void get_eigen_system_hd (const Real gamma, const Real vx, const Real vy, const Real vz, const Real h,
			  Real ev[5], Real rem[5][5])			  	     
{

  Real ke = half*(vx*vx + vy*vy + vz*vz);  
  Real c0 = (gamma - One)*fmax(h - ke,MINIMUM_NUM);

  ev[0] = vx - c0;
  ev[1] = vx;
  ev[2] = vx;
  ev[3] = vx;
  ev[4] = vx + c0;

  /* right eigenvectors, stored as COLUMNS eq. B3 of [3] */
  rem[0][0] = One;
  rem[0][1] = Zero;
  rem[0][2] = Zero;
  rem[0][3] = One;
  rem[0][4] = One;
  
  rem[1][0] = vx - c0;
  rem[1][1] = Zero;
  rem[1][2] = Zero;
  rem[1][3] = vx;
  rem[1][4] = vx + c0;
  
  rem[2][0] = vy;
  rem[2][1] = One;
  rem[2][2] = Zero;
  rem[2][3] = vy;
  rem[2][4] = vy;
  
  rem[3][0] = vz;
  rem[3][1] = Zero;
  rem[3][2] = One;
  rem[3][3] = vz;
  rem[3][4] = vz;
  
  rem[4][0] = h - vx*c0;
  rem[4][1] = vy;
  rem[4][2] = vz;
  rem[4][3] = ke;
  rem[4][4] = h + vx*c0;    

}

/*****************************************************/
/* Eigenvector and eigenmatrix of left/right         */
/* eigenvectors using Roe averages for adiabatic     */
/* magnetohydrodynamics.                             */
/*                                                   */
/*---------------------------------------------------*/
/*  Input :                                          */
/*  Output :                                         */
/*****************************************************/
__host__ __device__
void get_eigen_system_mhd (const Real gamma, const Real d, const Real vx, const Real vy, const Real vz, 
			   const Real h, const Real bx, const Real by, const Real bz,
			   const Real xfac, const Real yfac,Real ev[7], Real lem[][7], Real rem[][7])
			   
			   
{

  Real d_inv = One/d;
  Real ke = half*(vx*vx + vy*vy + vz*vz);  
  Real c0 = (gamma - One)*fmax(h - ke,MINIMUM_NUM);

  // fast and slow wave speeds
  Real btsq = by*by + bz*bz;
  Real bt = std::sqrt(btsq);
  Real casq = bx*bx*d_inv;	
  Real ctsq = (by*by + bz*bz)*d_inv;	
  Real hp = h - (casq + ctsq);
  Real twid_asq = fmax(Zero,((gamma - One)*(hp - ke) - (gamma - Two)*xfac));
  Real ws_sum = casq + ctsq + twid_asq;
  Real ws_dif = casq + ctsq - twid_asq;
  
  Real cfsq_cssq = std::sqrt(ws_dif*ws_dif + Real(4.0)*twid_asq*ctsq);
  Real cfsq = 0.5*(ws_sum + cfsq_cssq);
  Real cssq = twid_asq*casq/cfsq;
  Real cf = std::sqrt(cfsq);
  Real cs = std::sqrt(cssq);
  Real ca = std::sqrt(casq);

  Real btstarsq = (gamma - One - (gamma - Two)*yfac)*btsq;
  Real btstar = std::sqrt(btstarsq);
  
  Real beta_y = One;
  Real beta_z = Zero;
  // bt is positive
  if(bt > Zero){
    beta_y = by/bt;
    beta_z = bz/bt;
  }
  
  Real betast_y = beta_y/std::sqrt(gamma - One - (gamma - Two)*yfac);
  Real betast_z = beta_z/std::sqrt(gamma - One - (gamma - Two)*yfac);
  
  Real betast_sq = betast_y*betast_y + betast_z*betast_z;
  Real vbeta = vy*betast_y + vz*betast_z;

  Real alpha_f, alpha_s;
  
  if((cfsq - cssq) == Zero){
    alpha_f = One;
    alpha_s = Zero;
  }
  else if ((twid_asq - cssq) <= Zero){
    alpha_f = Zero;
    alpha_s = One;
  }
  else if ((cfsq - twid_asq) <= Zero){
    alpha_f = One;
    alpha_s = Zero;
  }
  else{
      alpha_f = std::sqrt((twid_asq - cssq)/(cfsq - cssq));
      alpha_s = std::sqrt((cfsq - twid_asq)/(cfsq - cssq));
  }
  
  ev[0] = vx - cf;
  ev[1] = vx - ca;
  ev[2] = vx - cs;
  ev[3] = vx;
  ev[4] = vx + cs;
  ev[5] = vx + ca;
  ev[6] = vx + cf; 
  
  Real sgn_bn = One;
  if (fabs(bx) > Zero) sgn_bn = bx/fabs(bx);
  
  Real sqrtd = std::sqrt(d);
  Real sqrtd_inv = One/sqrtd;
  Real twid_a = std::sqrt(twid_asq);
  Real qf = cf*alpha_f*sgn_bn;
  Real qs = cs*alpha_s*sgn_bn;
  Real af_prime = twid_a*alpha_f*sqrtd_inv;
  Real as_prime = twid_a*alpha_s*sqrtd_inv;
  Real afpbb = af_prime*btstar*betast_sq;
  Real aspbb = as_prime*btstar*betast_sq;
 
  /* right eigenvectors, stored as COLUMNS eq. B21 of [1] */
  /* right eigenvectors are grouped as rows for efficient memory access. */
  rem[0][0] = alpha_f;
  rem[0][1] = Zero;
  rem[0][2] = alpha_s;
  rem[0][3] = One;
  rem[0][4] = alpha_s;
  rem[0][5] = Zero;
  rem[0][6] = alpha_f;
  
  rem[1][0] = alpha_f*ev[0];
  rem[1][1] = Zero;
  rem[1][2] = alpha_s*ev[2];
  rem[1][3] = vx;
  rem[1][4] = alpha_s*ev[4];
  rem[1][5] = Zero;
  rem[1][6] = alpha_f*ev[6];
  
  Real qa = alpha_f*vy;
  Real qb = alpha_s*vy;
  Real qc = qs*betast_y;
  Real qd = qf*betast_y;
  rem[2][0] = qa + qc;
  rem[2][1] = -beta_z;
  rem[2][2] = qb - qd;
  rem[2][3] = vy;
  rem[2][4] = qb + qd;
  rem[2][5] = beta_z;
  rem[2][6] = qa - qc;
  
  qa = alpha_f*vz;
  qb = alpha_s*vz;
  qc = qs*betast_z;
  qd = qf*betast_z;
  rem[3][0] = qa + qc;
  rem[3][1] = beta_y;
  rem[3][2] = qb - qd;
  rem[3][3] = vz;
  rem[3][4] = qb + qd;
  rem[3][5] = -beta_y;
  rem[3][6] = qa - qc;
  
  rem[4][0] = alpha_f*(hp - vx*cf) + qs*vbeta + aspbb;
  rem[4][1] = -(vy*beta_z - vz*beta_y);
  rem[4][2] = alpha_s*(hp - vx*cs) - qf*vbeta - afpbb;
  rem[4][3] = ke + (gamma - Two)*xfac/(gamma - One);
  rem[4][4] = alpha_s*(hp + vx*cs) + qf*vbeta - afpbb;
  rem[4][5] = -rem[4][1];
  rem[4][6] = alpha_f*(hp + vx*cf) - qs*vbeta + aspbb;
  
  rem[5][0] = as_prime*betast_y;
  rem[5][1] = -beta_z*sgn_bn*sqrtd_inv;
  rem[5][2] = -af_prime*betast_y;
  rem[5][3] = Zero;
  rem[5][4] = rem[5][2];
  rem[5][5] = rem[5][1];
  rem[5][6] = rem[5][0];
  
  rem[6][0] = as_prime*betast_z;
  rem[6][1] = beta_y*sgn_bn*sqrtd_inv;
  rem[6][2] = -af_prime*betast_z;
  rem[6][3] = Zero;
  rem[6][4] = rem[6][2];
  rem[6][5] = rem[6][1];
  rem[6][6] = rem[6][0];

  /* left eigenvectors, stored as ROWS eq. B29 of [1] */
  /* normalized by 1/(2*c0^2)                         */

  Real norm = half/twid_asq;
  Real cff = norm*alpha_f*cf;
  Real css = norm*alpha_s*cs;
  qf *= norm;
  qs *= norm;
  Real af  = norm*af_prime*d;
  Real as  = norm*as_prime*d;
  Real afpb = norm*af_prime*btstar;
  Real aspb = norm*as_prime*btstar;

  /* normalize by (gamma - 1)/(2*c02) */
  norm *= (gamma - One);
  alpha_f *= norm;
  alpha_s *= norm;
  Real qst_y = betast_y/betast_sq;
  Real qst_z = betast_z/betast_sq;
  Real vqstr = (vy*qst_y + vz*qst_z);
  norm *= Two;

  lem[0][0] = alpha_f*(Two*ke - hp) + cff*(cf + vx) - qs*vqstr - aspb;
  lem[0][1] = -alpha_f*vx - cff;
  lem[0][2] = -alpha_f*vy + qs*qst_y;
  lem[0][3] = -alpha_f*vz + qs*qst_z;
  lem[0][4] = alpha_f;
  lem[0][5] = as*qst_y - alpha_f*by;
  lem[0][6] = as*qst_z - alpha_f*bz;

  lem[1][0] = half*(vy*beta_z - vz*beta_y);
  lem[1][1] = Zero;
  lem[1][2] = -0.5*beta_z;
  lem[1][3] = half*beta_y;
  lem[1][4] = Zero;
  lem[1][5] = -half*sqrtd*beta_z*sgn_bn;
  lem[1][6] = half*sqrtd*beta_y*sgn_bn;

  lem[2][0] = alpha_s*(Two*ke - hp) + css*(cs + vx) + qf*vqstr + afpb;
  lem[2][1] = -alpha_s*vx - css;
  lem[2][2] = -alpha_s*vy - qf*qst_y;
  lem[2][3] = -alpha_s*vz - qf*qst_z;
  lem[2][4] = alpha_s;
  lem[2][5] = -af*qst_y - alpha_s*by;
  lem[2][6] = -af*qst_z - alpha_s*bz;

  lem[3][0] = 1.0 - norm*(ke - (gamma - Two)*xfac/(gamma - One)); 
  lem[3][1] = norm*vx;
  lem[3][2] = norm*vy;
  lem[3][3] = norm*vz;
  lem[3][4] = -norm;
  lem[3][5] = norm*by;
  lem[3][6] = norm*bz;

  lem[4][0] = alpha_s*(Two*ke - hp) + css*(cs - vx) - qf*vqstr + afpb;
  lem[4][1] = -alpha_s*vx + css;
  lem[4][2] = -alpha_s*vy + qf*qst_y;
  lem[4][3] = -alpha_s*vz + qf*qst_z;
  lem[4][4] = alpha_s;
  lem[4][5] = lem[2][5];
  lem[4][6] = lem[2][6];

  lem[5][0] = -lem[1][0];
  lem[5][1] = Zero;
  lem[5][2] = -lem[1][2];
  lem[5][3] = -lem[1][3];
  lem[5][4] = Zero;
  lem[5][5] = lem[1][5];
  lem[5][6] = lem[1][6];

  lem[6][0] = alpha_f*(Two*ke - hp) + cff*(cf - vx) + qs*vqstr - aspb;
  lem[6][1] = -alpha_f*vx + cff;
  lem[6][2] = -alpha_f*vy - qs*qst_y;
  lem[6][3] = -alpha_f*vz - qs*qst_z;
  lem[6][4] = alpha_f;
  lem[6][5] = lem[0][5];
  lem[6][6] = lem[0][6];
}

