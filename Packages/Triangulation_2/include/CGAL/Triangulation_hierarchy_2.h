// ============================================================================
//
// Copyright (c) 1998 The CGAL Consortium
//
// This software and related documentation is part of an INTERNAL release
// of the Computational Geometry Algorithms Library (CGAL). It is not
// intended for general use.
//
// ----------------------------------------------------------------------------
//
// release       :
// release_date  :
//
// file          : include/CGAL/Triangulation_hierarchy_2.h
// package       : Triangulation
// source        : $RCSfile$
// revision      : 
// revision_date : 
// package       : Triangulation
// author(s)     : Olivier Devillers <Olivivier.Devillers@sophia.inria.fr>
//
// coordinator   : INRIA Sophia-Antipolis (<Mariette.Yvinec@sophia.inria.fr>)
//
// ============================================================================

#ifndef TRIANGULATION_HIERARCHY_2_H
#define TRIANGULATION_HIERARCHY_2_H

#include <CGAL/Random.h>

CGAL_BEGIN_NAMESPACE

template < class Vb>
class Triangulation_hierarchy_vertex_base_2
 : public Vb
{
  typedef Vb Base;
  typedef typename Base::Point                        Point;

 public:
  Triangulation_hierarchy_vertex_base_2()
    : Base(), _up(0), _down(0)
    {}
  Triangulation_hierarchy_vertex_base_2(const Point & p, void* f)
    : Base(p,f), _up(0), _down(0)
    {}
  Triangulation_hierarchy_vertex_base_2(const Point & p)
    : Base(p), _up(0), _down(0)
    {}

 public:  // for use in Triangulation_hierarchy only
  //  friend class Triangulation_hierarchy_2;
  void* up() {return _up;}
  void* down() {return _down;}
  void set_up(void *u) {_up=u;}
  void set_down(void *d) {if (this) _down=d;}


 private:
  void* _up;    // same vertex one level above
  void* _down;  // same vertex one level below
};

// parameterization of the  hierarchy
const float Triangulation_hierarchy_2__ratio    = 30.0;
const int   Triangulation_hierarchy_2__minsize  = 20;
const int   Triangulation_hierarchy_2__maxlevel = 5;
// maximal number of points is 30^5 = 24 millions !

template < class Triangulation>
class Triangulation_hierarchy_2
: public Triangulation
{
 public:
  typedef typename Triangulation::Geom_traits  Geom_traits;
  typedef typename Geom_traits::Point_2             Point;
  typedef Triangulation                        Base;
  typedef typename Base::Vertex_handle     Vertex_handle;
  typedef typename Base::Face_handle       Face_handle;
  typedef typename Base::Vertex_iterator   Vertex_iterator;
  typedef typename Base::Vertex           Vertex;
  typedef typename Base::Locate_type       Locate_type;

 private:
  // here is the stack of triangulations which form the hierarchy
  Base*   hierarchy[Triangulation_hierarchy_2__maxlevel];
  Random random; // random generator

public:
  Triangulation_hierarchy_2(const Geom_traits& traits = Geom_traits());
  Triangulation_hierarchy_2(const Triangulation_hierarchy_2& tr);

  Triangulation_hierarchy_2 &operator=(const  Triangulation_hierarchy_2& tr);
  ~Triangulation_hierarchy_2();

  //Helping
  void copy_triangulation(const Triangulation_hierarchy_2 &tr);
  void swap(Triangulation_hierarchy_2 &tr);
  void clear();

  // CHECKING
  bool is_valid() const;

  // INSERT REMOVE
  Vertex_handle insert(const Point &p);
  Vertex_handle push_back(const Point &p);
 
  template < class InputIterator >
  int insert(InputIterator first, InputIterator last)
    {
      int n = number_of_vertices();
      while(first != last){
	insert(*first);
	++first;
      }
      return number_of_vertices() - n;
    }

  void remove_degree_3(Vertex_handle  v);
  void remove_first(Vertex_handle  v);
  void remove_second(Vertex_handle v);
  void remove(Vertex_handle  v);

  //LOCATE
  Face_handle  locate(const Point& p, Locate_type& lt,int& li) const;
  Face_handle  locate(const Point& p) const;

private:
  void  locate(const Point& p,
	       Locate_type& lt,
	       int& li,
	       Face_handle pos[Triangulation_hierarchy_2__maxlevel]) const;
  int random_level();


 // added to make the test program of usual triangulations work
  // undocuumented
public:
  
  Vertex_handle insert(const Point  &p, Face_handle start){
    return Base::insert(p,start);
  }
  Vertex_handle insert(const Point& p,
		       Locate_type lt,
		       Face_handle loc, int li ){
    return Base::insert(p);
  }

  Face_handle  locate(const Point& p, 
		      Locate_type& lt,
		      int& li,
		      Face_handle start) const{
    return Base::locate(p, lt, li, start);
  }

};



