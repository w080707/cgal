#include <iostream>
#include <string>
#include <algorithm>

#include <CGAL/array.h>
#include <CGAL/Bbox_3.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/repair.h>

#include <CGAL/Polygon_mesh_processing/internal/named_function_params.h>
#include <CGAL/Polygon_mesh_processing/internal/named_params_helper.h>

//a material is composed of an material Id and a material name.
typedef std::pair<int, std::string> material;

struct MaterialData
{
  material innerRegion;
  material outerRegion;
};

std::string to_lower_case(const std::string& str)
{
  std::string r = str;
  std::transform(r.begin(), r.end(), r.begin(), ::tolower);
  return r;
}

bool get_material_metadata(std::istream& input,
                           std::string& line,
                           material& _material,
                           int material_id)
{
  std::istringstream iss;
  iss.str(line);

  iss >> _material.second;//name

  while (std::getline(input, line))
  {
    std::string prop; //property
    iss.clear();
    iss.str(line);
    iss >> prop;

    if (prop.compare("Id") == 0)
    {
      int tmp_id;
      iss >> tmp_id;
      _material.first = material_id;

      if ((0 == to_lower_case(_material.second).compare("exterior"))
          && _material.first != 0)
      {
        std::cerr << "Exterior should have index 0. ";
        std::cerr << "In this file it has index " << _material.first << "." << std::endl;
        std::cerr << "Reader failed, because Meshing will fail to terminate." << std::endl;
        return false;
      }
    }
    else if (prop.compare("}") == 0)
      return true; //end of this material
  }
  return false;
}

bool line_starts_with(const std::string& line, const char* cstr)
{
  const std::size_t fnws = line.find_first_not_of(" \t");
  if (fnws != std::string::npos)
    return (line.compare(fnws, strlen(cstr), cstr) == 0);
  return false;
}
namespace IO{
namespace internal{
bool treat_surf_materials(std::istream& input,
                          std::vector<material>& materials,
                          int& material_id)
{
  std::string line;
  while(std::getline(input, line))
  {
    if(line_starts_with(line, "}"))
      break;
    else
    {
      material _material;
      if (!get_material_metadata(input, line, _material, material_id++))
        return false;
      materials.push_back(_material);
    }
  }
  return true;
}

void treat_surf_grid_box(const std::string& line,
                        CGAL::Bbox_3& grid_box)
{
  std::istringstream iss;
  iss.str(line);
  std::string dump;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  iss >> dump >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;
  grid_box = CGAL::Bbox_3(xmin, ymin, zmin, xmax, ymax, zmax);
}

void treat_surf_grid_size(const std::string& line,
                         std::array<unsigned int, 3>& grid_size)
{
  std::istringstream iss;
  iss.str(line);
  std::string dump;
  iss >> dump >> grid_size[0] >> grid_size[1] >> grid_size[2];
}

template<typename Point_3>
void treat_surf_vertices(std::istream& input,
                         const std::string& input_line,
                         std::size_t& nb_vertices,
                         std::vector<Point_3>& points)
{
  std::istringstream iss;
  iss.str(input_line);
  std::string dump;
  iss >> dump >> nb_vertices;
  points.reserve(nb_vertices);
  std::cout<<nb_vertices<<" vertices"<<std::endl;

  std::string line;
  //get vertices
  while(std::getline(input, line))
  {
    if (line_starts_with(line, "NBranchingPoints"))
      break;
    double x(0),y(0),z(0);
    iss.clear();
    iss.str(line);
    iss >> CGAL::iformat(x) >> CGAL::iformat(y) >> CGAL::iformat(z);
    points.push_back(Point_3(x,y,z));
  }
}

template<typename MaterialData, typename Mesh>
void get_surf_patches(const std::string& line,
                      int& nb_patches,
                      std::vector<MaterialData>& metadata,
                      std::vector<Mesh>& output)
{
  std::istringstream iss;
  iss.str(line);
  std::string dump;
  iss >> dump >> nb_patches;
  std::cout<<nb_patches<<" patch(es)"<<std::endl;
  metadata.resize(nb_patches);
  output.resize(nb_patches);
}

void treat_surf_inner_region(std::istream& input,
                             const std::string& input_line,
                             std::vector<material>& materials,
                             std::vector<MaterialData>& metadata,
                             const int& i)
{
  std::istringstream iss;
  std::string name;
  iss.clear();
  iss.str(input_line);
  std::string dump;
  iss >> dump >> name;
  for(std::size_t j=0; j<materials.size(); ++j)
  {
    if(materials[j].second.compare(name) == 0)
    {
      metadata[i].innerRegion=materials[j];
      break;
    }
  }
  std::string line;
  std::getline(input, line);
  iss.clear();
  iss.str(line);
  iss >> dump >> name;
  for(std::size_t j=0; j<materials.size(); ++j)
  {
    if(materials[j].second.compare(name) == 0)
    {
      metadata[i].outerRegion=materials[j];
      break;
    }
  }
}

void treat_surf_triangles(const std::string& line,
                          std::size_t& nb_triangles)
{
  std::istringstream iss;
  iss.str(line);
  std::string dump;
  iss >> dump >> nb_triangles;
}

void connect_surf_triangles(std::istream& input,
                            const std::size_t& nb_triangles,
                            std::vector<std::array<std::size_t, 3> >& polygons)
{
  typedef std::array<std::size_t, 3> Triangle_ind;

  polygons.reserve(nb_triangles);
  std::string line;
  std::istringstream iss;
  while(std::getline(input, line))
  {
    std::size_t fnws=line.find_first_not_of(" \t");
    if(fnws != std::string::npos)
      line.erase(0, fnws);
    std::size_t index[3];
    if(line.compare(0, 1, "}") == 0)
    {
      break;
    }
    iss.clear();
    iss.str(line);
    iss >> index[0] >> index[1] >> index[2];

    Triangle_ind polygon;
    polygon[0] = index[0] - 1;
    polygon[1] = index[1] - 1;
    polygon[2] = index[2] - 1;
    polygons.push_back(polygon);
  }
  CGAL_assertion(nb_triangles == polygons.size());
}

template<typename Point_3, typename DuplicatedPointsOutIterator, typename Mesh>
void build_surf_patch(DuplicatedPointsOutIterator out,
                      std::vector<Mesh>& output,
                      std::vector<Point_3> points,
                      std::vector<std::array<std::size_t, 3> >& polygons,
                      const int& i)
{
  namespace PMP = CGAL::Polygon_mesh_processing;
  typedef std::array<std::size_t, 3> Triangle_ind;
  if (!PMP::is_polygon_soup_a_polygon_mesh(polygons))
  {
    std::cout << "Orientation of patch #" << (i + 1) << "...";
    std::cout.flush();

    const std::size_t nbp_init = points.size();
    bool no_duplicates =
        PMP::orient_polygon_soup(points, polygons);//returns false if some points
    //were duplicated

    std::cout << "\rOrientation of patch #" << (i + 1) << " done";

    if(!no_duplicates) //collect duplicates
    {
      for (std::size_t i = nbp_init; i < points.size(); ++i)
        *out++ = points[i];
      std::cout << " (non manifold -> "
                << (points.size() - nbp_init) << " duplicated vertices)";
    }
    std::cout << "." << std::endl;
  }

  Mesh& mesh = output[i];

  PMP::internal::PS_to_PM_converter<std::vector<Point_3>, std::vector<Triangle_ind> > converter(points, polygons);
  converter(mesh, false/*insert_isolated_vertices*/);

  CGAL_assertion(PMP::remove_isolated_vertices(mesh) == 0);
  CGAL_assertion(is_valid_polygon_mesh(mesh));
}

}//end internal
}//end IO

