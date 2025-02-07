// ---------------------------------------------------------------------
//
// Copyright (c) 2018 - 2022 by the IBAMR developers
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

#ifndef included_IBAMR_IBRedundantInitializer
#define included_IBAMR_IBRedundantInitializer

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <ibamr/config.h>

#include "ibamr/IBRodForceSpec.h"

#include "ibtk/LInitStrategy.h"
#include "ibtk/LSiloDataWriter.h"
#include "ibtk/ibtk_utilities.h"

#include "IntVector.h"
#include "tbox/Pointer.h"

#include <array>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace IBTK
{
class LData;
class LDataManager;
class Streamable;
} // namespace IBTK
namespace SAMRAI
{
namespace hier
{
template <int DIM>
class Patch;
template <int DIM>
class PatchHierarchy;
} // namespace hier
namespace tbox
{
class Database;
} // namespace tbox
} // namespace SAMRAI

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBRedundantInitializer is an abstract LInitStrategy that
 * initializes the configuration of one or more Lagrangian structures from input
 * files.
 *
 * \todo Document input database entries.
 *
 */
class IBRedundantInitializer : public IBTK::LInitStrategy
{
public:
    /*!
     * \brief Constructor.
     */
    IBRedundantInitializer(std::string object_name, SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db);

    /*!
     * \brief Destructor.
     */
    virtual ~IBRedundantInitializer();

    /*!
     * \brief Register a Silo data writer with the IB initializer object.
     */
    void registerLSiloDataWriter(SAMRAI::tbox::Pointer<IBTK::LSiloDataWriter> silo_writer);

    /*!
     * \brief Determine whether there are any Lagrangian nodes on the specified
     * patch level.
     *
     * \return A boolean value indicating whether Lagrangian data is associated
     * with the given level in the patch hierarchy.
     */
    bool getLevelHasLagrangianData(int level_number, bool can_be_refined) const override;

    /*!
     * \return A boolean value indicating whether or not all Lagrangian data is
     * within the computational domain specified by the patch hierarchy.
     */
    bool
    getIsAllLagrangianDataInDomain(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy) const override;

    /*!
     * \brief Determine the number of global nodes on the specified patch level.
     *
     * \return The number of global nodes on the specified level.
     */
    unsigned int
    computeGlobalNodeCountOnPatchLevel(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                       int level_number,
                                       double init_data_time,
                                       bool can_be_refined,
                                       bool initial_time) override;

    /*!
     * \brief Determine the number of local nodes on the specified patch level.
     *
     * \return The number of local nodes on the specified level.
     */
    unsigned int computeLocalNodeCountOnPatchLevel(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                                   int level_number,
                                                   double init_data_time,
                                                   bool can_be_refined,
                                                   bool initial_time) override;

    /*!
     * Typedef specifying the interface for initializing structures on a given level.
     */
    using InitStructureOnLevel = void (*)(const unsigned int& strct_num,
                                          const int& level_num,
                                          int& num_vertices,
                                          std::vector<IBTK::Point>& vertex_posn,
                                          void* ctx);

    /*!
     * Register the function to initialize a structure on a given level.
     *
     * \note A function must be registered or IBRedundantInitializer will
     * return an error.
     */
    void registerInitStructureFunction(InitStructureOnLevel fcn, void* ctx = nullptr);

    /*
     * Edge data structures.
     */
    using Edge = std::pair<int, int>;
    struct EdgeComp
    {
        inline bool operator()(const Edge& e1, const Edge& e2) const
        {
            return (e1.first < e2.first) || (e1.first == e2.first && e1.second < e2.second);
        }
    };

    /*
     * Struct for spring specifications.
     * Spring parameters should be structured as:
     * parameters[0] is the spring constant.
     * parameters[1] is the resting length.
     * force_fcn_idx is the spring function index.
     */
    struct SpringSpec
    {
        std::vector<double> parameters;
        int force_fcn_idx = 0;
    };

