#ifndef MESHLOADING_HPP_
#define MESHLOADING_HPP_

#include "linalgebra.hpp"
#include "mesh.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

inline std::vector<std::string> split( 
		const std::string& str, 
		const char delimiter
){
	std::vector<std::string> internal;
	std::stringstream ss(str);
	std::string tok;

	while(getline(ss, tok, delimiter)) {
		internal.push_back(tok);
	}
	return internal;
}

enum MeshFileType
{
	OBJ, OFF
};

template<MeshFileType FileType>
struct MeshIO
{
	
	static int loadMesh (
			std::string, 
			std::vector<Vector3f>&, 
			std::vector<int>&
	)
	{
		std::cout << "Not implemented.\n";
		return -1;
	}
	
	static int loadMesh (std::string)
	{
		std::cout << "Not implemented.\n";
		return -1;
	}
};

template<>
struct MeshIO<MeshFileType::OBJ>
{
	static int loadMesh(
			std::string path, 
			std::vector<Vector3f>& vertices, 
			std::vector<int>& indices
	)
	{
		std::ifstream fs;
		fs.open (path, std::ifstream::in);
		if(!fs)
		{
			std::cout << "Mesh file \"" << path << "\" could not be loaded." << std::endl;
			return -1;
		}
		std::vector<Vector3f> single_normals;
		std::vector<int> normal_indices;
		std::vector<Vector2f> single_uvs;
		std::vector<int> uvs_indices;

		while( !fs.eof() )
		{
			std::string type;
			fs >> type;
			if (type[0] == '#')
			{
				std::getline(fs,type);;
			}
			if (type == "v")
			{
				Vector3f new_vertex;
				fs >> new_vertex[0];
				fs >> new_vertex[1];
				fs >> new_vertex[2];
				vertices.push_back (new_vertex);
			}
			if (type == "vn")
			{
				Vector3f new_normal;
				fs >> new_normal[0];
				fs >> new_normal[1];
				fs >> new_normal[2];
				single_normals.push_back (new_normal);
			}
			if (type == "vt")
			{
				Vector2f new_uv;
				fs >> new_uv[0];
				fs >> new_uv[1];
				single_uvs.push_back (new_uv);
			}
			if (type == "f")
			{
				std::string vertex_desc;
				for (int i=0; i<3; ++i)
				{
					fs >> vertex_desc;
					std::vector<std::string> vert_split;
					vert_split = split (vertex_desc,'/');
					indices.push_back(std::stoi(vert_split[0])-1);
				}
			}
		}

		return 0;
	}

	static int loadMesh (std::string path, StandardMesh& mesh)
	{
		std::vector<Vector3f> raw_vertices;
		std::vector<int> indices;
		if (loadMesh (path, raw_vertices, indices) < 0)
			return -1;

		mesh.generateMesh (raw_vertices, indices);

		return 0;
	}

	static int writeMesh (std::string path, StandardMesh& mesh)
	{
		std::ofstream fs;
		fs.open (path, std::ofstream::out);

		for (auto i=mesh.vertices.begin(); i!=mesh.vertices.end(); i++)
		{
			fs << "v " << i->position << std::endl;
		}

		for (auto i=mesh.faces.begin(); i!=mesh.faces.end(); i++)
		{
			fs << "f ";
			auto he = i->halfedge;
			auto it = he;
			do {
//				std::cout << it->face->id << " " << he->id << " " << i->id << std::endl;
				fs << it->sink->id+1 << " ";
				it = it->next;
			} while (it != he);
			fs << std::endl;
		}

		return 0;
	}
};

#endif
