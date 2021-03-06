/////////////////////////////////////
// Object.cpp
// The mesh of object.
/////////////////////////////////////
#include "Mesh.h"
#include "D3DClass.h"
#include <fstream>
#include <string>

Mesh::Mesh()
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}

Mesh::Mesh(const Mesh &)
{
}

Mesh::~Mesh()
{
}

bool Mesh::InitializeBuffers()
{
	HRESULT hr;

	// Setup the description of the vertex buffer.
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(VertexDataType)*m_data.size();
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = &(m_data[0]);
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create vertex buffer
	hr = D3DClass::GetInstance().GetDevice()->CreateBuffer(&vbDesc, &vertexData, &m_vertexBuffer);
	if(FAILED(hr))
		return false;

	// Setup the description of index buffer.
	D3D11_BUFFER_DESC indexDesc;
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.ByteWidth = sizeof(unsigned int)*m_indices.size();
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.CPUAccessFlags = 0;
	indexDesc.MiscFlags = 0;
	indexDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = &(m_indices[0]);
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create index buffer
	hr = D3DClass::GetInstance().GetDevice()->CreateBuffer(&indexDesc, &indexData, &m_indexBuffer);
	if(FAILED(hr))
		return false;

	return true;
}

void Mesh::ShutdownBuffer()
{
	// Release the index buffer
	
	if(m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}
	

	// Release the vertex buffer
	if(m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	return;
}

void Mesh::RenderBuffers()
{
	// Set vertex buffer stride and offset
	UINT strides = sizeof(VertexDataType);
	UINT offsets = 0;

	// Set the vertex buffer to active 
	D3DClass::GetInstance().GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &strides, &offsets);
	// Set the index buffer to active
	D3DClass::GetInstance().GetDeviceContext()->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	// Set the type of primitive tha should be rendered from this vertex buffer
	D3DClass::GetInstance().GetDeviceContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Model::Model()
{
	m_texture = nullptr;
}

Model::Model(Model & model)
{
}

Model::~Model()
{
}

bool Model::Initialize(TCHAR* meshPath, TCHAR* texPath)
{
	bool result;

	// Load mesh
	result = LoadFromObj(meshPath);
	if(!result)
		return false;

	result = LoadMtl();
	if (!result)
		return false;

	// Load texture if texture is setted
	if(texPath)
	{
		result = LoadTexture(texPath);
		if(!result)
			return false;
	}

	// Initialize the vertex and index buffers.
	for(auto mesh : m_model)
	{
		result = mesh->InitializeBuffers();
		if(!result)
			return false;
	}
	return true;

}

void Model::Shutdown()
{
	// Release texture
	ReleaseTexture();

	// Release meshes
	for(auto mesh : m_model)
	{
		mesh->ShutdownBuffer();
		delete mesh;
		mesh = nullptr;
	}

	return;
}

void Model::Render()
{
	for(auto mesh: m_model)
		mesh->RenderBuffers();

}

int Model::GetIndexCount()
{
	int indexCnt = 0;
	for(auto mesh : m_model)
		indexCnt += mesh->m_indices.size();
	return indexCnt;
}

ID3D11ShaderResourceView * Model::GetTexture()
{
	if(m_texture)
		return m_texture->GetTexture();
	else
		return nullptr;
}

bool Model::LoadFromObj(TCHAR* filePath)
{
	ifstream fin;
	string input, input2;

	float maxX = 0.0f, minX = 0.0f, maxY = 0.0f, minY = 0.0f, maxZ = 0.0f, minZ = 0.0f;
	int cnt = 0;
	XMFLOAT3 v, vn;
	XMFLOAT2 vt;
	XMINT3 vertex;
	Face f;
	GroupInfo gInfo;


	// Open file
	fin.open(filePath, ios::in);
	if(fin.fail())
	{
		MessageBox(nullptr, _T("Can't open mesh file"), _T("Error"), MB_OK);
		return false;
	}

	// Read data
	while(!fin.eof() && fin.peek() != EOF)
	{

		fin >> input;

		// Read mtl file path
		if(input.compare("mtllib") == 0)
			fin >> m_objData.mtllib;

		// Read geometric vertex
		if(input.compare("v") == 0)
		{
			fin >> v.x >> v.y >> v.z;
			// Invert the Z vertex to change to left hand system
			v.z = v.z*-1.0f;
			// Store the maxin and minin of the mesh
			if(cnt == 0)
			{
				maxX = minX = v.x;
				maxY = minY = v.y;
				maxZ = minZ = v.z;
				cnt++;
			}
			else
			{
				if(v.x > maxX)
					maxX = v.x;
				else if(v.x < minX)
					minX = v.x;
				if(v.y > maxY)
					maxY = v.y;
				else if(v.y < minY)
					minY = v.y;
				if(v.z > maxZ)
					maxZ = v.z;
				else if(v.z < minZ)
					minZ = v.z;
			}
			m_objData.v.push_back(v);

		}

		// Read vertex texture Coordinate Indices
		if(input.compare("vt") == 0)
		{
			fin >> vt.x >> vt.y;
			// Invert the V texture coordinates to left hand system.
			vt.y = 1.0f - vt.y;
			m_objData.vt.push_back(vt);
		}

		// Read vertex normal
		if(input.compare("vn") == 0)
		{
			fin >> vn.x >> vn.y >> vn.z;
			// Invert the Z normal to left hand system.
			vn.z = vn.z*-1.0f;
			m_objData.vn.push_back(vn);
		}

		// Read group information
		if(input.compare("g") == 0)
		{
			string g;
			fin >> g >> input;

			if(input.compare("usemtl") != 0)
			{
				fin.unget();
				continue;
			}

			gInfo.name = g;
			// Read material name
			fin >> gInfo.usemtl;

			//  Read face
			fin.get();
			input = (char)fin.peek();
			while(input.compare("f") == 0)
			{
				fin >> input;
				while(fin.peek() != '\n')
				{
					fin >> input2;
					DivideIndexString(vertex, input2);
					f.vertices.insert(f.vertices.begin(), vertex);
				}
				gInfo.faces.push_back(f);
				f.vertices.clear();
				fin.get();
				input = (char)fin.peek();
			}

			m_objData.groups.push_back(gInfo);
			gInfo.name = "";
			gInfo.usemtl = "";
			gInfo.faces.clear();
		}
	}
	// Close file
	fin.close();

	// Calculate the size of the model
	m_width = maxX - minX;
	m_height = maxY - minY;
	m_depth = maxZ - minZ;


	// Write to mesh
	if(!WriteMesh())
		return false;
	return true;
}

bool Model::LoadMtl()
{
	ifstream fin;
	string input, input2;
	MaterialType material;
	string newmtl;

	// Open file
	fin.open(m_mtlPath, ios::in);
	if (fin.fail())
	{
		MessageBox(nullptr, _T("Can't open material file"), _T("Error"), MB_OK);
		return false;
	}
	
	// Read data
	while (!fin.eof() && fin.peek() != EOF)
	{
		
		fin >> input;

		// Read comment
		if (input.compare("#") == 0)
			fin.ignore(256, '\n');

		// Read material name
		if (input.compare("newmtl") == 0)
			fin >> newmtl;
		else
			continue;

		// Read material data
		while(input.compare("\n")!=0 && (!fin.eof() && fin.peek() != EOF))
		{
			fin >> input;
			// Read ambient color
			if (input.compare("Ka") == 0)
			{
				
				fin >> material.ambient.x >> material.ambient.y >> material.ambient.z >> material.ambient.w;
			}

			// Read diffuse color
			if (input.compare("Kd") == 0)
			{
				fin >> material.diffuse.x >> material.diffuse.y >> material.diffuse.z >> material.diffuse.w;
			}

			// Read specular color
			if (input.compare("Ks") == 0)
			{
				fin >> material.specular.x >> material.specular.y >> material.specular.z;
			}

			// Read specular power
			if (input.compare("Ns") == 0)
			{
				fin >> material.specularPower;
			}

			// Read comment
			if (input.compare("#") == 0)
			{
				fin.ignore(256, '\n');
				continue;
			}
			fin.get();
			input = fin.peek();
			//fin >> input;
		}
		m_materials.insert(pair<string, MaterialType>(newmtl, material));
	}
	// Close file
	fin.close();

	return true;
}

void Model::DivideIndexString(XMINT3 & index, string input)
{
	char num[3][256];
	int i = 0, j = 0;
	for(auto ch : input)
	{
		if(ch != '/')
		{
			num[j][i] = ch;
			++i;
		}
		else
		{
			num[j][i] = '\0';
			++j;
			i = 0;
		}
	}
	num[j][i] = '\0';
	index.x = atoi(num[0]);
	index.y = atoi(num[1]);
	index.z = atoi(num[2]);
}

void Model::ReleaseTexture()
{
	if(m_texture)
	{
		m_texture->ShutDown();
		delete m_texture;
		m_texture = nullptr;
	}
}

bool Model::WriteMesh()
{
	// Write mtl file path
	m_mtlPath = "./data/model/" + m_objData.mtllib;

	// Write vertex information
	for(auto group : m_objData.groups)
	{
		int vIndex = 0, vtIndex = 0, vnIndex = 0;
		Mesh* mesh = new Mesh;
		// Write group name and material name
		mesh->m_name = group.name;
		mesh->m_usemtl = group.usemtl;
		unsigned int idxCnt = 0;
		// Write face information
		for(auto f : group.faces)
		{
			vector<int> iList({ 0,1,2 });
			// If the face has 4 vertices
			if(f.vertices.size() == 4)
			{
				vector<int> sub({ 0,2,3 });
				iList.insert(iList.end(), sub.begin(), sub.end());
			}
			
			for(auto i : iList)
			{

				vIndex = f.vertices[i].x - 1;
				vtIndex = f.vertices[i].y - 1;
				vnIndex = f.vertices[i].z - 1;

				VertexDataType vd;
				// Write vertex
				vd.pos = m_objData.v[vIndex];
				if(vtIndex < 0) // If no texture coordinate data
					vd.tex = { 0.0,0.0 };
				else
					vd.tex = m_objData.vt[vtIndex];
				vd.nor = m_objData.vn[vnIndex];
				mesh->m_data.push_back(vd);
				// Write index
				
				mesh->m_indices.push_back(idxCnt);
				++idxCnt;
			}
		}

		m_model.push_back(mesh);
	}
	if(m_model.empty())
		return false;

	// Clear objData
	m_objData.v.clear();
	m_objData.vn.clear();
	m_objData.vt.clear();
	m_objData.mtllib = "";
	for(auto group : m_objData.groups)
	{
		for(auto face : group.faces)
			face.vertices.clear();
		group.faces.clear();
	}
	m_objData.groups.clear();

	return true;
}

bool Model::LoadTexture(TCHAR * texPath)
{
	bool result;

	// Create the texture object
	m_texture = new Texture;
	if(!m_texture)
		return false;

	result = m_texture->Initialize(texPath);
	if(!result)
		return false;
	return true;
}
