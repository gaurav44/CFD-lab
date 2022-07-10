#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Cell.hpp"
#include "Datastructures.hpp"
#include "Domain.hpp"
#include "Enums.hpp"
#include "Particle.hpp"

/**
 * @brief Data structure holds cells and related sub-containers
 *
 */
class Grid {
 public:
  Grid() = default;

  /**
   * @brief Constructor for the Grid
   *
   * @param[in] geometry file name
   * @param[in] number of cells in x direction
   * @param[in] number of cells in y direction
   * @param[in] cell size in x direction
   * @param[in] cell size in y direction
   *
   */
  Grid(std::string geom_name, Domain &domain);

  /// index based cell access
  Cell cell(int i, int j) const;

  /// access number of cells in x direction
  int imax() const;
  /// access number of cells in y direction
  int jmax() const;
  /// access number of cells in x direction including ghost cells
  int imaxb() const;
  /// access number of cells in x direction including ghost cells
  int jmaxb() const;

  const Domain &domain() const;

  /// access cell size in x-direction
  double dx() const;
  /// access cell size in y-direction
  double dy() const;

  /**
   * @brief Access inflow cells
   *
   * @param[out] vector of fluid cells
   */
  std::vector<Cell *> &fluid_cells();

  /**
   * @brief Access moving wall cells
   *
   * @param[out] vector of moving wall cells
   */
  const std::vector<Cell *> &moving_wall_cells() const;

  /**
   * @brief Access fixed wall cells
   *
   * @param[out] vector of fixed wall cells
   */
  const std::vector<Cell *> &fixed_wall_cells() const;

  const std::vector<Cell *> &free_slip_cells() const;

  /**
   * @brief Access obstacle cells
   *
   * @param[out] vector of obstacle cells
   */

  const std::vector<Cell *> &inlet_cells() const;

  const std::vector<Cell *> &outlet_cells() const;

  const std::vector<Cell *> &adiabatic_cells() const;

  const std::vector<Cell *> &buffer() const;

  std::vector<Cell *> &surface_cells();

  std::vector<Particle> &particle();

  void set_particles(int ppc);

  const std::vector<std::vector<int>> &get_geometry_excluding_ghosts() const;

  void reset_fluid_cells();

 private:
  /**@brief Default lid driven cavity case generator
   *
   * This function creates default lid driven cavity
   * case without need for a pgm file
   */
  void build_lid_driven_cavity();

  /// Build cell data structures with given geometrical data
  void assign_cell_types(std::vector<std::vector<int>> &geometry_data);
  /// Extract geometry from pgm file and create geometrical data
  void parse_geometry_file(std::string filedoc,
                           std::vector<std::vector<int>> &geometry_data);

  /// Check geometry
  void check_geometry_file(std::vector<std::vector<int>> &geometry_data);

  Matrix<Cell> _cells;

  std::vector<Cell *> _fluid_cells;
  std::vector<Cell *> _fixed_wall_cells;
  std::vector<Cell *> _moving_wall_cells;
  std::vector<Cell *> _inlet_cells;
  std::vector<Cell *> _outlet_cells;
  std::vector<Cell *> _adiabatic_cells;
  std::vector<Cell *> _buffer;
  std::vector<Cell *> _surface_cells;
  std::vector<Cell *> _free_slip_cells;

  std::vector<Particle> _particles;

  Domain _domain;

  std::vector<std::vector<int>> geometry_excluding_ghosts;
};