    /*!
     * Typedef specifying the interface for initializing springs on a given level.
     *
     * spring_map should contain the map between the master point and the Edge of the spring.
     * spring_spec should bw the map between the Edge and the SpringSpec.
     */
    using InitSpringDataOnLevel = void (*)(const unsigned int& strct_num,
                                           const int& level_num,
                                           std::multimap<int, Edge>& spring_map,
                                           std::map<Edge, SpringSpec, EdgeComp>& spring_spec,
                                           void* ctx);

    /*!
     * \brief Register a function to initialize spring data structures on a given level.
     */
    void registerInitSpringDataFunction(InitSpringDataOnLevel fcn, void* ctx = nullptr);

    /*
     * Struct for xspring specifications.
     * XSpring parameters should be structured as:
     * parameters[0] is the spring constant.
     * parameters[1] is the resting length.
     * force_fcn_idx is the spring function index.
     */
    struct XSpringSpec
    {
        std::vector<double> parameters;
        int force_fcn_idx = 0;
    };

    /*!
     * Typedef specifying the interface for initializing xsprings on a given level.
     *
     * xspring_map should contain the map between the master point and the Edge of the spring.
     * xspring_spec should be the map between the Edge and the XSpringSpec.
     */
    using InitXSpringDataOnLevel = void (*)(const unsigned int& strct_num,
                                            const int& level_num,
                                            std::multimap<int, Edge>& xspring_map,
                                            std::map<Edge, XSpringSpec, EdgeComp> xspring_spec,
                                            void* ctx);

    /*!
     * \brief Register a function to initialize xspring data structures on a given level.
     */
    void registerInitXSpringDataFunction(InitXSpringDataOnLevel fcn, void* ctx = nullptr);

    /*
     * Struct for beam specifications.
     * Beam parameters should be structured as:
     * neighbor_idxs should be neighboring vertex indices.
     * bend_rigidity should be bending rigidity.
     * curvature is a vector of the curvature of the rod.
     */
    struct BeamSpec
    {
        std::pair<int, int> neighbor_idxs = { 0, 0 };
        double bend_rigidity = -1.0;
        IBTK::Vector curvature = IBTK::Vector::Zero();
    };

    /*!
     * Typdef specifying the interface for initializing beams on a given level.
     *
     * beam_spec should be the map between the master index and the corresponding BeamSpec.
     */
    using InitBeamDataOnLevel = void (*)(const unsigned int& strct_num,
                                         const int& level_num,
                                         std::multimap<int, BeamSpec>& beam_spec,
                                         void* ctx);

    /*!
     * \brief Register a function to initialize beam data structures on a given level.
     */
    void registerInitBeamDataFunction(InitBeamDataOnLevel fcn, void* ctx = nullptr);

    /*!
     * Struct for rod specifications.
     * Rod parameters should be structured as:
     * [ds, a1, a2, a3, b1, b2, b3, kappa1, kappa2, tau]
     */
    struct RodSpec
    {
        std::array<double, IBRodForceSpec::NUM_MATERIAL_PARAMS> properties;
    };

    /*!
     * Typedef specifying the interface for initializing rods and director vectors on a given level.
     *
     * director_spec should be a vector of the initial orthonormal director vectors.
     * rod_edge_map should contain the map between the master point and the Edge of the rod.
     * rod_spec should be the map between the Edge and the RodSpec.
     */
    using InitDirectorAndRodOnLevel = void (*)(const unsigned int& strct_num,
                                               const int& level_num,
                                               std::vector<std::vector<double> >& director_spec,
                                               std::multimap<int, Edge>& rod_edge_map,
                                               std::map<Edge, RodSpec, EdgeComp>& rod_spec,
                                               void* ctx);

    /*!
     * \brief Register a funcion to initialize director and rod data structures on a given level.
     */
    void registerInitDirectorAndRodFunction(InitDirectorAndRodOnLevel fcn, void* ctx = nullptr);

