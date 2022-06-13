/*
In this file, in the first part, we read and assign values from the input file
present in Examples folder, construct boundaries, create file system for our
outputs and create our output directory. In the next part, based on the
parameters from the file, we simulate our case and generate results. They are
saved in the defined output folder.
*/
#include "Case.hpp"

#include <algorithm>

#include "Enums.hpp"
#ifdef GCC_VERSION_9_OR_HIGHER
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <mpi.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

#ifdef GCC_VERSION_9_OR_HIGHER
namespace filesystem = std::filesystem;
#else
namespace filesystem = std::experimental::filesystem;
#endif

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
// #include <vtkPointVisibilityArray.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredGridWriter.h>
#include <vtkTuple.h>

// Read input parameters.

Case::Case(std::string file_name, int argn, char **args) {
  const int MAX_LINE_LENGTH = 1024;
  std::ifstream file(file_name);
  double nu;     /* viscosity   */
  double UI;     /* velocity x-direction */
  double VI;     /* velocity y-direction */
  double TI = 0; /* Initial temperature.*/
  double UIN;
  double VIN;
  double PI;        /* pressure */
  double GX;        /* gravitation x-direction */
  double GY;        /* gravitation y-direction */
  double xlength;   /* length of the domain x-dir.*/
  double ylength;   /* length of the domain y-dir.*/
  double dt;        /* time step */
  int imax;         /* number of cells x-direction*/
  int jmax;         /* number of cells y-direction*/
  double gamma;     /* uppwind differencing factor*/
  double omg;       /* relaxation factor */
  double tau;       /* safety factor for time step*/
  int itermax;      /* max. number of iterations for pressure per time step */
  double eps;       /* accuracy bound for pressure*/
  double alpha = 0; /* Thermal Diffusivity*/
  double beta = 0;  /* Thermal expansion coefficient*/
  int num_walls;
  std::string energy_eqn;
  double temp3;
  double temp4;
  double temp5;
  int iproc = 1;
  int jproc = 1;
  // Assigning parameters from the file to variables.

  if (file.is_open()) {
    std::string var;
    while (!file.eof() && file.good()) {
      file >> var;
      if (var[0] == '#') {
        file.ignore(MAX_LINE_LENGTH, '\n');
      } else {
        if (var == "geo_file") file >> _geom_name;
        if (var == "xlength") file >> xlength;
        if (var == "ylength") file >> ylength;
        if (var == "nu") file >> nu;
        if (var == "t_end") file >> _t_end;
        if (var == "dt") file >> dt;
        if (var == "omg") file >> omg;
        if (var == "eps") file >> eps;
        if (var == "tau") file >> tau;
        if (var == "gamma") file >> gamma;
        if (var == "dt_value") file >> _output_freq;
        if (var == "UI") file >> UI;
        if (var == "VI") file >> VI;
        if (var == "TI") file >> TI;
        if (var == "GX") file >> GX;
        if (var == "GY") file >> GY;
        if (var == "PI") file >> PI;
        if (var == "UIN") file >> UIN;
        if (var == "VIN") file >> VIN;
        if (var == "itermax") file >> itermax;
        if (var == "imax") file >> imax;
        if (var == "jmax") file >> jmax;
        if (var == "alpha") file >> alpha;
        if (var == "beta") file >> beta;
        if (var == "num_walls") file >> num_walls;
        if (var == "wall_temp_3") file >> temp3;
        if (var == "wall_temp_4") file >> temp4;
        if (var == "wall_temp_5") file >> temp5;
        if (var == "energy_eq") file >> energy_eqn;
        if (var == "iproc") file >> iproc;
        if (var == "jproc") file >> jproc;
      }
    }
  }
  file.close();

  std::map<int, double> wall_temp;
  bool boolenergy_eq = false;

  if (energy_eqn.compare("on") == 0) {
    boolenergy_eq = true;
    wall_temp.insert(std::pair<int, double>(cellID::fixed_wall_3, temp3));
    wall_temp.insert(std::pair<int, double>(cellID::fixed_wall_4, temp4));
  }

  std::map<int, double> wall_vel;
  if (_geom_name.compare("NONE") == 0) {
    wall_vel.insert(std::pair<int, double>(LidDrivenCavity::moving_wall_id,
                                           LidDrivenCavity::wall_velocity));
  }

  // Setting file names for geometry file and output directory

  set_file_names(file_name);

  // Building up our domain
  Domain domain;
  domain.dx = xlength / imax;
  domain.dy = ylength / jmax;
  domain.domain_size_x = imax;
  domain.domain_size_y = jmax;

  if (Communication::rank == 0) {
    int curr_rank = 0;
    domain.imin = (curr_rank % iproc) * (imax / iproc);
    domain.jmin = (curr_rank / iproc) % jproc * (jmax / jproc);
    domain.imax = (curr_rank % iproc + 1) * (imax / iproc) + 2;
    domain.jmax = ((curr_rank / iproc) % jproc + 1) * (jmax / jproc) + 2;
    domain.size_x = imax / iproc;
    domain.size_y = jmax / jproc;
    if (curr_rank % iproc + 1 < iproc) {
      domain.domain_neighbors.at(0) = curr_rank + 1;
    }
    if (curr_rank % iproc - 1 >= 0) {
      domain.domain_neighbors.at(2) = curr_rank - 1;
    }
    if (curr_rank + iproc < iproc * jproc - 1) {
      domain.domain_neighbors.at(1) = curr_rank + iproc;
    }
    if (curr_rank - iproc >= 0) {
      domain.domain_neighbors.at(3) = curr_rank - iproc;
    }
    build_domain(domain, imax, jmax, iproc, jproc);
  } else {
    std::vector<int> rec_data(10);
    MPI_Status status;
    MPI_Recv(rec_data.data(), 10, MPI_INT, 0, Communication::rank,
             MPI_COMM_WORLD, &status);
    domain.imin = rec_data.at(0);
    domain.jmin = rec_data.at(1);
    domain.imax = rec_data.at(2);
    domain.jmax = rec_data.at(3);
    domain.size_x = rec_data.at(4);
    domain.size_y = rec_data.at(5);
    domain.domain_neighbors.at(0) = rec_data.at(6);
    domain.domain_neighbors.at(2) = rec_data.at(7);
    domain.domain_neighbors.at(1) = rec_data.at(8);
    domain.domain_neighbors.at(3) = rec_data.at(9);
  }

  std::cout << Communication::rank << " " << domain.imin << " " << domain.imax
            << " " << domain.jmin << " " << domain.jmax << " "
            << domain.domain_neighbors.at(0) << " "
            << domain.domain_neighbors.at(1) << " "
            << domain.domain_neighbors.at(2) << " "
            << domain.domain_neighbors.at(3) << "\n";

  _grid = Grid(_geom_name, domain);

  _field = Fields(nu, alpha, beta, dt, tau, _grid.domain().size_x,
                  _grid.domain().size_y, UI, VI, PI, TI, GX, GY, boolenergy_eq);

  _discretization = Discretization(domain.dx, domain.dy, gamma);
  _pressure_solver = std::make_unique<SOR>(omg);
  _max_iter = itermax;
  _tolerance = eps;

  //   // // Constructing boundaries

  if (not _grid.moving_wall_cells().empty()) {
    _boundaries.push_back(std::make_unique<MovingWallBoundary>(
        _grid.moving_wall_cells(), LidDrivenCavity::wall_velocity));
    // std::cout<<_grid.moving_wall_cells().size()<<"
    // "<<Communication::rank<<'\n';
  }
  if (not _grid.inlet_cells().empty()) {
    _boundaries.push_back(
        std::make_unique<InletBoundary>(_grid.inlet_cells(), UIN, VIN));
  }
  if (not _grid.outlet_cells().empty()) {
    _boundaries.push_back(
        std::make_unique<OutletBoundary>(_grid.outlet_cells()));
  }
  if (not _grid.fixed_wall_cells().empty()) {
    if (!boolenergy_eq) {
      _boundaries.push_back(
          std::make_unique<FixedWallBoundary>(_grid.fixed_wall_cells()));
      // std::cout<<_grid.fixed_wall_cells().size()<<"
      // "<<Communication::rank<<'\n';
    } else {
      _boundaries.push_back(std::make_unique<FixedWallBoundary>(
          _grid.fixed_wall_cells(), wall_temp));
    }
  }
  if (not _grid.adiabatic_cells().empty()) {
    _boundaries.push_back(
        std::make_unique<AdiabaticBoundary>(_grid.adiabatic_cells()));
  }

  output_vtk(0, Communication::rank);
}

