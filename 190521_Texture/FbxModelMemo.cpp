#include "FbxModelMemo.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	FbxMobelApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}


FbxMobelApp::FbxMobelApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mVB(0), mIB(0), 
	//mFX(0), mTech(0),
	//mfxWorldViewProj(0), 
	mInputLayout(0), mWireframeRS(0),
	mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(15.0f), mDiffuseMapSRV(0)
{
	mMainWndCaption = L"Shapes Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMStoreFloat4x4(&mBoxWorld, I);
	XMStoreFloat4x4(&mTexTransform, I);

	XMMATRIX boxScale = XMMatrixScaling(0.01f, 0.01f, 0.01f);
	//XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale);

	//XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	//XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	//XMStoreFloat4x4(&mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

	mDirLights[0].Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mDirLights[0].Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLights[0].Specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
	mDirLights[0].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	mDirLights[1].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Diffuse = XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
	mDirLights[1].Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
	mDirLights[1].Direction = XMFLOAT3(-0.707f, 0.0f, 0.707f);

	mBoxMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
}

FbxMobelApp::~FbxMobelApp()
{
	fbxScene->Clear();
	fbxScene->Destroy();

	fbxMng->Destroy();

	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
	ReleaseCOM(mDiffuseMapSRV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool FbxMobelApp::Init()
{
	if (!D3DApp::Init())
		return false;

	Effects::InitAll(md3dDevice);
	InputLayouts::InitAll(md3dDevice);

	ID3D11Resource* texResource = nullptr;
	HR(CreateDDSTextureFromFile(md3dDevice,
		L"FbxModel/ToonDragon.dds", &texResource, &mDiffuseMapSRV));
	ReleaseCOM(texResource)	// view saves reference

	BuildGeometryBuffers();

	return true;
}

void FbxMobelApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void FbxMobelApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void FbxMobelApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view * proj;

	//XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(mDirLights);
	Effects::BasicFX->SetEyePosW(mEyePosW);

	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light2TexTech;

	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		md3dImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

		// Draw the box.
		XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world * view*proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&mTexTransform));
		Effects::BasicFX->SetMaterial(mBoxMat);
		Effects::BasicFX->SetDiffuseMap(mDiffuseMapSRV);

		// 36 indices for the box.
		activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mFbxModelIndexCount, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void FbxMobelApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void FbxMobelApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void FbxMobelApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.01f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 200.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void FbxMobelApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData fbxModel;

	fbxMng = FbxManager::Create();
	fbxScene = FbxScene::Create(fbxMng, "TestScene");
	fbxImporter = FbxImporter::Create(fbxMng, "TestImporter");

	if (fbxScene->GetGlobalSettings().GetAxisSystem() != FbxAxisSystem::Max)
	{
		fbxScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::Max);
	}

	bool isSuccess = fbxImporter->Initialize("FbxModel/ToonDragon.fbx", -1, fbxMng->GetIOSettings());
	
	if (isSuccess == false)
	{
		assert(0);
	}

	isSuccess = fbxImporter->Import(fbxScene);
	if (isSuccess == false)
	{
		assert(0);
	}

	FbxNode* rootNode = fbxScene->GetRootNode();

	if (rootNode)
	{
		for (int i = 0; i < rootNode->GetChildCount(); i++)
		{
			LoadNode(rootNode->GetChild(i), fbxModel, fbxModel.Vertices.size());
		}
	}

	fbxImporter->Destroy();

	UINT k = 0;
	std::vector<Vertex::Basic32> vertices(fbxModel.Vertices.size());
	for (size_t i = 0; i < fbxModel.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = fbxModel.Vertices[i].Position;
		vertices[k].Normal = fbxModel.Vertices[i].Normal;
		vertices[k].Tex = fbxModel.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * fbxModel.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));

	mFbxModelIndexCount = fbxModel.Indices.size();
	std::vector<UINT> indices(mFbxModelIndexCount);
	for (size_t i = 0; i < mFbxModelIndexCount; ++i)
	{
		indices[i] = fbxModel.Indices[i];
	}


	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mFbxModelIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}

void FbxMobelApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();

	//HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 0, md3dDevice, &mFX));

	//mTech = mFX->GetTechniqueByName("ColorTech");
	//mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void FbxMobelApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	D3DX11_PASS_DESC passDesc;
	//mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize, &mInputLayout));
}