template <class Triangulation >
Triangulation_hierarchy_2<Triangulation>::
Triangulation_hierarchy_2(const Geom_traits& traits)
  : Base(traits), random((long)0)
{ 
  hierarchy[0] = this; 
  for(int i=1;i<Triangulation_hierarchy_2__maxlevel;++i)
    hierarchy[i] = new Base();
}


// copy constructor duplicates vertices and faces
template <class Triangulation>
Triangulation_hierarchy_2<Triangulation>::
Triangulation_hierarchy_2(const Triangulation_hierarchy_2<Triangulation> &tr)
    : Base(), random((long)0)
{ 
  // create an empty triangulation to be able to delete it !
  hierarchy[0] = this; 
  for(int i=1;i<Triangulation_hierarchy_2__maxlevel;++i)
    hierarchy[i] = new Base();
  copy_triangulation(tr);
} 
 

//Assignement
template <class Triangulation>
Triangulation_hierarchy_2<Triangulation> &
Triangulation_hierarchy_2<Triangulation>::
operator=(const Triangulation_hierarchy_2<Triangulation> &tr)
{
  copy_triangulation(tr);
  return *this;
}


template <class Triangulation>
void
Triangulation_hierarchy_2<Triangulation>::   
copy_triangulation(const Triangulation_hierarchy_2<Triangulation> &tr)
{
  std::map< const void*, void*, std::less<const void*> > V;

  for(int i=0;i<Triangulation_hierarchy_2__maxlevel;++i)
    hierarchy[i]->copy_triangulation(*tr.hierarchy[i]);
  //up and down have been copied in straightforward way
  // compute a map at lower level
  for( Vertex_iterator it=hierarchy[0]->vertices_begin(); 
       it != hierarchy[0]->vertices_end(); ++it) {
    if (it->up()) V[ ((Vertex*)(it->up()))->down() ] = &(*it);
      }
  for(int i=1;i<Triangulation_hierarchy_2__maxlevel;++i) {
    for( Vertex_iterator it=hierarchy[i]->vertices_begin(); 
	 it != hierarchy[i]->vertices_end(); ++it) {
      // down pointer goes in original instead in copied triangulation
      it->set_down(V[it->down()]);
      // make reverse link
      ((Vertex*)(it->down()))->set_up( &(*it) );
      // make map for next level
      if (it->up()) V[ ((Vertex*)(it->up()))->down() ] = &(*it);
    }
  }
}

template <class Triangulation>
void
Triangulation_hierarchy_2<Triangulation>:: 
swap(Triangulation_hierarchy_2<Triangulation> &tr)
{
//   Base** h= hierarchy;
//   hierarchy = tr.hierarchy;
//   tr.hierarchy = h;
  Base* temp;
  Base::swap(tr);
  for(int i= 1; i<Triangulation_hierarchy_2__maxlevel; ++i){
    temp = hierarchy[i];
    hierarchy[i] = tr.hierarchy[i];
    tr.hierarchy[i]= temp;
  }
}

template <class Triangulation>
Triangulation_hierarchy_2<Triangulation>:: 
~Triangulation_hierarchy_2()
{
  clear();
  for(int i= 1; i<Triangulation_hierarchy_2__maxlevel; ++i){ 
    delete hierarchy[i];
  }
}

template <class Triangulation>
void
Triangulation_hierarchy_2<Triangulation>:: 
clear()
{
        for(int i=0;i<Triangulation_hierarchy_2__maxlevel;++i)
	hierarchy[i]->clear();
}


template <class Triangulation>
bool
Triangulation_hierarchy_2<Triangulation>:: 
is_valid() const
{
  bool result = true;
  int i;
  Vertex_iterator it;
  //verify correctness of triangulation at all levels
  for(i=0;i<Triangulation_hierarchy_2__maxlevel;++i)
	result = result && hierarchy[i]->is_valid();
  //verify that lower level has no down pointers
  for( it = hierarchy[0]->vertices_begin(); 
       it != hierarchy[0]->vertices_end(); ++it) 
    result = result && ( it->down() == 0 );
  //verify that other levels has down pointer and reciprocal link is fine
  for(i=1;i<Triangulation_hierarchy_2__maxlevel;++i)
    for( it = hierarchy[i]->vertices_begin(); 
	 it != hierarchy[i]->vertices_end(); ++it) 
      result = result && 
	       ( ((Vertex*)((Vertex*)it->down())->up()) ==  &(*it) );
  return result;
}

  
template <class Triangulation >
Triangulation_hierarchy_2<Triangulation>::Vertex_handle
Triangulation_hierarchy_2<Triangulation>::
insert(const Point &p)
{
  int vertex_level = random_level();
  Locate_type lt;
  int i;
  // locate using hierarchy
  Face_handle positions[Triangulation_hierarchy_2__maxlevel];
  locate(p,lt,i,positions);
  //insert at level 0
  Vertex_handle vertex=hierarchy[0]->insert(p,positions[0]);
  Vertex_handle previous=vertex;
  Vertex_handle first = vertex;
      
  int level  = 1;
  while (level <= vertex_level ){
    vertex=hierarchy[level]->insert(p,positions[level]);
    vertex->set_down((void *) &*previous);// link with level above
    previous->set_up((void *) &*vertex);
    previous=vertex;
    level++;
  }
  return first;
}