void Case::set_file_names(std::string file_name) {
  std::string temp_dir;
  bool case_name_flag = true;
  bool prefix_flag = false;

  for (int i = file_name.size() - 1; i > -1; --i) {
    if (file_name[i] == '/') {
      case_name_flag = false;
      prefix_flag = true;
    }
    if (case_name_flag) {
      _case_name.push_back(file_name[i]);
    }
    if (prefix_flag) {
      _prefix.push_back(file_name[i]);
    }
  }

  for (int i = file_name.size() - _case_name.size() - 1; i > -1; --i) {
    temp_dir.push_back(file_name[i]);
  }

  std::reverse(_case_name.begin(), _case_name.end());
  std::reverse(_prefix.begin(), _prefix.end());
  std::reverse(temp_dir.begin(), temp_dir.end());

  _case_name.erase(_case_name.size() - 4);
  _dict_name = temp_dir;
  _dict_name.append(_case_name);
  _dict_name.append("_Output");

  if (_geom_name.compare("NONE") != 0) {
    _geom_name = _prefix + _geom_name;
  }

  // Creating output directory to store the results.

  filesystem::path folder(_dict_name);
  try {
    filesystem::create_directory(folder);
  } catch (const std::exception &e) {
    std::cerr << "Output directory could not be created." << std::endl;
    std::cerr << "Make sure that you have write permissions to the "
                 "corresponding location"
              << std::endl;
  }
}