void FbxMobelApp::LoadNode(FbxNode* node, GeometryGenerator::MeshData& meshData, UINT vertexOffset)
{
	FbxNodeAttribute::EType lAttributeType;

	lAttributeType = node->GetNodeAttribute()->GetAttributeType();

	if (lAttributeType == FbxNodeAttribute::eMesh)
	{
		FbxMesh* pMesh = (FbxMesh*)node->GetNodeAttribute();

		int lControlPointsCount = pMesh->GetControlPointsCount();
		FbxVector4* lControlPoints = pMesh->GetControlPoints();

		for (int i = 0; i < lControlPointsCount; i++)
		{
			GeometryGenerator::Vertex vertex;
			vertex.Position.x = (float)lControlPoints[i].mData[0];
			vertex.Position.y = (float)lControlPoints[i].mData[1];
			vertex.Position.z = (float)lControlPoints[i].mData[2];


			meshData.Vertices.push_back(vertex);
		}

		int lPolygonCount = pMesh->GetPolygonCount();
		int vertexCount = 0;
		for (int i = 0; i < lPolygonCount; i++)
		{
			int polygonSize = pMesh->GetPolygonSize(i);
			for (int j = 0; j < polygonSize; ++j)
			{
				int lControlPointIndex = pMesh->GetPolygonVertex(i, j);

				// 폴리곤의 사이즈가 4일경우
				// 버텍스가 4개의 위치가 나온다 0, 1, 2, 3
				// 그럴때 인덱스 버퍼에 쓰여질 값은
				// 0, 1, 2 - 3, 0, 2

				/*
				   3              0
					-------------
					|			|
					|			|
					|			|
					|			|
					|			|
					-------------
				   2              1


				*/
				// 폴리곤의 사이즈가 3일경우
				// 버텍스가 3개
				// 인덱스 버퍼에 쓰여질값은 0, 1, 2

				// 폴리곤의 사이즈가 4보다 큰경우가 있을까?
				meshData.Indices.push_back(lControlPointIndex + vertexOffset);
				meshData.Vertices[lControlPointIndex + vertexOffset].TexC = ReadUV(pMesh, lControlPointIndex, vertexCount);
				meshData.Vertices[lControlPointIndex + vertexOffset].Normal = ReadNormal(pMesh, lControlPointIndex, vertexCount);
				meshData.Vertices[lControlPointIndex + vertexOffset].TangentU = ReadTangent(pMesh, lControlPointIndex, vertexCount);

				++vertexCount;

				if (j >= 3)
				{
					meshData.Indices.push_back(pMesh->GetPolygonVertex(i, 0));
					meshData.Indices.push_back(pMesh->GetPolygonVertex(i, j - 1));
				}
			}
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		LoadNode(node->GetChild(i), meshData, meshData.Vertices.size());
	}
}
XMFLOAT3 FbxMobelApp::ReadNormal(FbxMesh * mesh, int controlPointIndex, int vertexCounter)
{
	if (mesh->GetElementNormalCount() < 1)
		return XMFLOAT3();

	FbxGeometryElementNormal* vertexNormal = mesh->GetElementNormal(0);

	XMFLOAT3 vec3;

	switch (vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[0]);
			vec3.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[1]);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexNormal->GetIndexArray().GetAt(controlPointIndex);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
		//	vec3 = XMFLOAT3(0, 0, 0);
		//	break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[0]);
			vec3.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[1]);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexNormal->GetIndexArray().GetAt(vertexCounter);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
			//vec3 = XMFLOAT3(0, 0, 0);
			//break;
		}
		break;
	}

	return vec3;
}
XMFLOAT3 FbxMobelApp::ReadBinormal(FbxMesh * mesh, int controlPointIndex, int vertexCounter)
{
	if (mesh->GetElementBinormalCount() < 1)
		return XMFLOAT3();

	FbxGeometryElementBinormal* vertexBinormal = mesh->GetElementBinormal(0);

	XMFLOAT3 vec3;

	switch (vertexBinormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexBinormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(controlPointIndex).mData[0]);
			vec3.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(controlPointIndex).mData[1]);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(controlPointIndex).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexBinormal->GetIndexArray().GetAt(controlPointIndex);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
			//vec3 = XMFLOAT3(0, 0, 0);
			//break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexBinormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(vertexCounter).mData[0]);
			vec3.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(vertexCounter).mData[1]);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(vertexCounter).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexBinormal->GetIndexArray().GetAt(vertexCounter);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexBinormal->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
			//vec3 = XMFLOAT3(0, 0, 0);
			//break;
		}
		break;
	}

	return vec3;
}
XMFLOAT3 FbxMobelApp::ReadTangent(FbxMesh * mesh, int controlPointIndex, int vertexCounter)
{
	if (mesh->GetElementTangentCount() < 1)
		return XMFLOAT3();

	FbxGeometryElementTangent* vertexTangent = mesh->GetElementTangent(0);

	XMFLOAT3 vec3;

	switch (vertexTangent->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(controlPointIndex).mData[0]);
			vec3.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(controlPointIndex).mData[1]);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(controlPointIndex).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexTangent->GetIndexArray().GetAt(controlPointIndex);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
			//vec3 = XMFLOAT3(0, 0, 0);
			//break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(vertexCounter).mData[0]);
			vec3.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(vertexCounter).mData[1]);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(vertexCounter).mData[2]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexTangent->GetIndexArray().GetAt(vertexCounter);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			vec3.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			vec3.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
			break;
		//default:
			//vec3 = XMFLOAT3(0, 0, 0);
			//break;
		}
		break;
	}

	return vec3;
}
XMFLOAT2 FbxMobelApp::ReadUV(FbxMesh * mesh, int controlPointIndex, int vertexCounter)
{
	if (mesh->GetElementUVCount() < 1)
		return XMFLOAT2();

	FbxGeometryElementUV* vertexUV = mesh->GetElementUV(0);

	XMFLOAT2 vec2;

	switch (vertexUV->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec2.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(controlPointIndex).mData[0]);
			vec2.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(controlPointIndex).mData[1]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexUV->GetIndexArray().GetAt(controlPointIndex);
			vec2.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
			vec2.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
			break;
		//default:
			//vec2 = XMFLOAT2(0, 0);
			//break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			vec2.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[0]);
			vec2.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[1]);
			break;
		case FbxGeometryElement::eIndexToDirect:
			int index = vertexUV->GetIndexArray().GetAt(vertexCounter);
			vec2.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
			vec2.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
			break;
		//default:
			//vec2 = XMFLOAT2(0, 0);
			//break;
		}
		break;
	}

	return vec2;
}