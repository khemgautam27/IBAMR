// ---------------------------------------------------------------------
//
// Copyright (c) 2018 - 2020 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

/////////////////////////////// INCLUDE GUARD ////////////////////////////////

#ifndef included_IBAMR_AdvDiffConservativeMassTransportQuantityIntegrator
#define included_IBAMR_AdvDiffConservativeMassTransportQuantityIntegrator

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <ibamr/config.h>

#include "ibamr/MassIntegrator.h"
#include "ibamr/StaggeredStokesPhysicalBoundaryHelper.h"
#include "ibamr/ibamr_enums.h"

#include "ibtk/CartGridFunction.h"
#include "ibtk/HierarchyGhostCellInterpolation.h"
#include "ibtk/HierarchyMathOps.h"
#include "ibtk/ibtk_utilities.h"

#include "CoarseFineBoundary.h"
#include "HierarchySideDataOpsReal.h"
#include "IntVector.h"
#include "PatchHierarchy.h"
#include "SideVariable.h"
#include "tbox/Database.h"
#include "tbox/DescribedClass.h"
#include "tbox/Pointer.h"

#include <array>
#include <string>
#include <vector>

namespace SAMRAI
{
namespace solv
{
template <int DIM, class TYPE>
class SAMRAIVectorReal;
template <int DIM>
class RobinBcCoefStrategy;
} // namespace solv
namespace hier
{
template <int DIM>
class BasePatchHierarchy;
} // namespace hier
} // namespace SAMRAI

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class AdvDiffConservativeMassTransportQuantityIntegrator integrates
 * the colocated density field
 *
 *  \f$ \frac{\partial \rho}{\partial t} + \nabla \cdot (\rho u) = S(x,t) \f$
 *
 * and computes the conservative form of the convective operator
 * \f$ \nabla \cdot (\rho C_p u T)\f$.
 *
 * Class AdvDiffConservativeMassTransportQuantityIntegrator computes the convective
 * derivative of a cell-centered temperature field using various bounded-limiters
 * described by Patel and Natarajan.
 *
 * A cell-centered density update is provided by this class, which is used in the
 * conservative discretization of the incompressible energy equation.
 *
 * References
 * Patel, JK. and Natarajan, G., <A HREF="https://www.sciencedirect.com/science/article/pii/S0045793014004009">
 * A generic framework for design of interface capturing schemes for multi-fluid flows</A>
 *
 * \note This class is specialized in that it computes a conservative discretization of the form
 * \f$N = \nabla \cdot (u \rho C_p T)\f$, where the density \f$\rho\f$ can vary in space and time.
 * This operator is to be used in conjuction with the conservative form of the variable coefficient
 * energy equations.
 *
 */
class AdvDiffConservativeMassTransportQuantityIntegrator : public MassIntegrator
{
public:
    /*!
     * \brief Class constructor.
     */
    AdvDiffConservativeMassTransportQuantityIntegrator(std::string object_name,
                                                       SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db);

    /*!
     * \brief Destructor.
     */
    ~AdvDiffConservativeMassTransportQuantityIntegrator();

    /*!
     * \brief Integrate density and momentum field.
     */
    void integrate(double dt) override;

    /*!
     * \name General operator functionality.
     */
    //\{

    /*!
     * \brief Compute hierarchy dependent data required for time integrating variables.
     */
    void
    initializeTimeIntegrator(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > base_hierarchy) override;

    /*!
     * \brief Remove all hierarchy dependent data allocated by
     * initializeTimeIntegrator().
     *
     * \note It is safe to call deallocateTimeIntegrator() when the time integrator
     * is already deallocated.
     *
     * \see initializeTimeIntegrator
     */
    void deallocateTimeIntegrator() override;

    //\}

    /*!
     * \brief Set the current cell-centered density patch data index.
     */
    //    void setCellCenteredDensityPatchDataIndex(int rho_cc_idx);

    /*!
     * \brief Set the current cell-centered specific heat patch data index.
     */
    void setCellCenteredSpecificHeatPatchDataIndex(int cp_cc_idx);

    /*!
     * \brief Set the current cell-centered temperature patch data index.
     */
    void setCellCenteredTemperaturePatchDataIndex(int T_cc_idx);

