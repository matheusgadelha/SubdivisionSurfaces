#ifndef MESH_H_
#define MESH_H_

#define _USE_MATH_DEFINES

#include "linalgebra.hpp"
#include <utility>
#include <vector>
#include <list>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

const int POLY_SIZE = 3;

template <typename V, typename H>
struct THalfedge;

template <typename V, typename H>
class Mesh;

template <typename V, typename H>
struct TVertex
{
	TVertex (Vector3f pos){ position = pos; }
	TVertex (){}

	typename std::list<THalfedge<V,H> >::iterator outHalfedge;
	int id;
	V data;
	Vector3f position;

	Mesh<V,H>* mesh_ref;
};

template <typename A, typename B>
std::vector<std::pair<A,B> > getAllPairsWithSecond (std::vector<std::pair<A,B> >& v, B b)
{
	std::vector<std::pair<A,B> > result;
	for (int i=0; i<v.size(); ++i)
		if (v[i].second == b) result.push_back (v[i]);

	return result;
}

template <typename A, typename B>
std::vector<std::pair<A,B> > getAllPairsWithFirst (std::vector<std::pair<A,B> >& v, A a)
{
	std::vector<std::pair<A,B> > result;
	for (int i=0; i<v.size(); ++i)
		if (v[i].first == a) result.push_back (v[i]);

	return result;
}


template <typename V, typename H>
struct TFace
{
	TFace() : halfedge(NULL){}
	TFace (typename std::list<THalfedge<V,H> >::iterator he) : halfedge(he){}

	typename std::list<THalfedge<V,H> >::iterator halfedge;
	int id;

	Mesh<V,H>* mesh_ref;
};

template <typename V, typename H>
struct THalfedge
{
	THalfedge(){}

	typename std::list<TVertex<V,H> >::iterator sink;
	typename std::list<TFace<V,H> >::iterator face;
	typename std::list<THalfedge<V,H> >::iterator next;
	typename std::list<THalfedge<V,H> >::iterator opposite;
	typename std::list<THalfedge<V,H> >::iterator prev;

	typename std::list<THalfedge<V,H> >::iterator nextOverLine()
	{
		auto result = next;
		if (next->opposite->face == opposite->face) return result->next;
		else return result;
	}
	
	typename std::list<THalfedge<V,H> >::iterator prevOverLine()
	{
		auto result = prev;
		if (prev->opposite->face == opposite->face) return result->prev;
		else return result;
	}

	int id;

	H data;

	Mesh<V,H>* mesh_ref;
};

template <typename V, typename H>
class Mesh
{
	public:
		Mesh(){}

		void addVertex (TVertex<V,H> v)
		{
			v.mesh_ref = this;
			v.id = vertices.size();
			vertices.push_back(v);
		}

		void addHalfedge (THalfedge<V,H> h)
		{
			h.mesh_ref = this;
			h.id = halfedges.size();
			halfedges.push_back(h);
		}

		void addFace (TFace<V,H> f)
		{
			f.mesh_ref = this;
			f.id = faces.size();
			faces.push_back(f);
		}

		void generateMesh (std::vector<Vector3f>& raw_vertices, std::vector<int>& indices)
		{
			std::vector<std::pair<int,int> >edges;
			std::vector<int> edge_he;

			for (size_t i=0; i<raw_vertices.size(); ++i)
			{
				addVertex (TVertex<V,H> (raw_vertices[i]));
			}

			for (size_t i=0; i<indices.size(); ++i)
			{
				THalfedge<V,H> new_halfedge;
				addHalfedge (new_halfedge);
				typename std::list<THalfedge<V,H> >::iterator h = std::prev(halfedges.end(),1);

				if (i % POLY_SIZE != 0)
				{
					h->prev= std::prev(halfedges.end(),2);
					h->prev->next = h;
				}

				std::next(vertices.begin(), indices[i])->outHalfedge = h;
				std::pair<int,int> new_edge;
				if ((i+1) % POLY_SIZE != 0)
				{
					h->sink = std::next(vertices.begin(),indices[i+1]);
					new_edge = std::make_pair (indices[i], indices[i+1]);
				}
				else 
				{
					h->sink = std::next(vertices.begin(),indices[i+1-POLY_SIZE]);
					new_edge = std::make_pair (indices[i], indices[i+1-POLY_SIZE]);
				}
				edges.push_back(new_edge);
				edge_he.push_back(h->id);
				int pos = -1;
				for (size_t e=0; e<edges.size(); ++e)
				{
					if (edges[e] == std::make_pair (new_edge.second, new_edge.first))
						pos = e;
				}

				if( pos != -1 )
				{
					std::next(halfedges.begin(),edge_he[pos])->opposite = h;
					h->opposite = std::next(halfedges.begin(),edge_he[pos]);
				}

				if (((i+1) % POLY_SIZE) == 0)
				{
					addFace (TFace<V,H>(h));

					typename std::list<THalfedge<V,H> >::iterator it = h;
					for (int i=0; i<POLY_SIZE-1; ++i)
					{
						it->face = std::prev(faces.end(),1);
						it = it->prev;
					}
					h->next = it;
					h->next->prev = h;
					h->next->face = std::prev(faces.end(),1);
				}
			}
		}