// /**
//  * This function is the main simulation loop. In the simulation loop,
//  following
//  * steps are required
//  * - Calculate and apply boundary conditions for all the boundaries in
//  * _boundaries container using apply() member function of Boundary class
//  * - Calculate fluxes (F and G) using calculate_fluxes() member function of
//  * Fields class. Flux consists of diffusion and convection part, which are
//  * located in Discretization class
//  * - Calculate right-hand-side of PPE using calculate_rs() member function
//  of
//  * Fields class
//  * - Iterate the pressure poisson equation until the residual becomes
//  smaller
//  * than the desired tolerance or the maximum number of the iterations are
//  * performed using solve() member function of PressureSolver class
//  * - Calculate the velocities u and v using calculate_velocities() member
//  * function of Fields class
//  * - Calculat the maximal timestep size for the next iteration using
//  * calculate_dt() member function of Fields class
//  * - Write vtk files using output_vtk() function
//  *
//  * Please note that some classes such as PressureSolver, Boundary are
//  abstract
//  * classes which means they only provide the interface. No member functions
//  * should be defined in abstract classes. You need to define functions in
//  * inherited classes such as MovingWallBoundary class.
//  *
//  * For information about the classes and functions, you can check the
//  header
//  * files.
//  */
void Case::simulate() {
  // Defining parameters for running of the loop

  double t = 0.0;
  double dt = _field.dt();
  int timestep = 0;
  double output_counter = _output_freq;
  int iter;
  double res;
  int total_iter = 1;
  std::ofstream logfile;
  // logfile.open("log.txt");

  for (int i = 0; i < _boundaries.size(); i++) {
    _boundaries[i]->apply(_field);
  }

  output_vtk(timestep, Communication::rank);

  // Following is the actual loop that runs till the defined time limit.

  while (t <= _t_end) {
    //   // Calculating timestep for advancement to the next iteration.
    dt = _field.calculate_dt(_grid);
    // std::cout << Communication::rank << " " << dt << "\n";

    dt = Communication::reduce_min(dt);

    // std::cout << Communication::rank << " " << dt << "\n";

    //   // Calculate new Temperatures
    if (_field.energy_eq()) {
      _field.calculate_temperature(_grid);
      Communication::communicate(_field.t_matrix(), _grid.domain());
    }

    //   // Calculating Fluxes (_F and _G) for velocities in X and Y direction
    //   // respectively.
    _field.calculate_fluxes(_grid);
    Communication::communicate(_field.f_matrix(), _grid.domain());
    Communication::communicate(_field.g_matrix(), _grid.domain());

    // Calculating RHS for pressure poisson equation
    _field.calculate_rs(_grid);

    iter = 0;  // Pressure poisson solver iteration initialization
    res = std::numeric_limits<double>::max();
    double num_fluid_cells = _grid.fluid_cells().size();

    while (res > _tolerance) {
      if (iter >= _max_iter) {
        if (Communication::rank == 0) {
          std::cout << "Pressure poisson solver did not converge to the given "
                       "tolerance...\n ";
          std::cout << "Timestep size: " << setw(10) << dt << " | "
                    << "Time: " << setw(8) << t << setw(3) << " | "
                    << "Residual: " << setw(11) << res << setw(3) << " | "
                    << "Pressure Poisson Iterations: " << setw(3) << iter
                    << '\n';
        }
        break;
      }
      for (int i = 0; i < _boundaries.size(); i++) {
        _boundaries[i]->apply_pressure(_field);
      }
      res = _pressure_solver->solve(_field, _grid, _boundaries);
      Communication::communicate(_field.p_matrix(), _grid.domain());
      double squared_res = res * res * num_fluid_cells;
      res = Communication::reduce_sum(squared_res);
      num_fluid_cells = Communication::reduce_sum(num_fluid_cells);
      res = res / num_fluid_cells;
      res = std::sqrt(res);
      // if (Communication::rank == 0) {
      //   std::cout << Communication::rank << " "
      //             << "Timestep size: " << setw(10) << dt << " | "
      //             << "Time: " << setw(8) << t << setw(3) << " | "
      //             << "Residual: " << setw(11) << res << setw(3) << " | "
      //             << "Pressure Poisson Iterations: " << setw(3) << iter <<
      //             '\n';
      // }

      iter++;
      total_iter++;
      // logfile << "Residual: " << res << " Iteration:" << total_iter <<
    }

    // Calculating updated velocities using pressure calculated in the
    // pressure poisson equation
    _field.calculate_velocities(_grid);
    Communication::communicate(_field.u_matrix(), _grid.domain());
    Communication::communicate(_field.v_matrix(), _grid.domain());

    for (int i = 0; i < _boundaries.size(); i++) {
      _boundaries[i]->apply(_field);
    }
    // Updating t for the next step
    t += dt;
    timestep++;

    // Printing Data in the terminal
    if (Communication::rank == 0) {
      if (timestep % 100 == 0) {
        std::cout << "Timestep size: " << setw(10) << dt << " | "
                  << "Time: " << setw(8) << t << setw(3) << " | "
                  << "Residual: " << setw(11) << res << setw(3) << " | "
                  << "Pressure Poisson Iterations: " << setw(3) << iter << '\n';
      }
    }

    if (t >= _output_freq) {
      output_vtk(timestep, Communication::rank);
      _output_freq = _output_freq + output_counter;
    }
  }
  // logfile.close();
}

