#include "precomp.h"
#include <tiny_gltf.h>
#include <glm/gtx/string_cast.hpp>

void Mesh::LoadMesh()
{
	cout << "Loading mesh " << endl;

	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	string err;
	string warn;
	char prefix[64] = "rooms/";
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, strcat(strcat(prefix, modelFile), ".glb")); // for binary glTF(.glb)
	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return;
	}

	tinygltf::Primitive& primitive = model.meshes[0].primitives[0];
	const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes["POSITION"]];
	const tinygltf::Accessor& texcoordAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
	const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
	const tinygltf::BufferView& texcoordBufferView = model.bufferViews[texcoordAccessor.bufferView];
	const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
	const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
	const tinygltf::Buffer& texcoordBuffer = model.buffers[texcoordBufferView.buffer];
	const tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
	const float* positions = reinterpret_cast<const float*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
	const float* texcoords = reinterpret_cast<const float*>(&texcoordBuffer.data[texcoordBufferView.byteOffset + texcoordAccessor.byteOffset]);

	// Depending on the mesh size, the indices will either be shorts or ints
	const unsigned short* temps;
	const unsigned int* tempi;
	bool shortIndices = indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	if (shortIndices)
	{
		temps = reinterpret_cast<const unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}
	else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		tempi = reinterpret_cast<const unsigned int*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}
	
	triangles = new Tri[indicesAccessor.count / 3];
	vertices = new float[indicesAccessor.count * 3];
	uvcoords = new float[indicesAccessor.count * 2];
	for (size_t i = 0; i < indicesAccessor.count / 3; ++i) {
		Tri* triangle = &triangles[i];
		int v1ID = i * 3 + 0, v2ID = i * 3 + 1, v3ID = i * 3 + 2;
		int v1 = shortIndices ? temps[v1ID] : tempi[v1ID], v2 = shortIndices ? temps[v2ID] : tempi[v2ID], v3 = shortIndices ? temps[v3ID] : tempi[v3ID];
		triangle->vertex0 = make_float3_strict(
			positions[v1 * 3 + 0],
			positions[v1 * 3 + 1],
			positions[v1 * 3 + 2]);
		triangle->vertex1 = make_float3_strict(
			positions[v2 * 3 + 0],
			positions[v2 * 3 + 1],
			positions[v2 * 3 + 2]);
		triangle->vertex2 = make_float3_strict(
			positions[v3 * 3 + 0],
			positions[v3 * 3 + 1],
			positions[v3 * 3 + 2]);
		vertices[i * 9 + 0] = triangle->vertex0.x;
		vertices[i * 9 + 1] = triangle->vertex0.y;
		vertices[i * 9 + 2] = triangle->vertex0.z;
		vertices[i * 9 + 3] = triangle->vertex1.x;
		vertices[i * 9 + 4] = triangle->vertex1.y;
		vertices[i * 9 + 5] = triangle->vertex1.z;
		vertices[i * 9 + 6] = triangle->vertex2.x;
		vertices[i * 9 + 7] = triangle->vertex2.y;
		vertices[i * 9 + 8] = triangle->vertex2.z;
		uvcoords[i * 6 + 0] = texcoords[v1 * 2 + 0];
		uvcoords[i * 6 + 1] = texcoords[v1 * 2 + 1];
		uvcoords[i * 6 + 2] = texcoords[v2 * 2 + 0];
		uvcoords[i * 6 + 3] = texcoords[v2 * 2 + 1];
		uvcoords[i * 6 + 4] = texcoords[v3 * 2 + 0];
		uvcoords[i * 6 + 5] = texcoords[v3 * 2 + 1];
	}

	vertexCount = indicesAccessor.count * 3;
	triangleCount = indicesAccessor.count / 3;

	DetermineFloorHeight();

	cout << "Vertex count: " << vertexCount << " triangle count: " << triangleCount << endl;
	bvh = new BVH(this);
	cout << "BVH size: " << bvh->nodesUsed << endl;
	BindMesh(model);
}

void Mesh::DetermineFloorHeight()
{
	// Determine floor height
	int binCount = 48;
	float maxVal = 0.0f, minVal = 0.0f;
	int* heightHistogram = new int[binCount];
	for (int j = 0; j < binCount; ++j)
	{
		heightHistogram[j] = 0;
	}
	for (int i = 0; i < vertexCount / 3; ++i)
	{
		if (vertices[i * 3 + 1] < minVal)
			minVal = vertices[i * 3 + 1];
	}
	float range = maxVal - minVal;
	for (int i = 0; i < vertexCount / 3; ++i)
	{
		for (int j = 0; j < binCount; ++j)
		{
			if (j * range / binCount + minVal < vertices[i * 3 + 1] && vertices[i * 3 + 1] < (j + 1) * (range) / binCount + minVal)
			{
				heightHistogram[j]++;
			}
		}
	}
	int maxCount = 0, maxIndex = -1;
	for (int i = 0; i < binCount; ++i)
	{
		if (heightHistogram[i] > maxCount)
		{
			maxIndex = i;
			maxCount = heightHistogram[i];
		}
	}
	floorHeight = (maxIndex + 0.5f) * range / binCount + minVal;
}

void Mesh::BindMesh(tinygltf::Model& model)
{
	cout << "Binding the mesh " << endl;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &UVBuffer);
	glGenBuffers(1, &dosageBufferID);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	// Free up memory on host
	delete[] vertices;

	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, (vertexCount * 2 / 3) * sizeof(float), uvcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, dosageBufferID);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Load the mesh texture
	if (model.textures.size() > 0) {
		tinygltf::Texture& tex = model.textures[model.materials[0].pbrMetallicRoughness.baseColorTexture.index];

		if (tex.source > -1) {

			glGenTextures(1, &textureBuffer);

			tinygltf::Image& image = model.images[tex.source];

			glBindTexture(GL_TEXTURE_2D, textureBuffer);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			GLenum format = GL_RGBA;
			if (image.component == 1) {
				format = GL_RED;
			}
			else if (image.component == 2) {
				format = GL_RG;
			}
			else if (image.component == 3) {
				format = GL_RGB;
			}

			GLenum type = image.bits == 16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
				format, type, &image.image.at(0));

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}