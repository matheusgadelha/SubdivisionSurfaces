#include <iostream>
#include <vector>
#include <cstring>

#include "linalgebra.hpp"
#include "meshio.hpp"
#include "mesh.hpp"

int main(int argc, char** argv)
{
	if(argc != 5)
	{
		std::cout << "Invalid Arguments. Usage: ./subdivide <meshpath> <outputpath> <butterfly | loop> <iterations>" << std::endl;
		std::cout << "Example: ./subdivide bunny_1k.obj output.obj butterfly 2" << std::endl;
		return -1;
	}

	Mesh<float,float> mesh;
	
	MeshIO<MeshFileType::OBJ>::loadMesh (argv[1], mesh);
	for (int i=0; i<std::stoi(argv[4]); ++i)
	{
		if(strcmp(argv[3],"butterfly") == 0)
			mesh.butterflySubdivision();
		if(strcmp(argv[3],"loop") == 0)
			mesh.loopSubdivision();
	}
	MeshIO<MeshFileType::OBJ>::writeMesh (argv[2], mesh);

	return 0;
}