    /*!
     * \brief Set the patch index to store mass conservation.
     */
    void setMassConservationPatchDataIndex(int M_idx);

    //    /*
    //     * \brief Set the boundary condition object for the side-centered velocity.
    //     */
    //    void setSideCenteredVelocityBoundaryConditions(
    //        const std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*>& u_sc_bc_coefs);

    /*!
     * \brief Set the boundary condition object for the cell-centered density.
     */
    void setCellCenteredDensityBoundaryConditions(SAMRAI::solv::RobinBcCoefStrategy<NDIM>*& rho_cc_bc_coefs);

    /*!
     * \brief Set the boundary condition object for the cell-centered specific heat.
     */
    void setCellCenteredSpecificHeatBoundaryConditions(SAMRAI::solv::RobinBcCoefStrategy<NDIM>*& cp_cc_bc_coefs);

    /*!
     * \brief Set the boundary condition object for the cell-centered temperature.
     */
    void setCellCenteredTemperatureBoundaryConditions(SAMRAI::solv::RobinBcCoefStrategy<NDIM>*& T_cc_bc_coefs);

    /*!
     * \brief Get the newly constructed cell-centered density patch data index.
     *
     * \note This data is produced as a part of the apply() routine and should be used
     * in the linear operator for the INSVC solver.
     */
    int getUpdatedCellCenteredDensityPatchDataIndex();

    /*!
     * \brief Set the patch data indices corresponding to the velocity at the previous time step
     * to be used when computing the density update
     *
     * \note This velocities will be used to compute an approximation to velocities required for SSPRK updates
     * V_old_idx = n-1, V_current_idx = n, V_new_idx = n+1,k (after an INS cycle)
     * If V_old_idx or V_new_idx are not set, then they will degenerate to V_current automatically, for the very first
     * simulation time step and cases where an INS cycle has not been executed, respectively.
     */
    //    void setFluidVelocityPatchDataIndices(int V_old_idx, int V_current_idx, int V_new_idx);

    /*!
     * \brief Set the patch data indices corresponding to the specific heat at the previous time step
     * to be used when computing the density update
     *
     * \note This specific heats will be used to compute an approximation to specific heats required for computing
     * convective derivative. cp_old_idx = n-1, cp_current_idx = n, cp_new_idx = n+1,k (after an INS cycle) If
     * cp_old_idx or cp_new_idx are not set, then they will degenerate to cp_current automatically, for the very first
     * simulation time step and cases where an INS cycle has not been executed, respectively.
     */
    void setSpecificHeatPatchDataIndices(int cp_old_idx, int cp_current_idx, int cp_new_idx);

    /*!
     * \brief Set the patch data indices corresponding to the temperature at the previous time step
     * to be used when computing the computing derivative.
     *
     * \note This specific heats will be used to compute an approximation to temperature required for computing
     * convective derivative. cp_old_idx = n-1, cp_current_idx = n, cp_new_idx = n+1,k (after an INS cycle) If
     * cp_old_idx or cp_new_idx are not set, then they will degenerate to cp_current automatically, for the very first
     * simulation time step and cases where an INS cycle has not been executed, respectively.
     */
    void setTemperaturePatchDataIndices(int T_old_idx, int T_current_idx, int T_new_idx);

private:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    AdvDiffConservativeMassTransportQuantityIntegrator() = delete;

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    AdvDiffConservativeMassTransportQuantityIntegrator(const AdvDiffConservativeMassTransportQuantityIntegrator& from) =
        delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    AdvDiffConservativeMassTransportQuantityIntegrator&
    operator=(const AdvDiffConservativeMassTransportQuantityIntegrator& that) = delete;

    /*!
     * \brief Compute the interpolation of a quantity Q onto Q_half, faces of the cell centered control volumes
     */
    void interpolateCellQuantity(SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > Q_half_data,
                                 SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > U_adv_data,
                                 const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > Q_data,
                                 const SAMRAI::hier::IntVector<NDIM>& patch_lower,
                                 const SAMRAI::hier::IntVector<NDIM>& patch_upper,
                                 const SAMRAI::hier::Box<NDIM>& patch_box,
                                 const LimiterType& convective_limiter);

