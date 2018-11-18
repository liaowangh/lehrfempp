/* **************************************************************************
 * LehrFEM++ - A simple C++ finite element libray for teaching
 * Developed from 2018 at the Seminar of Applied Mathematics of ETH Zurich,
 * lead developers Dr. R. Casagrande and Prof. R. Hiptmair
 ***************************************************************************/

/**
 * @file
 * @brief Unit tests for parametric finite element facilities
 * @author Ralf Hiptmair
 * @date October 2018
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include <iostream>
#include "lf/fe/fe_space.h"
#include "lf/fe/fe_tools.h"
#include "lf/fe/lin_fe.h"
#include "lf/fe/loc_comp_ellbvp.h"

#include <lf/mesh/utils/utils.h>
#include "lf/mesh/test_utils/test_meshes.h"

namespace lf::fe::test {

TEST(lf_fe, lf_fe_linfe) {
  // Three points in the reference element
  Eigen::MatrixXd refcoords{
      (Eigen::MatrixXd(2, 3) << 0.3, 0.1, 0.7, 0.2, 0.5, 0.1).finished()};
  std::cout << "Points in reference cell\n" << refcoords << std::endl;

  // Testing triangular element
  {
    TriaLinearLagrangeFE<double> tlfe{};
    EXPECT_EQ(tlfe.NumRefShapeFunctions(), 3);
    EXPECT_EQ(tlfe.NumRefShapeFunctions(0, 0), 0);
    EXPECT_EQ(tlfe.NumRefShapeFunctions(1, 0), 0);
    EXPECT_EQ(tlfe.NumRefShapeFunctions(2, 0), 1);

    auto rsf_vals = tlfe.EvalReferenceShapeFunctions(refcoords);
    for (const auto &v : rsf_vals) {
      std::cout << "Tria: RSF values: " << v << std::endl;
    }
    for (const auto &v : tlfe.GradientsReferenceShapeFunctions(refcoords)) {
      std::cout << "Tria: RSF gradients:\n " << v << std::endl;
    }
    std::cout << "Tria: Evaluation nodes\n"
              << tlfe.EvaluationNodes() << std::endl;
  }

  // Testing quadrilateral element
  {
    QuadLinearLagrangeFE<double> qlfe{};
    EXPECT_EQ(qlfe.NumRefShapeFunctions(), 4);
    EXPECT_EQ(qlfe.NumRefShapeFunctions(0, 0), 0);
    EXPECT_EQ(qlfe.NumRefShapeFunctions(1, 0), 0);
    EXPECT_EQ(qlfe.NumRefShapeFunctions(2, 0), 1);

    auto rsf_vals = qlfe.EvalReferenceShapeFunctions(refcoords);
    for (const auto &v : rsf_vals) {
      std::cout << "Quad: RSF values: " << v << std::endl;
    }
    for (const auto &v : qlfe.GradientsReferenceShapeFunctions(refcoords)) {
      std::cout << "Quad: RSF gradients:\n " << v << std::endl;
    }
    std::cout << "Quad: Evaluation nodes\n"
              << qlfe.EvaluationNodes() << std::endl;
  }
}

TEST(lf_fe, lf_fe_segment) {
  // Three points in unit interval
  Eigen::MatrixXd refcoords{
      (Eigen::MatrixXd(1, 3) << 0.3, 0.1, 0.7).finished()};
  std::cout << "Points in reference cell\n" << refcoords << std::endl;

  SegmentLinearLagrangeFE<double> slfe{};
  EXPECT_EQ(slfe.NumRefShapeFunctions(), 2);
  EXPECT_EQ(slfe.NumRefShapeFunctions(0, 0), 0);
  EXPECT_EQ(slfe.NumRefShapeFunctions(1, 0), 1);

  auto rsf_vals = slfe.EvalReferenceShapeFunctions(refcoords);
  for (const auto &v : rsf_vals) {
    std::cout << "Segment: RSF values: " << v << std::endl;
  }
  for (const auto &v : slfe.GradientsReferenceShapeFunctions(refcoords)) {
    std::cout << "Segment: RSF gradients:\n " << v << std::endl;
  }
  std::cout << "Segment: Evaluation nodes\n"
            << slfe.EvaluationNodes() << std::endl;
}

TEST(lf_fe, lf_fe_ellbvp) {
  std::cout << "### TEST: Computation of element matrices" << std::endl;
  // Building the test mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh();

  // Set up finite elements
  TriaLinearLagrangeFE<double> tlfe{};
  QuadLinearLagrangeFE<double> qlfe{};

  // Set up objects taking care of local computations
  auto alpha = [](Eigen::Vector2d) -> double { return 1.0; };
  auto gamma = [](Eigen::Vector2d) -> double { return 0.0; };
  using loc_comp_t =
      LagrangeFEEllBVPElementMatrix<double, decltype(alpha), decltype(gamma)>;
  // Set debugging flags
  // loc_comp_t::ctrl_ = 255;
  // lf::quad::QuadRule::out_ctrl_ = 1;

  loc_comp_t comp_elem_mat(tlfe, qlfe, alpha, gamma);

  // For comparison
  LinearFELaplaceElementMatrix lfe_elem_mat{};

  // Loop over cells and compute element matrices;
  for (const lf::mesh::Entity &cell : mesh_p->Entities(0)) {
    const lf::base::size_type n(cell.RefEl().NumNodes());
    std::cout << "CELL " << cell << ":" << std::endl;
    std::cout << "Element matrix from LinearFELaplaceElementMatrix:"
              << std::endl;
    LinearFELaplaceElementMatrix::ElemMat lfe_mat{lfe_elem_mat.Eval(cell)};
    std::cout << lfe_mat << std::endl;
    std::cout << "Element matrix from LagrangeFEEllBVPElementMatrix:"
              << std::endl;
    loc_comp_t::ElemMat quad_mat{comp_elem_mat.Eval(cell)};
    std::cout << quad_mat << std::endl;
    EXPECT_NEAR((lfe_mat.block(0, 0, n, n) - quad_mat).norm(), 0.0, 1E-2);
  }
}

TEST(lf_fe, lf_fe_loadvec) {
  std::cout << "### TEST: Computation of element vectors" << std::endl;
  // Building the test mesh: a purely triangular mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh(3);

  // Set up finite elements
  TriaLinearLagrangeFE<double> tlfe{};
  QuadLinearLagrangeFE<double> qlfe{};

  // Set up objects taking care of local computations
  auto f = [](Eigen::Vector2d x) -> double { return (2 * x[0] + x[1]); };
  using loc_comp_t = ScalarFELocalLoadVector<double, decltype(f)>;

  // Set debugging flags
  loc_comp_t::ctrl_ = 0;                                       // 255;
  lf::quad::QuadRule::out_ctrl_ = 0;                           // 1;
  LinearFELocalLoadVector<double, decltype(f)>::dbg_ctrl = 0;  // 3;

  // Instantiate object for local computations
  loc_comp_t comp_elem_vec(tlfe, qlfe, f);

  // For comparison
  LinearFELocalLoadVector<double, decltype(f)> lfe_elem_vec(f);

  // Loop over cells and compute element matrices;
  for (const lf::mesh::Entity &cell : mesh_p->Entities(0)) {
    const lf::base::size_type n(cell.RefEl().NumNodes());
    std::cout << "CELL " << cell << ":" << std::endl;
    std::cout << "Element vector from LinearFELaplaceElementMatrix:"
              << std::endl;
    LinearFELocalLoadVector<double, decltype(f)>::ElemVec lfe_vec{
        lfe_elem_vec.Eval(cell)};
    std::cout << "[ " << lfe_vec.transpose() << "] " << std::endl;
    std::cout << "Element vector from ScalarFELocalLoadVector:" << std::endl;
    loc_comp_t::ElemVec quad_vec{comp_elem_vec.Eval(cell)};
    std::cout << "[ " << quad_vec.transpose() << "] " << std::endl;
    EXPECT_NEAR((lfe_vec.head(n) - quad_vec).norm(), 0.0, 1E-2);
  }
}

TEST(lf_fe, lf_fe_l2norm) {
  // LocCompLagrFEPreprocessor::ctrl_ = 255;
  std::cout << "### TEST Computation of L2 norm" << std::endl;
  // This test computes an approximation of the L2 norm of a function
  // by local quadrature on a finite element mesh, using the facilities
  // for computation of norms of differences of functions.

  // Building the test mesh: a purely affine mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh(3);

  // Set up global FE space
  UniformScalarFiniteElementSpace fe_space(
      mesh_p, std::make_unique<TriaLinearLagrangeFE<double>>(),
      std::make_unique<QuadLinearLagrangeFE<double>>());

  // Dummy vector for coefficients of FE function
  const lf::assemble::DofHandler &dofh{fe_space.LocGlobMap()};
  std::vector<double> zerovec(dofh.NoDofs(), 0.0);

  // Helper object for computation of norm
  // Function passed as a generic lambda expression
  LocalL2NormDifference loc_comp(
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kTria()),
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kQuad()),
      [](auto x) { return (x[0] * (1.0 - x[1])); }, 4);
  // loc_comp.ctrl_ = 255;

  /*
  // Alternative implementation
  // Function whose L2 should be computed; a generic lambda here
  auto u = [](auto x) { return (x[0] * (1.0 - x[1])); };
   LocalL2NormDifference<decltype(u)> loc_comp(
        fe_space.TriaShapeFunctionLayout(), fe_space.QuadShapeFunctionLayout(),
        [](auto x) { return (x[0] * (1.0 - x[1])); }, 4);
  */

  // Actual compuation of norm
  double norm = NormOfDifference(dofh, loc_comp, zerovec);
  std::cout << "Norm = " << norm << std::endl;
  EXPECT_NEAR(norm, 5.19615, 1E-4);
}