		int loopSubdivision()
		{
			std::vector<int> splitted;
			size_t num_halfedges = halfedges.size();
			size_t old_verts = vertices.size();

			std::vector< std::pair<typename std::list<TVertex<V,H> >::iterator,int> > new_vertices_faces;
			std::vector< std::pair<typename std::list<THalfedge<V,H> >::iterator,int> > new_halfedges_faces;
			std::map< int, std::vector<typename std::list<TVertex<V,H> >::iterator> > vert_one_ring;

			for (auto it = vertices.begin(); it!=vertices.end(); it++)
			{
				vert_one_ring[it->id] = getOneRing(it);
			}

			for (size_t i=0; i<num_halfedges; ++i)
			{
				if (std::find (splitted.begin(), splitted.end(), i) == splitted.end())
				{
					auto nvert = loopSplitEdge(i);
					new_vertices_faces.push_back (std::make_pair(nvert, nvert->outHalfedge->face->id));
					new_vertices_faces.push_back (std::make_pair(nvert, nvert->outHalfedge->opposite->face->id));
					new_halfedges_faces.push_back (std::make_pair(nvert->outHalfedge, nvert->outHalfedge->face->id));
					splitted.push_back(i);
					splitted.push_back( std::next(halfedges.begin(),i)->opposite->id);
				}
			}
			
			faces.clear();

			std::vector<typename std::list<TVertex<V,H> >::iterator> vstart;
			std::vector<typename std::list<TVertex<V,H> >::iterator> vend;
			std::vector<typename std::list<THalfedge<V,H> >::iterator> hedges;
			num_halfedges = halfedges.size();

			for (auto vit = vertices.begin(); vit != std::next(vertices.begin(), old_verts); vit++)
			{
				typename std::list<THalfedge<V,H> >::iterator it = vit->outHalfedge;
				do{
					addHalfedge (THalfedge<V,H>());
					typename std::list<THalfedge<V,H> >::iterator nhe = std::prev(halfedges.end(),1);
					nhe->prev = it;
					nhe->next = it->prev;
					nhe->sink = nhe->next->prev->sink;
					nhe->prev->next = nhe;
					nhe->next->prev = nhe;
					nhe->opposite = halfedges.end();

					addFace (TFace<V,H>(nhe));
					typename std::list<TFace<V,H> >::iterator nf = std::prev(faces.end(),1);
					nhe->face = nf;
					nhe->next->face = nf;
					nhe->next->next->face = nf;

					it = nhe->next->opposite;
				} while (it != vit->outHalfedge);
			}

			std::vector< std::pair<typename std::list<TVertex<V,H> >::iterator,int> > query;
			std::vector<int> filled_faces;
			for (size_t i=0; i< new_vertices_faces.size(); i++)
			{
				if (std::find (filled_faces.begin(), filled_faces.end(), new_vertices_faces[i].second) != filled_faces.end())
					continue;
				query = getAllPairsWithSecond (new_vertices_faces, new_vertices_faces[i].second);
				assert (query.size() == 3);

				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he1 = std::prev(halfedges.end(),1);
				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he2 = std::prev(halfedges.end(),1);
				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he3 = std::prev(halfedges.end(),1);

				typename std::list<THalfedge<V,H> >::iterator it = query[0].first->outHalfedge;
				if (it->next->sink != query[1].first && it->next->sink != query[2].first)
				{
					it = it->opposite;
					it = it->next;
				}
				else
				{
					it = it->next->next;
				}

				he1->next = he2;
				he1->prev = he3;
				he1->sink = it->prev->sink;
				he2->next = he3;
				he2->prev = he1;
				he3->next = he1;
				he3->prev = he2;
				he3->sink = it->sink;

				if (he3->sink != query[0].first && he1->sink != query[0].first)
					he2->sink = query[0].first;
				else if (he3->sink != query[1].first && he1->sink != query[1].first)
					he2->sink = query[1].first;
				else he2->sink = query[2].first;

				assert( he1->sink == query[0].first || he1->sink == query[1].first || he1->sink == query[2].first);
				assert( he3->sink == query[0].first || he3->sink == query[1].first || he3->sink == query[2].first);

				addFace (TFace<V,H>(he1));
				typename std::list<TFace<V,H> >::iterator nf = std::prev(faces.end(),1);
				he1->face = he2->face = he3->face = nf;
				filled_faces.push_back (new_vertices_faces[i].second);
			}

			for (auto it = halfedges.begin(); it!=halfedges.end(); it++)
				updateOpposite (it);

			for (auto it = vertices.begin(); it!=std::next(vertices.begin(),old_verts); it++)
			{
				auto oneRing = vert_one_ring[it->id];
				int n = oneRing.size();
				float alpha_n = alpha (n);
				Vector3f new_pos = alpha_n*it->position;

				Vector3f sum({0,0,0}) ;
				for (size_t pj = 0; pj<oneRing.size(); ++pj)
				{
					sum = sum + oneRing[pj]->position;
				}

				new_pos = new_pos + ((1-alpha_n)/n)*sum;
				it->position = new_pos;
			}

			return 0;
		}