    /*
     * Struct for massive point specifications.
     * bdry_mass should be the mass of the point.
     * stiffness is the penalty spring constant.
     */
    struct BdryMassSpec
    {
        double bdry_mass, stiffness;
    };

    /*!
     * Typedef specifying the interface for initializing massive points on a given level.
     *
     * bdry_mass_spec should be a map between indices and the corresponding BdryMassSpec.
     */
    using InitBoundaryMassOnLevel = void (*)(const unsigned int& strct_num,
                                             const int& level_num,
                                             std::multimap<int, BdryMassSpec>& bdry_mass_spec,
                                             void* ctx);

    /*!
     * \brief Register a function to initialize massive points on a given level.
     */
    void registerInitBoundaryMassFunction(InitBoundaryMassOnLevel fcn, void* ctx = nullptr);

    /*!
     * Struct for target point specifications.
     *
     * stiffness should be the penalty spring constant.
     * damping should be the penalty damping coefficient.
     */
    struct TargetSpec
    {
        double stiffness, damping;
    };

    /*!
     * Typedef specifying the interface for initializing target points on a given level.
     *
     * tg_pt_spec should be a map between indices and the corresponding TargetSpec.
     */

    using InitTargetPtOnLevel = void (*)(const unsigned int& strct_num,
                                         const int& level_num,
                                         std::multimap<int, TargetSpec>& tg_pt_spec,
                                         void* ctx);

    /*!
     * \brief Register a function to initialize target points on a given level.
     */
    void registerInitTargetPtFunction(InitTargetPtOnLevel fcn, void* ctx = nullptr);

    /*!
     * Struct for anchor point specifications.
     *
     * is_anchor_point should be TRUE for points that are anchor points.
     */
    struct AnchorSpec
    {
        bool is_anchor_point;
    };

    /*!
     * Typedef specifying the interface for initializing anchor points on a given level.
     *
     * anchor_pt_spec should be a map between indices and the corresponding AnchorSpec.
     */
    using InitAnchorPtOnLevel = void (*)(const unsigned int& strct_num,
                                         const int& level_num,
                                         std::multimap<int, AnchorSpec>& anchor_pt_spec,
                                         void* ctx);

    /*!
     * \brief Register a function to initialize anchor points on a given level.
     */
    void registerInitAnchorPtFunction(InitAnchorPtOnLevel fcn, void* ctx = nullptr);

    /*!
     * Typedef specifying the interface for initializing flow meters and pressure gauges on a given level.
     *
     * instrument_name should be the list of names of the instruments. Note that instrument names and indices are global
     * over all levels and structures. instrument_spec should be the map between the master index of the instrument and
     * a pair of instrument number and node index. Note that this map is ordered.
     */
    using InitInstrumentationOnLevel = void (*)(const unsigned int& strct_num,
                                                const int& level_num,
                                                std::vector<std::string>& instrument_name,
                                                std::map<int, std::pair<int, int> >& instrument_spec,
                                                void* ctx);

    /*!
     * \brief Register a function to initialize instrumentation data on a given level.
     */
    void registerInitInstrumentationFunction(InitInstrumentationOnLevel fcn, void* ctx = nullptr);

    /*
     * Typedef specifying the interface for initializing source and sink data on a given level.
     *
     * source_spec should be the map between vertices and source/sink index. The location of the source/sink is the
     * arithmetic mean of the positions of the nodes. source_names should be the list of source/sink names. Note that
     * the instrument names and indices are global over all levels and structures. source_radii should be the
     * source/sink radii.
     */
    using InitSourceOnLevel = void (*)(const unsigned int& strct_num,
                                       const int& level_num,
                                       std::map<int, int>& source_spec,
                                       std::vector<std::string>& source_names,
                                       std::vector<double>& source_radii,
                                       void* ctx);
    /*!
     * \brief Register a funciton to initialize source/sink data on a given level.
     */
    void registerInitSourceFunction(InitSourceOnLevel fcn, void* ctx = nullptr);