TEST(lf_fe, lf_fe_l2norm_vf) {
  // LocCompLagrFEPreprocessor::ctrl_ = 255;
  std::cout << "### TEST Computation of L2 norm of vectorfield" << std::endl;
  // This test computes an approximation of the L2 norm of a function
  // by local quadrature on a finite element mesh, using the facilities
  // for computation of norms of differences of functions.

  // Building the test mesh: a purely affine mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh(3);

  // Set up global FE space
  UniformScalarFiniteElementSpace fe_space(
      mesh_p, std::make_unique<TriaLinearLagrangeFE<double>>(),
      std::make_unique<QuadLinearLagrangeFE<double>>());

  // Dummy vector for coefficients of FE function
  const lf::assemble::DofHandler &dofh{fe_space.LocGlobMap()};
  std::vector<double> zerovec(dofh.NoDofs(), 0.0);

  // Helper object for computation of norm
  // Function passed as a generic lambda expression
  LocL2GradientFEDifference loc_comp(
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kTria()),
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kQuad()),
      [](auto x) { return (Eigen::Vector2d() << x[1], -x[0]).finished(); }, 4);
  // loc_comp.ctrl_ = 255;

  // Actual compuation of norm
  double norm = NormOfDifference(dofh, loc_comp, zerovec);
  std::cout << "Norm of vectorfield = " << norm << std::endl;
  EXPECT_NEAR(norm, 7.34847, 1E-4);
}