		int butterflySubdivision()
		{
			std::vector<int> splitted;
			size_t num_halfedges = halfedges.size();
			size_t old_verts = vertices.size();

			std::vector< std::pair<typename std::list<TVertex<V,H> >::iterator,int> > new_vertices_faces;
			std::vector< std::pair<typename std::list<THalfedge<V,H> >::iterator,int> > new_halfedges_faces;
			std::map< int, std::vector<typename std::list<TVertex<V,H> >::iterator> > vert_one_ring;

			for (size_t i=0; i<num_halfedges; ++i)
			{
				if (std::find (splitted.begin(), splitted.end(), i) == splitted.end())
				{
					auto nvert = butterflySplitEdge(i);
					new_vertices_faces.push_back (std::make_pair(nvert, nvert->outHalfedge->face->id));
					new_vertices_faces.push_back (std::make_pair(nvert, nvert->outHalfedge->opposite->face->id));
					new_halfedges_faces.push_back (std::make_pair(nvert->outHalfedge, nvert->outHalfedge->face->id));
					splitted.push_back(i);
					splitted.push_back( std::next(halfedges.begin(),i)->opposite->id);
				}
			}
			
			faces.clear();

			std::vector<typename std::list<TVertex<V,H> >::iterator> vstart;
			std::vector<typename std::list<TVertex<V,H> >::iterator> vend;
			std::vector<typename std::list<THalfedge<V,H> >::iterator> hedges;
			num_halfedges = halfedges.size();

			for (auto vit = vertices.begin(); vit != std::next(vertices.begin(), old_verts); vit++)
			{
				typename std::list<THalfedge<V,H> >::iterator it = vit->outHalfedge;
				do{
					addHalfedge (THalfedge<V,H>());
					typename std::list<THalfedge<V,H> >::iterator nhe = std::prev(halfedges.end(),1);
					nhe->prev = it;
					nhe->next = it->prev;
					nhe->sink = nhe->next->prev->sink;
					nhe->prev->next = nhe;
					nhe->next->prev = nhe;
					nhe->opposite = halfedges.end();

					addFace (TFace<V,H>(nhe));
					typename std::list<TFace<V,H> >::iterator nf = std::prev(faces.end(),1);
					nhe->face = nf;
					nhe->next->face = nf;
					nhe->next->next->face = nf;

					it = nhe->next->opposite;
				} while (it != vit->outHalfedge);
			}

			std::vector< std::pair<typename std::list<TVertex<V,H> >::iterator,int> > query;
			std::vector<int> filled_faces;
			for (size_t i=0; i< new_vertices_faces.size(); i++)
			{
				if (std::find (filled_faces.begin(), filled_faces.end(), new_vertices_faces[i].second) != filled_faces.end())
					continue;
				query = getAllPairsWithSecond (new_vertices_faces, new_vertices_faces[i].second);
				assert (query.size() == 3);

				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he1 = std::prev(halfedges.end(),1);
				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he2 = std::prev(halfedges.end(),1);
				addHalfedge (THalfedge<V,H>());
				typename std::list<THalfedge<V,H> >::iterator he3 = std::prev(halfedges.end(),1);

				typename std::list<THalfedge<V,H> >::iterator it = query[0].first->outHalfedge;
				if (it->next->sink != query[1].first && it->next->sink != query[2].first)
				{
					it = it->opposite;
					it = it->next;
				}
				else
				{
					it = it->next->next;
				}

				he1->next = he2;
				he1->prev = he3;
				he1->sink = it->prev->sink;
				he2->next = he3;
				he2->prev = he1;
				he3->next = he1;
				he3->prev = he2;
				he3->sink = it->sink;

				if (he3->sink != query[0].first && he1->sink != query[0].first)
					he2->sink = query[0].first;
				else if (he3->sink != query[1].first && he1->sink != query[1].first)
					he2->sink = query[1].first;
				else he2->sink = query[2].first;

				assert( he1->sink == query[0].first || he1->sink == query[1].first || he1->sink == query[2].first);
				assert( he3->sink == query[0].first || he3->sink == query[1].first || he3->sink == query[2].first);

				addFace (TFace<V,H>(he1));
				typename std::list<TFace<V,H> >::iterator nf = std::prev(faces.end(),1);
				he1->face = he2->face = he3->face = nf;
				filled_faces.push_back (new_vertices_faces[i].second);
			}

			for (auto it = halfedges.begin(); it!=halfedges.end(); it++)
				updateOpposite (it);

			return 0;
		}

