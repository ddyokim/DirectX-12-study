#pragma once

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
#include "Effects.h"
#include "Vertex.h"
#include <vector>
#include <fbxsdk.h>
#include <d3dcompiler.h>

//using namespace fbxsdk;

//struct Vertex
//{
//	XMFLOAT3 Pos;
//	XMFLOAT3 Normal;
//	XMFLOAT2 Tex;
//};

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

	XMFLOAT3 ReadNormal(FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	XMFLOAT3 ReadBinormal(FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	XMFLOAT3 ReadTangent(FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	XMFLOAT2 ReadUV(FbxMesh* mesh, int controlPointIndex, int vertexCounter);

private:
	ID3D11Buffer* mVB;
	ID3D11Buffer* mIB;

	//ID3DX11Effect* mFX;
	//ID3DX11EffectTechnique* mTech;
	//ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	ID3D11ShaderResourceView* mDiffuseMapSRV;

	ID3D11RasterizerState* mWireframeRS;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mSphereWorld[10];
	XMFLOAT4X4 mCylWorld[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mCenterSphere;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	DirectionalLight mDirLights[3];
	Material mBoxMat;

	XMFLOAT3 mEyePosW;

	XMFLOAT4X4 mTexTransform;

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

	std::vector<Vertex::Basic32> fbxVec;

	POINT mLastMousePos;

	//////////////////////

};