// // Following is the pre-defined function for writing the output files.

void Case::output_vtk(int timestep, int rank) {
  // Creating a new structured grid
  vtkSmartPointer<vtkStructuredGrid> structuredGrid =
      vtkSmartPointer<vtkStructuredGrid>::New();

  // Creating grid
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  double dx = _grid.dx();
  double dy = _grid.dy();

  double x = _grid.domain().imin * dx;
  double y = _grid.domain().jmin * dy;

  { y += dy; }
  { x += dx; }

  double z = 0;
  for (int col = 0; col < _grid.domain().size_y + 1; col++) {
    x = _grid.domain().imin * dx;
    { x += dx; }
    for (int row = 0; row < _grid.domain().size_x + 1; row++) {
      points->InsertNextPoint(x, y, z);
      x += dx;
    }
    y += dy;
  }
  std::vector<vtkIdType> pointVisibility;
  auto _geom_excl_ghosts = _grid.get_geometry_excluding_ghosts();
  for (int j = 0; j < _grid.jmax(); j++) {
    for (int i = 0; i < _grid.imax(); i++) {
      if (_geom_excl_ghosts.at(i).at(j) == cellID::fixed_wall_3 ||
          _geom_excl_ghosts.at(i).at(j) == cellID::fixed_wall_4 ||
          _geom_excl_ghosts.at(i).at(j) == cellID::fixed_wall_5) {
        pointVisibility.push_back(i + (j)*_grid.domain().size_x);
      }
    }
  }

  // Specify the dimensions of the grid, addition of 1 to accomodate
  // neighboring cells
  structuredGrid->SetDimensions(_grid.domain().size_x + 1,
                                _grid.domain().size_y + 1, 1);
  structuredGrid->SetPoints(points);

  for (auto t{0}; t < pointVisibility.size(); t++) {
    structuredGrid->BlankCell(pointVisibility.at(t));
  }

  // Pressure Array
  vtkDoubleArray *Pressure = vtkDoubleArray::New();
  Pressure->SetName("pressure");
  Pressure->SetNumberOfComponents(1);

  // Velocity Array
  vtkDoubleArray *Velocity = vtkDoubleArray::New();
  Velocity->SetName("velocity");
  Velocity->SetNumberOfComponents(3);

  if (_field.energy_eq()) {
    vtkDoubleArray *Temperature = vtkDoubleArray::New();
    Temperature->SetName("temperature");
    Temperature->SetNumberOfComponents(1);

    for (int j = 1; j < _grid.domain().size_y + 1; j++) {
      for (int i = 1; i < _grid.domain().size_x + 1; i++) {
        double temperature = _field.t(i, j);
        Temperature->InsertNextTuple(&temperature);
      }
    }

    structuredGrid->GetCellData()->AddArray(Temperature);
  }

  // Print pressure and temperature from bottom to top
  for (int j = 1; j < _grid.domain().size_y + 1; j++) {
    for (int i = 1; i < _grid.domain().size_x + 1; i++) {
      double pressure = _field.p(i, j);
      Pressure->InsertNextTuple(&pressure);
    }
  }

  // Temp Velocity
  float vel[3];
  vel[2] = 0;  // Set z component to 0

  // Print Velocity from bottom to top
  for (int j = 0; j < _grid.domain().size_y + 1; j++) {
    for (int i = 0; i < _grid.domain().size_x + 1; i++) {
      vel[0] = (_field.u(i, j) + _field.u(i, j + 1)) * 0.5;
      vel[1] = (_field.v(i, j) + _field.v(i + 1, j)) * 0.5;
      Velocity->InsertNextTuple(vel);
    }
  }

  // Add Pressure to Structured Grid
  structuredGrid->GetCellData()->AddArray(Pressure);

  // Add Velocity to Structured Grid
  structuredGrid->GetPointData()->AddArray(Velocity);

  // Write Grid
  vtkSmartPointer<vtkStructuredGridWriter> writer =
      vtkSmartPointer<vtkStructuredGridWriter>::New();

  // Create Filename
  std::string outputname = _dict_name + '/' + _case_name + "_" +
                           std::to_string(rank) + "_" +
                           std::to_string(timestep) + ".vtk";

  writer->SetFileName(outputname.c_str());
  writer->SetInputData(structuredGrid);
  writer->Write();
}