    /*!
     * \brief Initialize the structure indexing information on the patch level.
     */
    void initializeStructureIndexingOnPatchLevel(std::map<int, std::string>& strct_id_to_strct_name_map,
                                                 std::map<int, std::pair<int, int> >& strct_id_to_lag_idx_range_map,
                                                 int level_number,
                                                 double init_data_time,
                                                 bool can_be_refined,
                                                 bool initial_time,
                                                 IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Initialize the LNode and LData data needed to specify the
     * configuration of the curvilinear mesh on the patch level.
     *
     * \return The number of local nodes initialized on the patch level.
     */
    unsigned int initializeDataOnPatchLevel(int lag_node_index_idx,
                                            unsigned int global_index_offset,
                                            unsigned int local_index_offset,
                                            SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                            SAMRAI::tbox::Pointer<IBTK::LData> U_data,
                                            SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                            int level_number,
                                            double init_data_time,
                                            bool can_be_refined,
                                            bool initial_time,
                                            IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Initialize the LData needed to specify the mass and spring
     * constant data required by the penalty IB method.
     *
     * \return The number of local nodes initialized on the patch level.
     */
    unsigned int initializeMassDataOnPatchLevel(unsigned int global_index_offset,
                                                unsigned int local_index_offset,
                                                SAMRAI::tbox::Pointer<IBTK::LData> M_data,
                                                SAMRAI::tbox::Pointer<IBTK::LData> K_data,
                                                SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                                int level_number,
                                                double init_data_time,
                                                bool can_be_refined,
                                                bool initial_time,
                                                IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Initialize the LNode data needed to specify director vectors
     * required by some material models.
     *
     * \return The number of local nodes initialized on the patch level.
     */
    unsigned int
    initializeDirectorDataOnPatchLevel(unsigned int global_index_offset,
                                       unsigned int local_index_offset,
                                       SAMRAI::tbox::Pointer<IBTK::LData> D_data,
                                       SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                       int level_number,
                                       double init_data_time,
                                       bool can_be_refined,
                                       bool initial_time,
                                       IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Tag cells for initial refinement.
     *
     * When the patch hierarchy is being constructed at the initial simulation
     * time, it is necessary to instruct the gridding algorithm where to place
     * local refinement in order to accommodate portions of the curvilinear mesh
     * that will reside in any yet-to-be-constructed level(s) of the patch
     * hierarchy.
     */
    void tagCellsForInitialRefinement(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                      int level_number,
                                      double error_data_time,
                                      int tag_index) override;

    /*!
     * \brief Set the names of the structures on a given level.
     *
     * \note The structure will be initialized in the same order as the supplied vector.
     */
    void setStructureNamesOnLevel(const int& level_num, const std::vector<std::string>& strct_names);

    /*!
     * \brief Initialize structure specific configurations.
     *
     * \note All functions should be registered with the object before init is called.
     */
    virtual void init() override;

protected:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    IBRedundantInitializer();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBRedundantInitializer(const IBRedundantInitializer& from) = delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBRedundantInitializer& operator=(const IBRedundantInitializer& that) = delete;

    /*!
     * \brief Configure the Lagrangian Silo data writer to plot the data
     * associated with the specified level of the locally refined Cartesian
     * grid.
     */
    void initializeLSiloDataWriter(int level_number);

    /*!
     * \brief Initialize vertex data programmatically.
     */
    void initializeStructurePosition();

    /*!
     * \brief Initialize spring data programmatically.
     */
    void initializeSprings();

    /*!
     * \brief Initialize xspring data programmatically.
     */
    void initializeXSprings();

    /*!
     * \brief Initialize beam data programmatically.
     */
    void initializeBeams();

    /*!
     * \brief Initialize director and rod data programmatically.
     */
    void initializeDirectorAndRods();

    /*!
     * \brief Initialize massive point data programmatically.
     */
    void initializeBoundaryMass();

    /*!
     * \brief Initialize target point data programmatically.
     */
    void initializeTargetPts();

    /*!
     * \brief Initialize anchor points programmatically.
     */
    void initializeAnchorPts();

    /*!
     * \brief Initialize instrumentation data.
     */
    void initializeInstrumentationData();

    /*!
     * \brief Initialize source/sink data.
     */
    void initializeSourceData();

    /*!
     * \brief Determine the indices of any vertices initially owned by the
     * specified patch.
     */
    void getPatchVertices(std::vector<std::pair<int, int> >& point_indices,
                          SAMRAI::tbox::Pointer<SAMRAI::hier::Patch<NDIM> > patch,
                          SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy) const;

    /*!
     * \brief Determine the indices of any vertices associated with a given
     * level number initially located within the specified patch.
     */
    void getPatchVerticesAtLevel(std::vector<std::pair<int, int> >& point_indices,
                                 SAMRAI::tbox::Pointer<SAMRAI::hier::Patch<NDIM> > patch,
                                 SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                 int level_number) const;

    /*!
     * \return The canonical Lagrangian index of the specified vertex.
     */
    int getCanonicalLagrangianIndex(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The initial position of the specified vertex.
     */
    IBTK::Point getVertexPosn(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The initial position of the specified vertex.
     */
    IBTK::Point getShiftedVertexPosn(const std::pair<int, int>& point_index,
                                     int level_number,
                                     const double* domain_x_lower,
                                     const double* domain_x_upper,
                                     const SAMRAI::hier::IntVector<NDIM>& periodic_shift) const;

    /*!
     * \return The target point specifications associated with a particular
     * node.
     */
    const TargetSpec& getVertexTargetSpec(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The anchor point specifications associated with a particular
     * node.
     */
    const AnchorSpec& getVertexAnchorSpec(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The massive boundary point specifications associated with a
     * particular node.
     */
    const BdryMassSpec& getVertexBdryMassSpec(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The directors associated with a particular node.
     */
    const std::vector<double>& getVertexDirectors(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The instrumentation indices associated with a particular node (or
     * std::make_pair(-1,-1) if there is no instrumentation data associated with
     * that node).
     */
    std::pair<int, int> getVertexInstrumentationIndices(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The source indices associated with a particular node (or -1 if
     * there is no source data associated with that node).
     */
    int getVertexSourceIndices(const std::pair<int, int>& point_index, int level_number) const;

    /*!
     * \return The specification objects associated with the specified vertex.
     */
    virtual std::vector<SAMRAI::tbox::Pointer<IBTK::Streamable> >
    initializeNodeData(const std::pair<int, int>& point_index,
                       unsigned int global_index_offset,
                       int level_number) const;

    /*!
     * Read input values, indicated above, from given database.
     *
     * When assertion checking is active, the database pointer must be non-null.
     */
    void getFromInput(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*
     * The object name is used as a handle to databases stored in restart files
     * and for error reporting purposes.
     */
    std::string d_object_name;

    /*
     * The maximum number of levels in the Cartesian grid patch hierarchy and a
     * vector of boolean values indicating whether a particular level has been
     * initialized yet.
     */
    int d_max_levels = -1;
    std::vector<bool> d_level_is_initialized;

    /*
     * An (optional) Lagrangian Silo data writer.
     */
    SAMRAI::tbox::Pointer<IBTK::LSiloDataWriter> d_silo_writer;

    /*
     * The base filenames of the structures are used to generate unique names
     * when registering data with the Silo data writer.
     */
    std::vector<std::vector<std::string> > d_base_filename;

    /*
     * Optional shift and scale factors.
     *
     * \note These shift and scale factors are applied to ALL structures read in
     * by this reader.
     *
     * \note The scale factor is applied both to positions and to spring rest
     * lengths.
     *
     * \note The shift factor should have the same units as the positions in the
     * input files, i.e., X_final = scale*(X_initial + shift).
     */
    double d_length_scale_factor = 1.0;
    IBTK::Vector d_posn_shift;

    /*
     * Vertex information.
     */
    std::vector<std::vector<int> > d_num_vertex, d_vertex_offset;
    std::vector<std::vector<std::vector<IBTK::Point> > > d_vertex_posn;

    /*
     * Spring information.
     */
    std::vector<std::vector<std::multimap<int, Edge> > > d_spring_edge_map;

    std::vector<std::vector<std::map<Edge, SpringSpec, EdgeComp> > > d_spring_spec_data;

    /*
     * Crosslink spring ("x-spring") information.
     */
    std::vector<std::vector<std::multimap<int, Edge> > > d_xspring_edge_map;

    std::vector<std::vector<std::map<Edge, XSpringSpec, EdgeComp> > > d_xspring_spec_data;

    /*
     * Beam information.
     */
    std::vector<std::vector<std::multimap<int, BeamSpec> > > d_beam_spec_data;

    /*
     * Rod information.
     */
    std::vector<std::vector<std::multimap<int, Edge> > > d_rod_edge_map;

    std::vector<std::vector<std::map<Edge, RodSpec, EdgeComp> > > d_rod_spec_data;

    /*
     * Target point information.
     */
    std::vector<std::vector<std::vector<TargetSpec> > > d_target_spec_data;

    /*
     * Anchor point information.
     */
    std::vector<std::vector<std::vector<AnchorSpec> > > d_anchor_spec_data;

    /*
     * Mass information for the pIB method.
     */
    std::vector<std::vector<std::vector<BdryMassSpec> > > d_bdry_mass_spec_data;

    /*
     * Orthonormal directors for the generalized IB method.
     */
    std::vector<std::vector<std::vector<std::vector<double> > > > d_directors;

    /*
     * Instrumentation information.
     */
    std::vector<std::vector<std::map<int, std::pair<int, int> > > > d_instrument_idx;

    /*
     * Source information.
     */
    std::vector<std::vector<std::map<int, int> > > d_source_idx;

    /*
     * Data required to specify connectivity information for visualization
     * purposes.
     */
    std::vector<unsigned int> d_global_index_offset;

    /*!
     * Check if user defined data has been processed.
     */
    bool d_data_processed = false;

private:
    /*
     * Functions used to initialize structures programmatically.
     */
    InitStructureOnLevel d_init_structure_on_level_fcn = nullptr;
    void* d_init_structure_on_level_ctx = nullptr;
    InitSpringDataOnLevel d_init_spring_on_level_fcn = nullptr;
    void* d_init_spring_on_level_ctx = nullptr;
    InitXSpringDataOnLevel d_init_xspring_on_level_fcn = nullptr;
    void* d_init_xspring_on_level_ctx = nullptr;
    InitBeamDataOnLevel d_init_beam_on_level_fcn = nullptr;
    void* d_init_beam_on_level_ctx = nullptr;
    InitDirectorAndRodOnLevel d_init_director_and_rod_on_level_fcn = nullptr;
    void* d_init_director_and_rod_on_level_ctx = nullptr;
    InitBoundaryMassOnLevel d_init_boundary_mass_on_level_fcn = nullptr;
    void* d_init_boundary_mass_on_level_ctx = nullptr;
    InitTargetPtOnLevel d_init_target_pt_on_level_fcn = nullptr;
    void* d_init_target_pt_on_level_ctx = nullptr;
    InitAnchorPtOnLevel d_init_anchor_pt_on_level_fcn = nullptr;
    void* d_init_anchor_pt_on_level_ctx = nullptr;
    InitInstrumentationOnLevel d_init_instrumentation_on_level_fcn = nullptr;
    void* d_init_instrumentation_on_level_ctx = nullptr;
    InitSourceOnLevel d_init_source_on_level_fcn = nullptr;
    void* d_init_source_on_level_ctx = nullptr;
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBAMR_IBRedundantInitializer