template <class Triangulation >
inline
Triangulation_hierarchy_2<Triangulation>::Vertex_handle
Triangulation_hierarchy_2<Triangulation>::
push_back(const Point &p)
{
  return insert(p);
}

template <class Triangulation >
void 
Triangulation_hierarchy_2<Triangulation>::
remove(Vertex_handle v )
{
  void * u=v->up();
  int l = 0 ;
  while(1){
    hierarchy[l++]->remove(v);
    if (!u) break; 
    if(l>Triangulation_hierarchy_2__maxlevel) break;
    v=(Vertex*)u; u=v->up();
  }
}

template <class Triangulation >
inline void 
Triangulation_hierarchy_2<Triangulation>::
remove_degree_3(Vertex_handle v )
{
  remove(v);
}

template <class Triangulation >
inline void 
Triangulation_hierarchy_2<Triangulation>::
remove_first(Vertex_handle v )
{
  remove(v);
}

template <class Triangulation >
inline void 
Triangulation_hierarchy_2<Triangulation>::
remove_second(Vertex_handle v )
{
  remove(v);
}

template <class Triangulation >
Triangulation_hierarchy_2<Triangulation>::Face_handle 
Triangulation_hierarchy_2<Triangulation>::
locate(const Point& p, Locate_type& lt, int& li) const
{
  Face_handle positions[Triangulation_hierarchy_2__maxlevel];
  locate(p,lt,li,positions);
  return positions[0];
}

template <class Triangulation >
Triangulation_hierarchy_2<Triangulation>::Face_handle 
Triangulation_hierarchy_2<Triangulation>::
locate(const Point& p) const
{
  Locate_type lt;
  int li;
  return locate(p, lt, li);
}

template <class Triangulation >
void
Triangulation_hierarchy_2<Triangulation>::
locate(const Point& p,
       Locate_type& lt,
       int& li,
       Face_handle pos[Triangulation_hierarchy_2__maxlevel]) const
{
  Face_handle position;
  Vertex_handle nearest;
  int level  = Triangulation_hierarchy_2__maxlevel;
  typename Geom_traits::Less_distance_to_point_2 
    closer = geom_traits().less_distance_to_point_2_object(p);

  // find the highest level with enough vertices
  while (hierarchy[--level]->number_of_vertices() 
	 < Triangulation_hierarchy_2__minsize){
    if ( ! level) break;  // do not go below 0
  }
  for (int i=level+1; i<Triangulation_hierarchy_2__maxlevel;++i) pos[i]=0;
  while(level > 0) {
    pos[level]=position=hierarchy[level]->locate(p,position);  
    // locate at that level from "position"
    // result is stored in "position" for the next level
    // find the nearest between vertices 0 and 1
    if (hierarchy[level]->is_infinite(position->vertex(0)))
      nearest = position->vertex(1);
    else if (hierarchy[level]->is_infinite(position->vertex(1)))
      nearest = position->vertex(0);
     else if ( closer(position->vertex(0)->point(),
		      position->vertex(1)->point()))
      nearest = position->vertex(0);
    else
      nearest = position->vertex(1);
    // compare to vertex 2
    if ( !  hierarchy[level]->is_infinite(position->vertex(2)))
      if ( closer( position->vertex(2)->point(),
		   nearest->point()))
	nearest = position->vertex(2);
    // go at the same vertex on level below
    nearest = (Vertex*)( nearest->down() );
    position = nearest->face();                // incident face
    --level;
  }
  pos[0]=hierarchy[level]->locate(p,lt,li,position);  // at level 0
}


template <class Triangulation >
int
Triangulation_hierarchy_2<Triangulation>::
random_level()
{
  int l = 0;
  while (1) {
    if ( random(Triangulation_hierarchy_2__ratio) ) break;
    ++l;
  }
  if (l >= Triangulation_hierarchy_2__maxlevel)
    l = Triangulation_hierarchy_2__maxlevel -1;
  return l;
}

CGAL_END_NAMESPACE
#endif //TRIANGULATION_HIERARCHY_2_H
