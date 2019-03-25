//***************************************************************************************
// ShapesDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates drawing simple geometric primitives in wireframe mode.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//***************************************************************************************

#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include <vector>
#include <fbxsdk.h>

//using namespace fbxsdk;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class FbxMobelApp : public D3DApp//, public FbxStream
{
public:
	FbxMobelApp(HINSTANCE hInstance);
	~FbxMobelApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

	void LoadNode(FbxNode* node, GeometryGenerator::MeshData& meshData, UINT vertexOffset);

private:
	ID3D11Buffer* mVB;
	ID3D11Buffer* mIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mSphereWorld[10];
	XMFLOAT4X4 mCylWorld[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mCenterSphere;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mFbxModelVertexOffset;

	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;
	UINT mFbxModelIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;
	UINT mFbxModelIndexCount;

	float mTheta;
	float mPhi;
	float mRadius;

	FbxManager*	fbxMng;
	FbxScene* fbxScene;
	FbxImporter* fbxImporter;

	std::vector<Vertex> fbxVec;

	POINT mLastMousePos;

	//////////////////////

};

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
	: D3DApp(hInstance), mVB(0), mIB(0), mFX(0), mTech(0),
	mfxWorldViewProj(0), mInputLayout(0), mWireframeRS(0),
	mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(15.0f)
{
	mMainWndCaption = L"Shapes Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX boxScale = XMMatrixScaling(0.01f, 0.01f, 0.01f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale);

	XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));
}

FbxMobelApp::~FbxMobelApp()
{
	fbxScene->Clear();
	fbxScene->Destroy();

	fbxMng->Destroy();

	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
}

bool FbxMobelApp::Init()
{
	if (!D3DApp::Init())
		return false;

	//fbxScene = FbxScene::Create(fbxMng, "Test");

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

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
	//md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	//md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	//
	//md3dImmediateContext->IASetInputLayout(mInputLayout);
	//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//
	//md3dImmediateContext->RSSetState(mWireframeRS);
	//
	//UINT stride = sizeof(Vertex);
	//UINT offset = 0;
	//md3dImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	//md3dImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
	//
	//// Set constants
	//
	//XMMATRIX view  = XMLoadFloat4x4(&mView);
	//XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	//XMMATRIX viewProj = view*proj;
	//
	//D3DX11_TECHNIQUE_DESC techDesc;
	//mTech->GetDesc( &techDesc );
	//for(UINT p = 0; p < techDesc.Passes; ++p)
	//{
	//	// Draw the grid.
	//	XMMATRIX world = XMLoadFloat4x4(&mGridWorld);
	//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world*viewProj)));
	//	mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
	//
	//	//D3D11_RASTERIZER_DESC wireframeDesc;
	//	//ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	//	//wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	//	//wireframeDesc.CullMode = D3D11_CULL_BACK;
	//	//wireframeDesc.FrontCounterClockwise = false;
	//	//wireframeDesc.DepthClipEnable = true;
	//	//
	//	//HR(md3dDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));
	//
	//	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//
	//	// Draw center sphere.
	//	world = XMLoadFloat4x4(&mCenterSphere);
	//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world*viewProj)));
	//	mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
	//
	//	// Draw fbx model.
	//	world = XMLoadFloat4x4(&mCenterSphere);
	//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world*viewProj)));
	//	mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->DrawIndexed(mFbxModelIndexCount, mFbxModelIndexOffset, mFbxModelVertexOffset);
	//
	//}
	//
	//HR(mSwapChain->Present(0, 0));

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view * proj;

	XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&(world * viewProj)));

	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);

		// 36 indices for the box.
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

	bool isSuccess = fbxImporter->Initialize("FbxModel/Box2.fbx", -1, fbxMng->GetIOSettings());
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
	std::vector<Vertex> vertices(fbxModel.Vertices.size());
	for (size_t i = 0; i < fbxModel.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = fbxModel.Vertices[i].Position;
		XMFLOAT4 color((rand() % 255) / 255.0f, (rand() % 255) / 255.0f, (rand() % 255) / 255.0f, (rand() % 255) / 255.0f);
		vertices[k].Color = color;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * fbxModel.Vertices.size();
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

	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 0, md3dDevice, &mFX));

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
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
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
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
};
