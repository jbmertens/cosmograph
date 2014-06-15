#ifndef COSMO_BSSN
#define COSMO_BSSN

#include "cosmo.h"

namespace cosmo
{

#include "BSSN_macros.h"

typedef struct {

  idx_t i, j, k, idx;

  // generic var for misc. expressions
  real_t trace, expression;

  // trace-free ricci tensor components
  real_t ricciTF11, ricciTF12, ricciTF13, ricciTF22, ricciTF23, ricciTF33;

  // derivatives of \alpha
    // covariant double-derivatives 
    real_t D1D1aTF, D1D2aTF, D1D3aTF, D2D2aTF, D2D3aTF, D3D3aTF;
    real_t DDaTR;
    // normal derivatives of
    real_t d1a, d2a, d3a;

  // derivatives of phi
    // covariant double-derivatives 
    real_t D1D1phi, D1D2phi, D1D3phi, D2D2phi, D2D3phi, D3D3phi;
    // normal derivatives of
    real_t d1phi, d2phi, d3phi;

  // Contravariant (upstairs index) ext. curvature
  real_t Acont11, Acont12, Acont13, Acont22, Acont23, Acont33;

  // Christoffel symbols
  real_t G111, G112, G113, G122, G123, G133,
         G211, G212, G213, G222, G223, G233,
         G311, G312, G313, G322, G323, G333;

  // derivatives of the metric, d_i g_jk
  real_t d1g11, d1g12, d1g13, d1g22, d1g23, d1g33,
         d2g11, d2g12, d2g13, d2g22, d2g23, d2g33,
         d3g11, d3g12, d3g13, d3g22, d3g23, d3g33;

  // derivatives of the inverse metric d_i g^jk
  real_t d1gi11, d1gi12, d1gi13, d1gi22, d1gi23, d1gi33,
         d2gi11, d2gi12, d2gi13, d2gi22, d2gi23, d2gi33,
         d3gi11, d3gi12, d3gi13, d3gi22, d3gi23, d3gi33;

  // second derivatives of the metric d_i d_j g_kl
  real_t d1d1g11, d1d1g12, d1d1g13, d1d1g22, d1d1g23, d1d1g33,
         d1d2g11, d1d2g12, d1d2g13, d1d2g22, d1d2g23, d1d2g33,
         d1d3g11, d1d3g12, d1d3g13, d1d3g22, d1d3g23, d1d3g33,
         d2d2g11, d2d2g12, d2d2g13, d2d2g22, d2d2g23, d2d2g33,
         d2d3g11, d2d3g12, d2d3g13, d2d3g22, d2d3g23, d2d3g33,
         d3d3g11, d3d3g12, d3d3g13, d3d3g22, d3d3g23, d3d3g33;

  // local copies of current field values
  BSSN_APPLY_TO_FIELDS(DECLARE_REAL_T)

  // local copies of adjacent current field values for fast derivatives
  BSSN_APPLY_TO_FIELDS(DECLARE_ADJACENT_REAL_T)

} PointData;


/** BSSN class **/
class BSSN
{
  /* arrays for storing fields */
  BSSN_APPLY_TO_FIELDS(RK4_ARRAY_CREATE)
  /* local values */
  PointData paq;

public:
  std::map <std::string, real_t *> fields;

  BSSN();
  ~BSSN();

  inline real_t der(real_t field_adj[3][3][3], int d)
  {
    switch (d) {
      case 1:
        return field_adj[2][1][1] - field_adj[0][1][1];
        break;
      case 2:
        return field_adj[1][2][1] - field_adj[1][0][1];
        break;
      case 3:
        return field_adj[1][1][2] - field_adj[1][1][0];
        break;
    }

    /* XXX */
    return 0;
  }

  inline real_t dder(real_t field_adj[3][3][3], int d1, int d2)
  {
    switch (d1) {
      case 1:
        switch (d2) {
          case 1:
            return field_adj[0][1][1] + field_adj[2][1][1] - 2.0*field_adj[1][1][1];
            break;
          case 2:
            return field_adj[0][0][1] + field_adj[2][2][1] - field_adj[0][2][1] - field_adj[2][0][1];
            break;
          case 3:
            return field_adj[0][1][0] + field_adj[2][1][2] - field_adj[0][1][2] - field_adj[2][1][0];
            break;
        }
        break;
      case 2:
        switch (d2) {
          case 1:
            return field_adj[0][0][1] + field_adj[2][2][1] - field_adj[0][2][1] - field_adj[2][0][1];
            break;
          case 2:
            return field_adj[1][0][1] + field_adj[1][2][1] - 2.0*field_adj[1][1][1];
            break;
          case 3:
            return field_adj[1][0][0] + field_adj[1][2][2] - field_adj[1][0][2] - field_adj[1][2][0];
            break;
        }
        break;
      case 3:
        switch (d2) {
          case 1:
            return field_adj[0][1][0] + field_adj[2][1][2] - field_adj[0][1][2] - field_adj[2][1][0];
            break;
          case 2:
            return field_adj[1][0][0] + field_adj[1][2][2] - field_adj[1][0][2] - field_adj[1][2][0];
            break;
          case 3:
            return field_adj[1][1][0] + field_adj[1][1][2] - 2.0*field_adj[1][1][1];
            break;
        }
        break;
    }

    /* XXX */
    return 0.0;
  }
  

