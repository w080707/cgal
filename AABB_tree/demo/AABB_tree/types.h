#ifndef AABB_DEMO_TYPES_H
#define AABB_DEMO_TYPES_H

#include <CGAL/basic.h>
//#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
//#include <CGAL/Cartesian.h>
#include <CGAL/Simple_cartesian.h>

typedef CGAL::Simple_cartesian<double> Kernel; // fastest in experiments
//typedef CGAL::Cartesian<double> Kernel;
//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;

typedef Kernel::FT FT;
typedef Kernel::Ray_3 Ray;
typedef Kernel::Line_3 Line;
typedef Kernel::Point_3 Point;
typedef Kernel::Plane_3 Plane;
typedef Kernel::Vector_3 Vector;
typedef Kernel::Segment_3 Segment;
typedef Kernel::Triangle_3 Triangle;

#include <CGAL/Polyhedron_3.h>
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;

#endif // AABB_DEMO_TYPES_H
