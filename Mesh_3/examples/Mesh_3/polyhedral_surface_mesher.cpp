#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Robust_circumcenter_traits_3.h>
#include <CGAL/Regular_triangulation_3.h>

#include <CGAL/Regular_triangulation_cell_base_3.h>

#include <CGAL/Mesh_3/IO.h>

#include <CGAL/make_surface_mesh.h>
#include <CGAL/Polyhedral_surface_3.h>

#include <CGAL/Volume_mesher_cell_base_3.h>
#include <CGAL/Regular_triangulation_euclidean_traits_3.h>
#include <CGAL/Regular_triangulation_filtered_traits_3.h>
#include <CGAL/Implicit_surfaces_mesher_3.h>

#include <CGAL/Surface_mesher/Standard_criteria.h>
#include <CGAL/Surface_mesher/Vertices_on_the_same_surface_criterion.h>
#include <CGAL/Mesh_3/Slivers_exuder.h>

#include <CGAL/IO/Complex_2_in_triangulation_3_file_writer.h>
#include <CGAL/IO/File_medit.h>

#include <CGAL/Surface_mesher/Point_surface_indices_oracle_visitor.h>

#include <CGAL/Mesh_criteria_3.h>

#include <CGAL/Point_traits.h>
#include <CGAL/Weighted_point_with_surface_index_geom_traits.h>

#include "parameters.h"

#include "debug.h"
#include <CGAL/Timer.h>

#include <iostream>
#include <fstream>

#include <sstream>

#include <boost/tuple/tuple.hpp> // boost::tie

using boost::tie;

/////////////// Types /////////////// 

struct K2 : public CGAL::Exact_predicates_inexact_constructions_kernel {};
typedef CGAL::Robust_circumcenter_traits_3<K2>  K;
typedef CGAL::Regular_triangulation_filtered_traits_3<K> Regular_traits;
typedef CGAL::Weighted_point_with_surface_index_geom_traits<Regular_traits> My_traits;
typedef CGAL::Complex_2_in_triangulation_vertex_base_3<My_traits> Vb;
typedef CGAL::Regular_triangulation_cell_base_3<My_traits> Cb1;
typedef CGAL::Surface_mesh_cell_base_3<My_traits, Cb1> Cb2;
typedef CGAL::Volume_mesher_cell_base_3<My_traits, Cb2> Cb;
typedef CGAL::Triangulation_data_structure_3<Vb, Cb> Tds;
typedef CGAL::Regular_triangulation_3<My_traits, Tds> Tr;

typedef My_traits::Point_3 Point_3;
typedef My_traits::Sphere_3 Sphere_3;
typedef My_traits::FT FT;

typedef CGAL::Polyhedral_surface_3<My_traits> Surface;


// typedef CGAL::Triangulation_vertex_base_3<K> Del_Vb;
// typedef CGAL::Triangulation_cell_base_3<K> Del_Cb1;
// typedef CGAL::Mesh_3::Complex_2_in_triangulation_cell_base_3<K, Del_Cb1> Del_Cb;
// typedef CGAL::Triangulation_data_structure_3<Del_Vb, Del_Cb> Del_Tds;
// typedef CGAL::Delaunay_triangulation_3<K, Del_Tds> Del_tr;

typedef CGAL::Surface_mesher::Refine_criterion<Tr> Criterion;
typedef CGAL::Surface_mesher::Standard_criteria <Criterion > Criteria;
typedef CGAL::Mesh_criteria_3<Tr> Tets_criteria;

typedef CGAL::Complex_2_in_triangulation_3<Tr> C2t3;

typedef CGAL::Surface_mesher::Point_surface_indices_visitor Volume_mesher_traits_visitor;

typedef CGAL::Surface_mesher::Polyhedral_oracle<
  Surface,
  CGAL::Creator_uniform_3<My_traits::FT, My_traits::Point_3>,
  Volume_mesher_traits_visitor> Volume_mesh_traits;

typedef Volume_mesh_traits::Construct_initial_points Initial_points;

typedef CGAL::Implicit_surfaces_mesher_3<
  C2t3, 
  Surface,
  Criteria,
  Tets_criteria,
  Volume_mesh_traits> Mesher;

typedef CGAL::Simple_cartesian<double> Simple_kernel;
typedef Simple_kernel::Iso_rectangle_2 Rectangle_2;
typedef Simple_kernel::Segment_2 Segment_2;
typedef Simple_kernel::Point_2 Point_2;

typedef CGAL::Point_traits<Point_3> Point_traits;
typedef Point_traits::Bare_point Bare_point_3;
typedef Regular_traits::Point_3 Kernel_point_3;

/// Global variables 
std::ostream *out = 0;
std::string filename = std::string();
std::string function_name = "sphere";
char* argv0 = "";