  /* set current local field values */
  inline void set_local_vals(PointData *paq)
  {
    // Need full mixed 2nd derivatives for all of these
    SET_LOCAL_VALUES_PFE(gamma11);
    SET_LOCAL_VALUES_PFE(gamma12);
    SET_LOCAL_VALUES_PFE(gamma13);
    SET_LOCAL_VALUES_PFE(gamma22);
    SET_LOCAL_VALUES_PFE(gamma23);
    SET_LOCAL_VALUES_PFE(gamma33);
    SET_LOCAL_VALUES_PFE(phi);
    SET_LOCAL_VALUES_PFE(K);

    SET_LOCAL_VALUES_PFE(beta1);
    SET_LOCAL_VALUES_PFE(beta2);
    SET_LOCAL_VALUES_PFE(beta3);
    
    SET_LOCAL_VALUES_PFE(alpha);

    // It might be faster to compute these at each step,
    //  rather than pulling them out of memory?
    // only first-derivatives here, at least.
    SET_LOCAL_VALUES_PF(gammai11);
    SET_LOCAL_VALUES_PF(gammai12);
    SET_LOCAL_VALUES_PF(gammai13);
    SET_LOCAL_VALUES_PF(gammai22);
    SET_LOCAL_VALUES_PF(gammai23);
    SET_LOCAL_VALUES_PF(gammai33);

    // adjacent values of these aren't needed (no derivatives taken)
    SET_LOCAL_VALUES_P(Gamma1);
    SET_LOCAL_VALUES_P(Gamma2);
    SET_LOCAL_VALUES_P(Gamma3);
    SET_LOCAL_VALUES_P(A11);
    SET_LOCAL_VALUES_P(A12);
    SET_LOCAL_VALUES_P(A13);
    SET_LOCAL_VALUES_P(A22);
    SET_LOCAL_VALUES_P(A23);
    SET_LOCAL_VALUES_P(A33);

  }

  inline void calculate_Acont(PointData *paq)
  {
    BSSN_APPLY_TO_IJ_PERMS(BSSN_CALCULATE_ACONT)
  }

  /* Calculate metric derivatives */
  inline void calculate_dgamma(PointData *paq)
  {
    BSSN_APPLY_TO_IJK_PERMS(BSSN_CALCULATE_DGAMMA)
  }

  inline void calculate_ddgamma(PointData *paq)
  {
    BSSN_APPLY_TO_IJ_PERMS(BSSN_CALCULATE_DIDJGAMMA_PERMS)
  }

  /* Calculate metric derivatives */
  inline void calculate_dgammai(PointData *paq)
  {
    BSSN_APPLY_TO_IJK_PERMS(BSSN_CALCULATE_DGAMMAI);
  }

  inline void calculate_christoffels(PointData *paq)
  {
    // christoffel symbols: \Gamma^i_{jk} = Gijk
    BSSN_APPLY_TO_IJK_PERMS(BSSN_CALCULATE_CHRISTOFFEL)
  }

  inline void calculate_dalpha_dphi(PointData *paq)
  {
    // normal derivatives of phi
    paq->d1phi = der(paq->phi_adj, 1);
    paq->d2phi = der(paq->phi_adj, 2);
    paq->d3phi = der(paq->phi_adj, 3);

    // normal derivatives of alpha
    paq->d1a = der(paq->alpha_adj, 1);
    paq->d2a = der(paq->alpha_adj, 2);
    paq->d3a = der(paq->alpha_adj, 3);
  }

  /* Calculate trace-free ricci tensor components */
  void calculateRicciTF(PointData *paq);
  void calculateDDphi(PointData *paq);
  void calculateDDalphaTF(PointData *paq);

  real_t ev_gamma11(PointData *paq);
  real_t ev_gamma12(PointData *paq);
  real_t ev_gamma13(PointData *paq);
  real_t ev_gamma22(PointData *paq);
  real_t ev_gamma23(PointData *paq);
  real_t ev_gamma33(PointData *paq);

  real_t ev_A11(PointData *paq);
  real_t ev_A12(PointData *paq);
  real_t ev_A13(PointData *paq);
  real_t ev_A22(PointData *paq);
  real_t ev_A23(PointData *paq);
  real_t ev_A33(PointData *paq);
  real_t ev_K(PointData *paq);
  real_t ev_phi(PointData *paq);
  real_t ev_Gamma1(PointData *paq);
  real_t ev_Gamma2(PointData *paq);
  real_t ev_Gamma3(PointData *paq);

  real_t ev_alpha(PointData *paq);
  real_t ev_beta1(PointData *paq);
  real_t ev_beta2(PointData *paq);
  real_t ev_beta3(PointData *paq);

  void set_paq_values(idx_t i, idx_t j, idx_t k, PointData *paq);

  void step();
  void init();

};



}

#endif