TEST(lf_fe, lf_fe_lintp) {
  std::cout << "### TEST: Linear Interpolation" << std::endl;
  // Building the test mesh: a general hybrid mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh(0);

  // Set up global FE space
  UniformScalarFiniteElementSpace fe_space(
      mesh_p, std::make_unique<TriaLinearLagrangeFE<double>>(),
      std::make_unique<QuadLinearLagrangeFE<double>>());

  auto u = [](auto x) { return std::exp(x[0] * (1.0 - x[1])); };
  auto coeffvec{NodalProjection(fe_space, u)};

  // Local-to-global index mapped
  const lf::assemble::DofHandler &dofh{fe_space.LocGlobMap()};
  EXPECT_EQ(coeffvec.size(), dofh.NoDofs())
      << "Coefficient vector length mismatch";

  // Check agreement of nodal values
  for (lf::assemble::gdof_idx_t j = 0; j < coeffvec.size(); ++j) {
    const lf::mesh::Entity &node{dofh.Entity(j)};
    EXPECT_TRUE(node.RefEl() == lf::base::RefEl::kPoint());
    auto tmp_coord(
        node.Geometry()->Global(lf::base::RefEl::kPoint().NodeCoords()));
    auto pt_coord(tmp_coord.col(0));
    // std::cout << "@ [" << pt_coord.transpose() << "]: u = " << u(pt_coord)
    //           << " <-> u_h = " << coeffvec[j] << std::endl;
    EXPECT_DOUBLE_EQ(coeffvec[j], u(pt_coord))
        << " @ [" << pt_coord.transpose() << "]:";
  }
}

// check whether linear function is interpolated exactly
bool checkInterpolationLinear(const UniformScalarFiniteElementSpace &fe_space) {
  // Model linear function
  auto u = [](auto x) { return (x[0] - 2 * x[1]); };
  // Interpolation
  auto coeffvec{NodalProjection(fe_space, u)};
  // Helper class for error computation
  LocalL2NormDifference loc_comp(
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kTria()),
      fe_space.ShapeFunctionLayout(lf::base::RefEl::kQuad()), u, 4);
  // Actual compuation of error norm
  double norm = NormOfDifference(fe_space.LocGlobMap(), loc_comp, coeffvec);
  std::cout << "Norm = " << norm << std::endl;
  EXPECT_NEAR(norm, 0.0, 1E-6);
  return (std::fabs(norm) < 1.0E-6);
}

TEST(lf_fe, lf_fe_lintp_exact) {
  std::cout << "### TEST: Reproduction of linear functions" << std::endl;
  // Building the test mesh: a general hybrid mesh
  auto mesh_p = lf::mesh::test_utils::GenerateHybrid2DTestMesh(0);

  // Set up global FE space
  UniformScalarFiniteElementSpace fe_space(
      mesh_p, std::make_unique<TriaLinearLagrangeFE<double>>(),
      std::make_unique<QuadLinearLagrangeFE<double>>());
  EXPECT_TRUE(checkInterpolationLinear(fe_space));
}

}  // end namespace lf::fe::test