void Case::build_domain(Domain &domain, int imax_domain, int jmax_domain,
                        int iproc, int jproc) {
  for (int curr_rank{1}; curr_rank < iproc * jproc; curr_rank++) {
    std::vector<int> data;
    int imin = (curr_rank % iproc) * (imax_domain / iproc);
    data.push_back(imin);
    int jmin = (curr_rank / iproc) % jproc * (jmax_domain / jproc);
    data.push_back(jmin);
    int imax = (curr_rank % iproc + 1) * (imax_domain / iproc) + 2;
    data.push_back(imax);
    int jmax = ((curr_rank / iproc) % jproc + 1) * (jmax_domain / jproc) + 2;
    data.push_back(jmax);
    int size_x = imax_domain / iproc;
    data.push_back(size_x);
    int size_y = jmax_domain / jproc;
    data.push_back(size_y);

    if (curr_rank % iproc + 1 < iproc) {
      int domain_neighbor_e = curr_rank + 1;
      data.push_back(domain_neighbor_e);
    } else {
      data.push_back(-1);
    }
    if (curr_rank % iproc - 1 >= 0) {
      int domain_neighbor_w = curr_rank - 1;
      data.push_back(domain_neighbor_w);
    } else {
      data.push_back(-1);
    }
    if (curr_rank + iproc < iproc * jproc) {
      int domain_neighbor_n = curr_rank + iproc;
      data.push_back(domain_neighbor_n);
    } else {
      data.push_back(-1);
    }
    if (curr_rank - iproc >= 0) {
      int domain_neighbor_s = curr_rank - iproc;
      data.push_back(domain_neighbor_s);
    } else {
      data.push_back(-1);
    }

    MPI_Send(data.data(), 10, MPI_INT, curr_rank, curr_rank, MPI_COMM_WORLD);
  }

  // std::cout << my_rank << " " << domain.imin << " " << domain.imax << " "
  //           << domain.jmin << " " << domain.jmax << " "
  //           << domain.domain_neighbors.at(0) << " "
  //           << domain.domain_neighbors.at(1) << " "
  //           << domain.domain_neighbors.at(2) << " "
  //           << domain.domain_neighbors.at(3) << "\n";
}