		inline float alpha (int n)
		{
			return (3.f/8.f) + ((3.f/8.f) + (1.f/4.f)*cos(2*M_PI/n))*((3.f/8.f) + (1.f/4.f)*cos(2*M_PI/n));
		}

		typename std::list<TVertex<V,H> >::iterator splitEdge (size_t he_id)
		{
			typename std::list<TVertex<V,H> >::iterator src_vertex, dst_vertex;
			typename std::list<THalfedge<V,H> >::iterator he = std::next(halfedges.begin(),he_id);
			typename std::list<THalfedge<V,H> >::iterator he_op = he->opposite;

			src_vertex = he->prev->sink;
			dst_vertex = he->sink;

			addVertex (Vertex (0.5f * (src_vertex->position + dst_vertex->position)));
			typename std::list<TVertex<V,H> >::iterator created_vertex = std::prev(vertices.end(),1);
			created_vertex->outHalfedge = he_op;
			he->sink = created_vertex;
			src_vertex->outHalfedge = he;

			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_go = std::prev(halfedges.end(),1);
			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_op = std::prev(halfedges.end(),1);

			new_he_go->opposite = new_he_op;
			new_he_op->opposite = new_he_go;

			new_he_go->sink = dst_vertex;
			new_he_op->sink = created_vertex;

			new_he_go->next = he->next;
			new_he_go->next->prev = new_he_go;
			new_he_op->next = he_op;

			new_he_go->prev = he;
			new_he_op->prev = he_op->prev;
			new_he_op->prev->next = new_he_op;

			new_he_go->face = he->face;
			new_he_op->face = he_op->face;

			he->next = new_he_go;
			he_op->prev = new_he_op;
			dst_vertex->outHalfedge = new_he_op;

			return created_vertex;
		}

		typename std::list<TVertex<V,H> >::iterator loopSplitEdge (size_t he_id)
		{
			typename std::list<TVertex<V,H> >::iterator src_vertex, dst_vertex, far_vertex1, far_vertex2;
			typename std::list<THalfedge<V,H> >::iterator he = std::next(halfedges.begin(),he_id);
			typename std::list<THalfedge<V,H> >::iterator he_op = he->opposite;

			src_vertex = he->prev->sink;
			dst_vertex = he->sink;

			far_vertex1 = he->nextOverLine()->sink;
			far_vertex2 = he_op->nextOverLine()->sink;

			addVertex (Vertex ((3.f/8.f) * (src_vertex->position + dst_vertex->position)
						+ (1.f/8.f) * (far_vertex1->position + far_vertex2->position)));

			typename std::list<TVertex<V,H> >::iterator created_vertex = std::prev(vertices.end(),1);
			created_vertex->outHalfedge = he_op;
			he->sink = created_vertex;
			src_vertex->outHalfedge = he;

			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_go = std::prev(halfedges.end(),1);
			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_op = std::prev(halfedges.end(),1);

			new_he_go->opposite = new_he_op;
			new_he_op->opposite = new_he_go;

			new_he_go->sink = dst_vertex;
			new_he_op->sink = created_vertex;

			new_he_go->next = he->next;
			new_he_go->next->prev = new_he_go;
			new_he_op->next = he_op;

			new_he_go->prev = he;
			new_he_op->prev = he_op->prev;
			new_he_op->prev->next = new_he_op;

			new_he_go->face = he->face;
			new_he_op->face = he_op->face;

			he->next = new_he_go;
			he_op->prev = new_he_op;
			dst_vertex->outHalfedge = new_he_op;

			return created_vertex;
		}