void usage(std::string error = "")
{
  if( error != "" )
    std:: cerr << "Error: " << error << std::endl;
  std::cerr << "Usage:\n  " 
            << argv0
            << " [-f function_name]"
            << " [output_file.mesh|-]\n"
            << "If output_file.mesh is '-', outputs to standard out.\n"
            << "-f  define the implicite function to use\n";
  for(String_options::iterator it = string_options.begin();
      it != string_options.end();
      ++it)
    std::cerr << "--" << it->first << " default value is \""
	      << it->second << "\".\n";
  for(Double_options::iterator it = double_options.begin();
      it != double_options.end();
      ++it)
    std::cerr << "--" << it->first << " default value is "
	      << it->second << ".\n";
  exit(EXIT_FAILURE);
}

std::pair<std::ostream*, const bool>
open_file_for_writing(std::string filename,
                      std::string display_string = "Writing to ")
{
  if( filename != "")
  {
    if( filename == "-" )
    {
      std::cerr << display_string << "standard out...\n";
      return std::make_pair(&std::cout, false);
    }
    else 
    {
      std::ofstream* result = new std::ofstream(filename.c_str());
      if( *result )
      {
        std::cerr << display_string << "file " << filename << "...\n";
        return std::make_pair(result, true);
      }
      else 
      {
        delete result;
        std::cerr << "Error: cannot create " << filename << "\n";
        usage();
        return std::pair<std::ostream*, bool>(0, false);
      }
    }
  }
  else 
    return std::pair<std::ostream*, bool>(0, false);
}

void parse_argv(int argc, char** argv, int extra_args = 0)
{
  if (argc >=(2 + extra_args))
    {
      std::string arg = argv[1+extra_args];
      if( arg == "-h" || arg == "--help")
        usage();
      else if( arg == "-f" )
        {
          if( argc < (3 + extra_args) )
            usage("-f must be followed by a function name!");
          function_name = argv[2 + extra_args];
          parse_argv(argc, argv, extra_args + 2);
        }
      else if( arg.substr(0, 2) == "--" )
	{
	  Double_options::iterator opt_it = 
	    double_options.find(arg.substr(2, arg.length()-2));
	  if( opt_it != double_options.end() )
	    {
	      if( argc < (3 + extra_args) )
		usage((arg + " must be followed by a double!").c_str());
	      std::stringstream s;
	      double val;
	      s << argv[extra_args + 2];
	      s >> val;
	      if( !s )
		usage(("Bad double after " + arg + "!").c_str());
	      opt_it->second = val;
	      parse_argv(argc, argv, extra_args + 2);
	    }
	  else
          {
            String_options::iterator opt_it = 
                string_options.find(arg.substr(2, arg.length()-2));
            if( opt_it != string_options.end() )
            {
              if( argc < (3 + extra_args) )
                usage((arg + " must be followed by a string!").c_str());
              std::string s = argv[extra_args + 2];
              opt_it->second = s;
              parse_argv(argc, argv, extra_args + 2);
            }
            else
              usage(("Invalid option " + arg).c_str());
          }
	}
      else 
	{
	  filename = argv[1+extra_args];
	  parse_argv(argc, argv, extra_args + 1);
	}
    }
}

/////////////// Main function /////////////// 

