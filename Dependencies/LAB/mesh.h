#ifndef _MESH_
#define _MESH_

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>

class Mesh {
public:
	void init();
	// should properly set up the geometry buffer
	void render(); // should be called in the main rendering loop
	void addPosCor(float pos);
	void addNorCor(float nor);
	void addTextCor(float col);
	void addInd(int ind);  

	static std::shared_ptr<Mesh> genSphere(const size_t resolution=16); // should generate a unit sphere

// ...
private:
	std::vector<float> m_vertexPositions;
	std::vector<float> m_vertexNormals;
	std::vector<float> m_vertexTexCoords;
	std::vector<unsigned int> m_triangleIndices;

	GLuint m_vao = 0;
	GLuint m_posVbo = 0;
	GLuint m_normalVbo = 0;
	GLuint m_texCoordVbo = 0;

	GLuint m_ibo = 0;
//

};

void Mesh::addPosCor(float pos){this->m_vertexPositions.push_back(pos);};
void Mesh::addNorCor(float nor){this->m_vertexNormals.push_back(nor);};
void Mesh::addTextCor(float textCord){this->m_vertexTexCoords.push_back(textCord);};
void Mesh::addInd(int ind){this->m_triangleIndices.push_back(ind);};



void Mesh::init() {
  // Create a single handle that joins together attributes (vertex positions,
  // normals) and connectivity (triangles indices)
  glCreateVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
  size_t vertexBufferSize = sizeof(float)*m_vertexPositions.size(); // Gather the size of the buffer from the CPU-side vector


  // Generate a GPU buffer to store the positions of the vertices
  glCreateBuffers(1, &m_posVbo);
  glNamedBufferStorage(m_posVbo, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data storage on the GPU
  glNamedBufferSubData(m_posVbo, 0, vertexBufferSize, m_vertexPositions.data()); // Fill the data storage from a CPU array
  glBindBuffer(GL_ARRAY_BUFFER, m_posVbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);
  
 
  // Generate a GPU buffer to store the normals of the vertices
  glCreateBuffers(1, &m_normalVbo);
  glNamedBufferStorage(m_normalVbo, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data storage on the GPU
  glNamedBufferSubData(m_normalVbo, 0, vertexBufferSize, m_vertexNormals.data()); // Fill the data storage from a CPU array
  glBindBuffer(GL_ARRAY_BUFFER, m_normalVbo);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);
  

  vertexBufferSize = sizeof(float)*m_vertexTexCoords.size(); // Gather the size of the buffer from the CPU-side vector

  // Generate a GPU buffer to store the colors of the vertices
  glCreateBuffers(1, &m_texCoordVbo);
  glNamedBufferStorage(m_texCoordVbo, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data storage on the GPU
  glNamedBufferSubData(m_texCoordVbo, 0, vertexBufferSize, m_vertexTexCoords.data()); // Fill the data storage from a CPU array
  glBindBuffer(GL_ARRAY_BUFFER, m_texCoordVbo);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);

  glBindVertexArray(0); // deactivate the VAO for now, will be activated at rendering time

  // Same for the index buffer that stores the list of indices of the
  // triangles forming the mesh
  size_t indexBufferSize = sizeof(unsigned int)*m_triangleIndices.size();
  glCreateBuffers(1, &m_ibo);
  glNamedBufferStorage(m_ibo, indexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
  glNamedBufferSubData(m_ibo, 0, indexBufferSize, m_triangleIndices.data());
}

void Mesh::render(){
  glBindVertexArray(m_vao);     // bind the VAO storing geometry data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo); // bind the IBO storing geometry data
  glDrawElements(GL_TRIANGLES, m_triangleIndices.size(), GL_UNSIGNED_INT, 0); // Call for rendering: stream the current GPU geometry through the current GPU program
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  
}


int getIndex(int p, int q, const size_t resolution)
{
	return (resolution+1)*p + q;
};


std::shared_ptr<Mesh> Mesh::genSphere(const size_t resolution)
{
	std::shared_ptr<Mesh> newMesh(new Mesh());

	int i,j;
	for (i=0;i<resolution+1;i=i+1){
		for (j=0;j<resolution+1;j=j+1){
	      	float x = (sin(i*(M_PI/resolution))*cos(j*(2*M_PI/resolution)));
	      	float y = (sin(i*(M_PI/resolution))*sin(j*(2*M_PI/resolution)));
	      	float z = cos(i*(M_PI/resolution));

	      	newMesh->addPosCor(x);
	      	newMesh->addPosCor(y);
		    newMesh->addPosCor(z);
	        

	        newMesh->addNorCor(x);
	        newMesh->addNorCor(y);
	        newMesh->addNorCor(z);


	        newMesh->addTextCor(static_cast<float>(j)/static_cast<float>(resolution));
	      	newMesh->addTextCor(static_cast<float>(i)/static_cast<float>(resolution));  // normally (1 - ...) but weirdly I get the correct texture mapping without it 
    	};
	}; 
 
for (i=0;i<resolution;i=i+1){
    for (j=0;j<resolution;j=j+1){
			newMesh->addInd(getIndex(i,j,resolution));
			newMesh->addInd(getIndex(i+1,j,resolution));
			newMesh->addInd(getIndex(i+1,j+1,resolution));

			newMesh->addInd(getIndex(i,j,resolution));
			newMesh->addInd(getIndex(i+1,j+1,resolution));
			newMesh->addInd(getIndex(i,j+1,resolution));
  	};
};
	return newMesh;

}
#endif