		typename std::list<TVertex<V,H> >::iterator butterflySplitEdge (size_t he_id)
		{
			typename std::list<TVertex<V,H> >::iterator src_vertex, dst_vertex, far_vertex1, far_vertex2,
					 wing_vertex1, wing_vertex2, wing_vertex3, wing_vertex4;
			typename std::list<THalfedge<V,H> >::iterator he = std::next(halfedges.begin(),he_id);
			typename std::list<THalfedge<V,H> >::iterator he_op = he->opposite;

			src_vertex = he->prev->sink;
			dst_vertex = he->sink;

			far_vertex1 = he->next->sink;
			far_vertex2 = he_op->next->sink;

			wing_vertex1 = he->nextOverLine()->opposite->nextOverLine()->sink;
			wing_vertex2 = he->prevOverLine()->opposite->nextOverLine()->sink;
			wing_vertex3 = he_op->nextOverLine()->opposite->nextOverLine()->sink;
			wing_vertex4 = he_op->prevOverLine()->opposite->nextOverLine()->sink;

			Vector3f new_vertex_position = (1.f/2.f) * (src_vertex->position + dst_vertex->position);
			new_vertex_position = new_vertex_position + (1.f/8.f) * (far_vertex1->position + far_vertex2->position);
			new_vertex_position = new_vertex_position - (1.f/16.f) * (wing_vertex1->position + wing_vertex2->position +
							wing_vertex3->position + wing_vertex4->position);

			addVertex (Vertex (new_vertex_position));

			typename std::list<TVertex<V,H> >::iterator created_vertex = std::prev(vertices.end(),1);
			created_vertex->outHalfedge = he_op;
			he->sink = created_vertex;
			src_vertex->outHalfedge = he;

			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_go = std::prev(halfedges.end(),1);
			addHalfedge (Halfedge());
			typename std::list<THalfedge<V,H> >::iterator new_he_op = std::prev(halfedges.end(),1);

			new_he_go->opposite = new_he_op;
			new_he_op->opposite = new_he_go;

			new_he_go->sink = dst_vertex;
			new_he_op->sink = created_vertex;

			new_he_go->next = he->next;
			new_he_go->next->prev = new_he_go;
			new_he_op->next = he_op;

			new_he_go->prev = he;
			new_he_op->prev = he_op->prev;
			new_he_op->prev->next = new_he_op;

			new_he_go->face = he->face;
			new_he_op->face = he_op->face;

			he->next = new_he_go;
			he_op->prev = new_he_op;
			dst_vertex->outHalfedge = new_he_op;

			return created_vertex;
		}

		void updateOpposite (typename std::list<THalfedge<V,H> >::iterator he)
		{
			int founds = 0;
			for (auto it=halfedges.begin(); it!=halfedges.end(); it++)
			{
				if (he->sink == it->prev->sink && it->sink == he->prev->sink)
				{
					he->opposite = it;
					it->opposite = he;
					founds++;
				}
			}
		}

		void destroyFace (typename std::list<TFace<V,H> >::iterator f_id)
		{
			for (auto hit = halfedges.begin(); hit != halfedges.end(); hit++)
			{
				if (hit->face == f_id)
				{
					hit--;
					halfedges.erase(std::next(hit,1));
				}
			}
			faces.erase (f_id);
		}

		std::vector<typename std::list<TVertex<V,H> >::iterator> getOneRing(
				typename std::list<TVertex<V,H> >::iterator v
		)
		{
			std::vector<typename std::list<TVertex<V,H> >::iterator > oneRing;
			auto it = v->outHalfedge;
			do{
				oneRing.push_back (it->sink);
				it = it->prev->opposite;
			} while (it != v->outHalfedge);

			return oneRing;
		}

		std::list<TVertex<V,H> > vertices;
		std::list<THalfedge<V,H> > halfedges;
		std::list<TFace<V,H> > faces;

		typedef TVertex<V,H> Vertex;
		typedef THalfedge<V,H> Halfedge;
		typedef TFace<V,H> Face;
	
};

typedef Mesh<float,float> StandardMesh;

#endif