int main(int argc, char **argv) {
  argv0 = argv[0];

  init_parameters();
  usage_ptr = &usage;

  parse_argv(argc, argv);

  std::ifstream surface_ifs(function_name.c_str());
  Surface surface(surface_ifs);
  surface_ifs.close();

  // 2D-complex in 3D-Delaunay triangulation
  Tr tr;
  C2t3 c2t3(tr);

  CGAL::Timer timer;

  bool need_delete = false;
  std::ostream* out = 0;

  // Create the volume_mesh_traits by hand, to pass it
  // Point_surface_indices_visitor(1), that is a visitor for oracles that
  // sets surface_index() to a given integer.
  Volume_mesh_traits volume_mesh_traits(Volume_mesher_traits_visitor(1));

  // Initial point sample
  std::string read_initial_points = get_string_option("read_initial_points");
  if( read_initial_points != "")     
  {
    std::ifstream in( read_initial_points.c_str() );
    int n;
    in >> n;
    CGAL_assertion(in);
    while( !in.eof() )
      {
	Point_3 p;
	if(in >> p)
	  {
	    tr.insert(p);
	    --n;
	  }
      }
    CGAL_assertion( n == 0 );
    double_options["number_of_initial_points"] = 0;
  }
  else
  {
    const int number_of_initial_points = 
      static_cast<int>(get_double_option("number_of_initial_points"));

    std::vector<Point_3> initial_point_sample;
    initial_point_sample.reserve(number_of_initial_points);

    Initial_points get_initial_points =
      volume_mesh_traits.construct_initial_points_object();

    get_initial_points(surface, 
                       std::back_inserter(initial_point_sample),
                       number_of_initial_points);

    tie(out, need_delete) = 
      open_file_for_writing(get_string_option("dump_of_initial_points"),
                            "Writing initial points to ");
    if( out )
    {
      *out << initial_point_sample.size() << "\n";
      for(std::vector<Point_3>::const_iterator it = 
            initial_point_sample.begin();
          it != initial_point_sample.end();
          ++it)
        *out << *it <<"\n";
      if(need_delete) 
        delete out;
    }
    tr.insert (initial_point_sample.begin(), initial_point_sample.end());
  }

  // Meshing criteria
  CGAL::Surface_mesher::Curvature_size_criterion<Tr> 
    curvature_size_criterion (get_double_option("distance_bound"));
  CGAL::Surface_mesher::Uniform_size_criterion<Tr>
    uniform_size_criterion (get_double_option("radius_bound"));
  CGAL::Surface_mesher::Aspect_ratio_criterion<Tr>
    aspect_ratio_criterion (get_double_option("angle_bound"));
  CGAL::Surface_mesher::Vertices_on_the_same_surface_criterion<Tr>
    vertices_on_the_same_surface_criterion;

  std::vector<Criterion*> criterion_vector;
  criterion_vector.push_back(&aspect_ratio_criterion);
  criterion_vector.push_back(&uniform_size_criterion);
  criterion_vector.push_back(&curvature_size_criterion);
  criterion_vector.push_back(&vertices_on_the_same_surface_criterion);
  Criteria criteria (criterion_vector);

  Tets_criteria tets_criteria(get_double_option("tets_aspect_ratio_bound"),
			      get_double_option("tets_size_bound"));

  std::cerr << "\nInitial number of points: " << tr.number_of_vertices() 
            << std::endl;

  
  // Surface meshing

  Mesher mesher (c2t3, surface, criteria, tets_criteria, volume_mesh_traits);
  timer.start();
  mesher.refine_surface();
  timer.stop();
  std::cerr << "\nNumber of points after refine_surface(): "
            << tr.number_of_vertices() << std::endl
            << "Elapsed time: " << timer.time() << std::endl;

  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("initial_surface_off"),
                          "Writing initial surface off to ");
  if( out )
  {
    CGAL::output_oriented_surface_facets_to_off(*out, tr);
    if(need_delete) 
      delete out;
  }
  timer.start();
  mesher.refine_mesh();
  timer.stop();

  std::cout.flush();

  std::cerr << "\nFinal number of points: " << tr.number_of_vertices() 
            << std::endl
            << "Total time: " << timer.time() << std::endl;

  tie(out, need_delete) = 
    open_file_for_writing(filename,
                          "Writing medit mesh before exudation to ");
  if( out )
  {
    CGAL::output_to_medit(*out, mesher.complex_2_in_triangulation_3());
    if(need_delete) 
      delete out;
  }

  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("cgal_mesh_before_exudation"),
                          "Writing cgal mesh before exudation to ");
  if( out )
  {
    CGAL::Mesh_3::output_mesh(*out, mesher.complex_2_in_triangulation_3());
    if(need_delete) 
      delete out;
  }

  CGAL::Mesh_3::Slivers_exuder<C2t3> exuder(tr);
  exuder.pump_vertices(get_double_option("pumping_bound"));
  
  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("cgal_mesh_after_filename"),
                          "Writing cgal mesh after exudation to ");
  if( out )
  {
    CGAL::Mesh_3::output_mesh(*out, mesher.complex_2_in_triangulation_3());
    if(need_delete) 
      delete out;
  }

  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("mesh_after_filename"),
                          "Writing medit mesh after exudation to ");
  if( out )
  {
    CGAL::output_to_medit(*out, mesher.complex_2_in_triangulation_3());
    if(need_delete) 
      delete out;
  }

  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("surface_off"),
                          "Writing finale surface off to ");
  if( out )
  {
    CGAL::output_oriented_surface_facets_to_off(*out, tr);
    if(need_delete) 
      delete out;
  }
//   {
//     std::string dump_final_surface_filename = get_string_option("surface_ghs");
//     if( dump_final_surface_filename != "" )
//       {
// 	std::ofstream dump_points((dump_final_surface_filename +
// 				   ".points").c_str());
// 	std::ofstream dump_faces((dump_final_surface_filename + 
// 				  ".faces").c_str());
// 	if( dump_points && dump_faces ) {
// 	  std::cerr << "Writing final surface to ghs file "
// 		    << dump_final_surface_filename << "..." << std::endl;
// 	  output_surface_facets_to_ghs(dump_points, dump_faces, tr);
// 	}
// 	else
// 	  usage(("Error: cannot create " + 
// 			  dump_final_surface_filename).c_str());
//       }
//   }
  
  tie(out, need_delete) = 
    open_file_for_writing(get_string_option("slivers_off"),
                          "Writing slivers off to ");
  if( out )
  {
    CGAL::output_slivers_to_off(*out, tr, get_double_option("sliver_test"));
    if(need_delete) 
      delete out;
  }

  std::cerr << " done\n";

#ifdef CGAL_SURFACE_MESHER_TEST_OPTIONS
  check_all_options_have_been_used();
#endif
}
