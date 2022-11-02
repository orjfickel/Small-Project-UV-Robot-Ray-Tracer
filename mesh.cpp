#include "precomp.h"
#include <tiny_gltf.h>
#include <glm/gtx/string_cast.hpp>

void Mesh::LoadMesh(string modelFile)
{
	cout << "Loading mesh " << endl;
	// Load the mesh (TODO: allow specifying model in gui and load after pushing button)
	tinygltf::TinyGLTF loader;
	string err;
	string warn;
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, modelFile); // for binary glTF(.glb)
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
	const unsigned short* temps;
	const unsigned int* tempi;
	bool shortIndices = indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	if (shortIndices)
	{
		cout << "Loading unsigned short type indices " << endl;
		temps = reinterpret_cast<const unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}
	else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		cout << "Loading unsigned int type indices " << endl;
		tempi = reinterpret_cast<const unsigned int*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}

	triangles = new Tri[indicesAccessor.count / 3];
	vertices = new float[indicesAccessor.count * 3];
	uvcoords = new float[indicesAccessor.count * 2];
	for (size_t i = 0; i < indicesAccessor.count / 3; ++i) {
		//cout << " vertexposz: " << positions[(shortIndices ? temps[i] : tempi[i]) * 3 + 2] << endl;
		Tri* triangle = &triangles[i];
		int v1ID = i * 3 + 0, v2ID = i * 3 + 1, v3ID = i * 3 + 2;
		triangle->vertex0 = make_float3_strict(
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 0],
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 1],
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 2]);
		triangle->vertex1 = make_float3_strict(
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 0],
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 1],
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 2]);
		triangle->vertex2 = make_float3_strict(
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 0],
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 1],
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 2]);
		vertices[i * 9 + 0] = triangle->vertex0.x;
		vertices[i * 9 + 1] = triangle->vertex0.y;
		vertices[i * 9 + 2] = triangle->vertex0.z;
		vertices[i * 9 + 3] = triangle->vertex1.x;
		vertices[i * 9 + 4] = triangle->vertex1.y;
		vertices[i * 9 + 5] = triangle->vertex1.z;
		vertices[i * 9 + 6] = triangle->vertex2.x;
		vertices[i * 9 + 7] = triangle->vertex2.y;
		vertices[i * 9 + 8] = triangle->vertex2.z;
		uvcoords[i * 6 + 0] = texcoords[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 2 + 0];
		uvcoords[i * 6 + 1] = texcoords[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 2 + 1];
		uvcoords[i * 6 + 2] = texcoords[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 2 + 0];
		uvcoords[i * 6 + 3] = texcoords[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 2 + 1];
		uvcoords[i * 6 + 4] = texcoords[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 2 + 0];
		uvcoords[i * 6 + 5] = texcoords[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 2 + 1];
	}

	vertexCount = indicesAccessor.count * 3;
	triangleCount = indicesAccessor.count / 3;
	cout << "vert " << vertexCount << " tricount " << triangleCount << endl;
	bvh = new BVH(this);
	cout << "BVH " << bvh->nodesUsed << " tricount " << triangleCount << endl;
	/*BVHNode* node = bvh->bvhNode;
	for (int i = 0; i < 1000; ++i)
	{
		cout << "nodex " << node->aabbMin.x << " nodey " << node->aabbMin.y << " nodez " << node->aabbMin.z
			<< " maxx " << node->aabbMax.x << " maxy " << node->aabbMax.y << " maxz " << node->aabbMax.z << " tricount " << node->triCount << endl;

		if (node->triCount > 0)
			break;
		node = &bvh->bvhNode[node->leftFirst];
	}*/
}