    /*!
     * \brief Compute div[rho_half*cp_half*u_adv*T_adv]
     */
    void computeConvectiveDerivative(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > N_data,
                                     SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > P_half_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > U_adv_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > R_half_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > T_half_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > C_half_data,
                                     const SAMRAI::hier::Box<NDIM>& patch_box,
                                     const double* const dx);

    /*!
     * \brief Compute the density update rho = a0*rho^0 + a1*rho^1 + a2*dt*(-div[u_adv*rho_half]) + a2*dt*S
     */
    void computeDensityUpdate(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > R_data,
                              const double& a0,
                              const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > R0_data,
                              const double& a1,
                              const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > R1_data,
                              const double& a2,
                              const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > U_adv_data,
                              const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > R_half_data,
                              const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > S_data,
                              const SAMRAI::hier::Box<NDIM>& patch_box,
                              const double& dt,
                              const double* const dx);

    /*!
     * \brief Compute the density update rho = a0*rho^0 + a1*rho^1 + a2*dt*(-div[u_adv*rho_half]) + a2*dt*S
     */
    void
    computeMassConservationMagnitude(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > R_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > Rnew_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM, double> > Rold_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > U_adv_data,
                                     const SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceData<NDIM, double> > R_half_data,
                                     const SAMRAI::hier::Box<NDIM>& patch_box,
                                     const double& dt,
                                     const double* const dx);

    /*!
     * \brief Enforce divergence free condition at the coarse-fine interface to ensure conservation of mass.
     */
    void enforceDivergenceFreeConditionAtCoarseFineInterface(const int U_idx);

    // Book keeping
    std::string d_object_name;

    // Cached communications operators.
    std::string d_temperature_bdry_extrap_type = "CONSTANT", d_specific_heat_bdry_extrap_type = "CONSTANT";

    std::vector<IBTK::HierarchyGhostCellInterpolation::InterpolationTransactionComponent> d_cp_transaction_comps;
    SAMRAI::tbox::Pointer<IBTK::HierarchyGhostCellInterpolation> d_hier_cp_bdry_fill;
    std::vector<IBTK::HierarchyGhostCellInterpolation::InterpolationTransactionComponent> d_T_transaction_comps;
    SAMRAI::tbox::Pointer<IBTK::HierarchyGhostCellInterpolation> d_hier_T_bdry_fill;

    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* d_rho_cc_bc_coefs;
    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* d_cp_cc_bc_coefs;
    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* d_T_cc_bc_coefs;

    // Scratch data.
    //    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM, double> > d_V_var;
    //    int d_V_scratch_idx = IBTK::invalid_index, d_V_old_idx = IBTK::invalid_index, d_V_current_idx =
    //    IBTK::invalid_index,
    //        d_V_new_idx = IBTK::invalid_index, d_V_composite_idx, d_N_idx = IBTK::invalid_index,
    int d_M_idx = IBTK::invalid_index;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_rho_cc_var;
    int d_rho_cc_current_idx = IBTK::invalid_index, d_rho_cc_scratch_idx = IBTK::invalid_index,
        d_rho_cc_new_idx = IBTK::invalid_index;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_cp_cc_var;
    int d_cp_cc_current_idx = IBTK::invalid_index, d_cp_cc_scratch_idx = IBTK::invalid_index,
        d_cp_cc_new_idx = IBTK::invalid_index, d_cp_cc_composite_idx, d_cp_cc_old_idx = IBTK::invalid_index;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_T_cc_var;
    int d_T_cc_current_idx = IBTK::invalid_index, d_T_cc_scratch_idx = IBTK::invalid_index,
        d_T_cc_new_idx = IBTK::invalid_index, d_T_cc_composite_idx, d_T_cc_old_idx = IBTK::invalid_index;

    // The limiter type for interpolation onto faces.
    LimiterType d_temperature_convective_limiter = CUI;
    LimiterType d_specific_heat_convective_limiter = CUI;

    // The required number of ghost cells for the chosen interpolation.
    int d_temperature_limiter_gcw = 1, d_specific_heat_limiter_gcw = 1;

    // Source term variable and function for the mass density update.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_S_var;
    int d_S_scratch_idx = IBTK::invalid_index;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_S_fcn;
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBAMR_AdvDiffConservativeMassTransportQuantityIntegrator