/*!
 * read_surf reads a file which extension is .surf and fills `output` with one `Mesh` per patch.
 * Mesh is a model of FaceListGraph.
 */
template<class Mesh, typename DuplicatedPointsOutIterator, class NamedParameters>
bool read_surf(std::istream& input, std::vector<Mesh>& output,
    std::vector<MaterialData>& metadata,
    CGAL::Bbox_3& grid_box,
    std::array<unsigned int, 3>& grid_size,
    DuplicatedPointsOutIterator out,
    const NamedParameters&)
{
  typedef typename CGAL::GetGeomTraits<Mesh, NamedParameters>::type Kernel;
  typedef typename Kernel::Point_3 Point_3;

  std::vector<Point_3> points;
  std::string line;
  std::istringstream iss;
  std::size_t nb_vertices(0);
  std::vector<material> materials;
  //ignore header
  int material_id = 0;
  int nb_patches = 0;
  while(std::getline(input, line))
  {
    if (line_starts_with(line, "Materials"))
    {
      if(!IO::internal::treat_surf_materials(input, materials, material_id))
        return false;
    }

    //get grid box
    if (line_starts_with(line, "GridBox"))
    {
      IO::internal::treat_surf_grid_box(line, grid_box);
    }

    //get grid size
    if (line_starts_with(line, "GridSize"))
    {
      IO::internal::treat_surf_grid_size(line, grid_size);
    }

    //get number of vertices
    if (line_starts_with(line, "Vertices"))
    {
      IO::internal::treat_surf_vertices(input, line, nb_vertices, points);
    }

    //get number of patches
    if (line_starts_with(line, "Patches"))
    {
      IO::internal::get_surf_patches(line, nb_patches, metadata, output);
      break;
    }
  }

  for(int i=0; i < nb_patches; ++i)
  {
    std::size_t nb_triangles(0);
    //get metada
    while(std::getline(input, line))
    {
      if (line_starts_with(line, "InnerRegion"))
      {
        IO::internal::treat_surf_inner_region(input, line, materials, metadata, i);
      }
      if (line_starts_with(line, "Triangles"))
      {
        IO::internal::treat_surf_triangles(line, nb_triangles);
        break;
      }
    }

    //connect triangles
    typedef std::array<std::size_t, 3> Triangle_ind;
    std::vector<Triangle_ind> polygons;
    IO::internal::connect_surf_triangles(input, nb_triangles, polygons);

    //build patch
    IO::internal::build_surf_patch(out, output, points, polygons, i);
  } // end loop on patches

  return true;
}

template<class Mesh, typename DuplicatedPointsOutIterator>
bool read_surf(std::istream& input, std::vector<Mesh>& output,
  std::vector<MaterialData>& metadata,
  CGAL::Bbox_3& grid_box,
  std::array<unsigned int, 3>& grid_size,
  DuplicatedPointsOutIterator out)
{
  return read_surf(input, output, metadata, grid_box, grid_size, out,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}

