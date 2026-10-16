// BlockoutStudio - 导出系统实现：OBJ / ASCII FBX / glTF 2.0 / USDA / USDZ / 自定义格式
#include "BlockoutExportLib.h"
#include "BlockoutActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshOverlay.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FIndex3i;

namespace
{
	struct FExportMesh
	{
		FString Name;
		TArray<FVector3d> Verts;
		TArray<FVector3d> Normals;
		TArray<FVector2d> UVs;
		TArray<int32> Tris;      // 3 per face
		TArray<int32> NormalIdx; // 3 per face
		TArray<int32> UVIdx;     // 3 per face
		FLinearColor Color = FLinearColor::White;
		FTransform Transform = FTransform::Identity;
	};

	void GatherMeshes(const TArray<ABlockoutActor*>& Actors, TArray<FExportMesh>& Out, bool bWorldSpace)
	{
		for (ABlockoutActor* Actor : Actors)
		{
			if (!Actor) continue;
			FExportMesh EM;
			EM.Name = Actor->Data.Name.IsEmpty() ? TEXT("Block") : Actor->Data.Name;
			EM.Color = Actor->Data.Color;
			EM.Transform = Actor->Data.Transform;
			const FDynamicMesh3& M = Actor->CachedMesh;
			TMap<int32, int32> VertMap;
			for (int32 ti = 0; ti < M.TriangleCount(); ti++)
			{
				FIndex3i Tri = M.GetTriangle(ti);
				for (int32 c = 0; c < 3; c++)
				{
					int32 Src = Tri[c];
					int32* Found = VertMap.Find(Src);
					int32 Idx;
					if (Found) { Idx = *Found; }
					else
					{
						FVector3d P = M.GetVertex(Src);
						if (bWorldSpace) P = FVector3d(EM.Transform.TransformPosition(FVector(P)));
						EM.Verts.Add(P);
						Idx = EM.Verts.Num() - 1;
						VertMap.Add(Src, Idx);
					}
					EM.Tris.Add(Idx);
					// 法线/UV
					FVector3d N(0, 0, 1);
					FVector2d UV(0, 0);
					if (M.HasAttributes())
					{
						if (M.Attributes()->NumNormalLayers() > 0)
						{
							auto* NL = M.Attributes()->GetNormalLayer(0);
							if (NL && NL->IsElement(ti))
							{
								FVector3f NF = NL->GetElement(NL->GetTriangle(ti)[c]);
								N = FVector3d(NF);
							}
						}
						if (M.Attributes()->NumUVLayers() > 0)
						{
							auto* UL = M.Attributes()->GetUVLayer(0);
							if (UL && UL->IsElement(ti))
							{
								FVector2f UF = UL->GetElement(UL->GetTriangle(ti)[c]);
								UV = FVector2d(UF);
							}
						}
					}
					EM.Normals.Add(N);
					EM.NormalIdx.Add(EM.Normals.Num() - 1);
					EM.UVs.Add(UV);
					EM.UVIdx.Add(EM.UVs.Num() - 1);
				}
			}
			Out.Add(MoveTemp(EM));
		}
	}
}

// ---------------- OBJ ----------------
bool FBlockoutExportLib::ExportOBJ(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bWorldSpace)
{
	TArray<FExportMesh> Meshes;
	GatherMeshes(Actors, Meshes, bWorldSpace);
	TStringBuilder<4096> Sb;
	Sb.Appendf(TEXT("# BlockoutStudio OBJ Export\n# units: cm\n"));
	int32 VBase = 1, NBase = 1, TBase = 1;
	for (const FExportMesh& M : Meshes)
	{
		Sb.Appendf(TEXT("o %s\n"), *M.Name);
		for (const FVector3d& V : M.Verts)
		{
			Sb.Appendf(TEXT("v %.6f %.6f %.6f\n"), V.X, V.Y, V.Z);
		}
		for (const FVector2d& UV : M.UVs)
		{
			Sb.Appendf(TEXT("vt %.6f %.6f\n"), UV.X, UV.Y);
		}
		for (const FVector3d& N : M.Normals)
		{
			Sb.Appendf(TEXT("vn %.6f %.6f %.6f\n"), N.X, N.Y, N.Z);
		}
		for (int32 i = 0; i + 2 < M.Tris.Num(); i += 3)
		{
			int32 A = M.Tris[i] + VBase, B = M.Tris[i + 1] + VBase, C = M.Tris[i + 2] + VBase;
			int32 TA = M.UVIdx[i] + TBase, TB = M.UVIdx[i + 1] + TBase, TC = M.UVIdx[i + 2] + TBase;
			int32 NA = M.NormalIdx[i] + NBase, NB = M.NormalIdx[i + 1] + NBase, NC = M.NormalIdx[i + 2] + NBase;
			Sb.Appendf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d\n"), A, TA, NA, B, TB, NB, C, TC, NC);
		}
		VBase += M.Verts.Num();
		TBase += M.UVs.Num();
		NBase += M.Normals.Num();
	}
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}

// ---------------- ASCII FBX（简化版，UE/Autodesk 工具可导入） ----------------
bool FBlockoutExportLib::ExportFBX(const TArray<ABlockoutActor*>& Actors, const FString& FilePath)
{
	TArray<FExportMesh> Meshes;
	GatherMeshes(Actors, Meshes, true);
	TStringBuilder<8192> Sb;
	Sb.Append(TEXT("; FBX 7.4.0 project file\nFBXHeaderExtension:  {\n\tFBXHeaderVersion: 1003\n\tFBXVersion: 7400\n\tCreator: \"BlockoutStudio\"\n}\n"));
	Sb.Append(TEXT("GlobalSettings:  {\n\tVersion: 1000\n\tProperties70:  {\n\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1.0\n\t}\n}\n"));
	// Objects
	int32 GeoIdBase = 1000000;
	for (int32 mi = 0; mi < Meshes.Num(); mi++)
	{
		const FExportMesh& M = Meshes[mi];
		int32 GeoId = GeoIdBase + mi * 10;
		int32 ModelId = GeoId + 1;
		int32 MatId = GeoId + 2;
		Sb.Appendf(TEXT("Objects:  {\n\tGeometry: %d, \"Geometry::%s\", \"Mesh\" {\n"), GeoId, *M.Name);
		Sb.Appendf(TEXT("\t\tVertices: *%d {\n\t\t\ta: "), M.Verts.Num() * 3);
		for (int32 i = 0; i < M.Verts.Num(); i++)
		{
			Sb.Appendf(TEXT("%s%.6f,%.6f,%.6f"), i ? TEXT(",") : TEXT(""), M.Verts[i].X, M.Verts[i].Y, M.Verts[i].Z);
		}
		Sb.Append(TEXT("\n\t\t}\n\t\tPolygonVertexIndex: *"));
		TArray<int32> PolyIdx;
		for (int32 i = 0; i + 2 < M.Tris.Num(); i += 3)
		{
			PolyIdx.Add(M.Tris[i]);
			PolyIdx.Add(M.Tris[i + 1]);
			PolyIdx.Add(-(M.Tris[i + 2] + 1)); // FBX 终结面负一
		}
		Sb.Appendf(TEXT("%d {\n\t\t\ta: "), PolyIdx.Num());
		for (int32 i = 0; i < PolyIdx.Num(); i++)
		{
			Sb.Appendf(TEXT("%s%d"), i ? TEXT(",") : TEXT(""), PolyIdx[i]);
		}
		Sb.Append(TEXT("\n\t\t}\n\t\tGeometryVersion: 124\n\t}\n"));
		// Model
		Sb.Appendf(TEXT("\tModel: %d, \"Model::%s\", \"Mesh\" {\n\t\tProperties70:  {\n\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",%.6f,%.6f,%.6f\n\t\t}\n\t\tShading: T\n\t\tCulling: \"CullingOff\"\n\t}\n"),
			ModelId, *M.Name, M.Transform.GetLocation().X, M.Transform.GetLocation().Y, M.Transform.GetLocation().Z);
		// Material
		Sb.Appendf(TEXT("\tMaterial: %d, \"Material::%s_mat\", \"\" {\n\t\tShadingModel: \"lambert\"\n\t\tProperties70:  {\n\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\",%.4f,%.4f,%.4f\n\t\t}\n\t}\n"),
			MatId, *M.Name, M.Color.R, M.Color.G, M.Color.B);
		Sb.Append(TEXT("}\n"));
	}
	Sb.Append(TEXT("Connections:  {\n"));
	for (int32 mi = 0; mi < Meshes.Num(); mi++)
	{
		int32 GeoId = GeoIdBase + mi * 10;
		int32 ModelId = GeoId + 1;
		int32 MatId = GeoId + 2;
		Sb.Appendf(TEXT("\tC: \"OO\",%d,%d\n"), GeoId, ModelId);
		Sb.Appendf(TEXT("\tC: \"OO\",%d,%d\n"), MatId, ModelId);
		Sb.Appendf(TEXT("\tC: \"OO\",%d,0\n"), ModelId);
	}
	Sb.Append(TEXT("}\n"));
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}

// ---------------- glTF 2.0（.gltf + 内嵌 base64 buffer） ----------------
bool FBlockoutExportLib::ExportGLTF(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludeLighting, bool bIncludeCamera)
{
	TArray<FExportMesh> Meshes;
	GatherMeshes(Actors, Meshes, true);
	// 二进制 buffer：位置/法线/UV/索引
	TArray<uint8> Buffer;
	TArray<int32> AccessorRanges; // 记录每 mesh 偏移
	struct FMeshRange { int32 PosOff, PosLen, NrmOff, NrmLen, UvOff, UvLen, IdxOff, IdxLen, Count; };
	TArray<FMeshRange> Ranges;
	for (const FExportMesh& M : Meshes)
	{
		FMeshRange R;
		R.PosOff = Buffer.Num();
		for (const FVector3d& V : M.Verts) { float F[3] = { (float)V.X, (float)V.Y, (float)V.Z }; Buffer.Append((uint8*)F, 12); }
		R.PosLen = Buffer.Num() - R.PosOff;
		R.NrmOff = Buffer.Num();
		for (const FVector3d& N : M.Normals) { float F[3] = { (float)N.X, (float)N.Y, (float)N.Z }; Buffer.Append((uint8*)F, 12); }
		R.NrmLen = Buffer.Num() - R.NrmOff;
		R.UvOff = Buffer.Num();
		for (const FVector2d& UV : M.UVs) { float F[2] = { (float)UV.X, (float)UV.Y }; Buffer.Append((uint8*)F, 8); }
		R.UvLen = Buffer.Num() - R.UvOff;
		R.IdxOff = Buffer.Num();
		for (int32 Idx : M.Tris) { uint16 V16 = (uint16)Idx; Buffer.Append((uint8*)&V16, 2); }
		R.IdxLen = Buffer.Num() - R.IdxOff;
		R.Count = M.Verts.Num();
		Ranges.Add(R);
	}
	// base64
	const TCHAR* B64 = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	FString B64Str;
	B64Str.Reserve(Buffer.Num() * 2);
	for (int32 i = 0; i < Buffer.Num(); i += 3)
	{
		uint32 A = Buffer[i];
		uint32 B = (i + 1 < Buffer.Num()) ? Buffer[i + 1] : 0;
		uint32 C = (i + 2 < Buffer.Num()) ? Buffer[i + 2] : 0;
		uint32 D = (A << 16) | (B << 8) | C;
		B64Str.AppendChar(B64[(D >> 18) & 63]);
		B64Str.AppendChar(B64[(D >> 12) & 63]);
		B64Str.AppendChar(i + 1 < Buffer.Num() ? B64[(D >> 6) & 63] : TEXT('='));
		B64Str.AppendChar(i + 2 < Buffer.Num() ? B64[D & 63] : TEXT('='));
	}

	TStringBuilder<8192> Sb;
	Sb.Append(TEXT("{\n  \"asset\": {\"version\": \"2.0\", \"generator\": \"BlockoutStudio\"},\n"));
	Sb.Appendf(TEXT("  \"scene\": 0,\n  \"scenes\": [{\"nodes\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		Sb.Appendf(TEXT("%s%d"), i ? TEXT(",") : TEXT(""), i);
	}
	Sb.Append(TEXT("]}],\n  \"nodes\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		const FVector L = Meshes[i].Transform.GetLocation();
		Sb.Appendf(TEXT("%s{\"mesh\": %d, \"translation\": [%.4f,%.4f,%.4f], \"name\": \"%s\"}"), i ? TEXT(",") : TEXT(""), i, L.X, L.Y, L.Z, *Meshes[i].Name);
	}
	Sb.Append(TEXT("],\n  \"meshes\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		Sb.Appendf(TEXT("%s{\"primitives\": [{\"attributes\": {\"POSITION\": %d, \"NORMAL\": %d, \"TEXCOORD_0\": %d}, \"indices\": %d, \"material\": %d}]}"),
			i ? TEXT(",") : TEXT(""), i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3, i);
	}
	Sb.Append(TEXT("],\n  \"materials\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		Sb.Appendf(TEXT("%s{\"pbrMetallicRoughness\": {\"baseColorFactor\": [%.4f,%.4f,%.4f,%.4f], \"metallicFactor\": 0.1, \"roughnessFactor\": 0.8}}"),
			i ? TEXT(",") : TEXT(""), Meshes[i].Color.R, Meshes[i].Color.G, Meshes[i].Color.B, Meshes[i].Color.A);
	}
	Sb.Append(TEXT("],\n  \"accessors\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		const FMeshRange& R = Ranges[i];
		Sb.Appendf(TEXT("%s{\"bufferView\": %d, \"componentType\": 5126, \"count\": %d, \"type\": \"VEC3\"}"), i ? TEXT(",") : TEXT(""), i * 4, R.Count);
		Sb.Appendf(TEXT(",{\"bufferView\": %d, \"componentType\": 5126, \"count\": %d, \"type\": \"VEC3\"}"), i * 4 + 1, R.Count);
		Sb.Appendf(TEXT(",{\"bufferView\": %d, \"componentType\": 5126, \"count\": %d, \"type\": \"VEC2\"}"), i * 4 + 2, R.Count);
		Sb.Appendf(TEXT(",{\"bufferView\": %d, \"componentType\": 5123, \"count\": %d, \"type\": \"SCALAR\"}"), i * 4 + 3, R.IdxLen / 2);
	}
	Sb.Append(TEXT("],\n  \"bufferViews\": ["));
	for (int32 i = 0; i < Meshes.Num(); i++)
	{
		const FMeshRange& R = Ranges[i];
		Sb.Appendf(TEXT("%s{\"buffer\": 0, \"byteOffset\": %d, \"byteLength\": %d}"), i ? TEXT(",") : TEXT(""), R.PosOff, R.PosLen);
		Sb.Appendf(TEXT(",{\"buffer\": 0, \"byteOffset\": %d, \"byteLength\": %d}"), R.NrmOff, R.NrmLen);
		Sb.Appendf(TEXT(",{\"buffer\": 0, \"byteOffset\": %d, \"byteLength\": %d}"), R.UvOff, R.UvLen);
		Sb.Appendf(TEXT(",{\"buffer\": 0, \"byteOffset\": %d, \"byteLength\": %d}"), R.IdxOff, R.IdxLen);
	}
	Sb.Appendf(TEXT("],\n  \"buffers\": [{\"byteLength\": %d, \"uri\": \"data:application/octet-stream;base64,%s\"}]\n}\n"), Buffer.Num(), *B64Str);
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}

// ---------------- USDA ----------------
bool FBlockoutExportLib::ExportUSD(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludePhysics, bool bPBR)
{
	TArray<FExportMesh> Meshes;
	GatherMeshes(Actors, Meshes, true);
	TStringBuilder<8192> Sb;
	Sb.Append(TEXT("#usda 1.0\n(\n    defaultPrim = \"Blockout\"\n    metersPerUnit = 0.01\n    upAxis = \"Z\"\n)\n\ndef Xform \"Blockout\" (\n    kind = \"group\"\n)\n{\n"));
	for (int32 mi = 0; mi < Meshes.Num(); mi++)
	{
		const FExportMesh& M = Meshes[mi];
		FString SafeName = M.Name.Replace(TEXT("\""), TEXT("_"));
		Sb.Appendf(TEXT("    def Mesh \"%s\"\n    {\n        int[] faceVertexCounts = ["), *SafeName);
		for (int32 i = 0; i + 2 < M.Tris.Num(); i += 3)
		{
			Sb.Appendf(TEXT("%s3"), i ? TEXT(",") : TEXT(""));
		}
		Sb.Append(TEXT("]\n        int[] faceVertexIndices = ["));
		for (int32 i = 0; i < M.Tris.Num(); i++)
		{
			Sb.Appendf(TEXT("%s%d"), i ? TEXT(",") : TEXT(""), M.Tris[i]);
		}
		Sb.Append(TEXT("]\n        point3f[] points = ["));
		for (int32 i = 0; i < M.Verts.Num(); i++)
		{
			Sb.Appendf(TEXT("%s(%.4f, %.4f, %.4f)"), i ? TEXT(",") : TEXT(""), M.Verts[i].X, M.Verts[i].Y, M.Verts[i].Z);
		}
		Sb.Appendf(TEXT("]\n        uniform token subdivisionScheme = \"none\"\n        double3 xformOp:translate = (%.4f, %.4f, %.4f)\n        uniform token[] xformOpOrder = [\"xformOp:translate\"]\n"),
			M.Transform.GetLocation().X, M.Transform.GetLocation().Y, M.Transform.GetLocation().Z);
		if (bPBR)
		{
			Sb.Append(TEXT("        def Material \"Mat\"\n        {\n            def Shader \"PBR\"\n            {\n                uniform token info:id = \"UsdPreviewSurface\"\n"));
			Sb.Appendf(TEXT("                color3f inputs:diffuseColor = (%.4f, %.4f, %.4f)\n"), M.Color.R, M.Color.G, M.Color.B);
			Sb.Append(TEXT("                float inputs:roughness = 0.8\n                float inputs:metallic = 0.1\n            }\n        }\n"));
		}
		if (bIncludePhysics)
		{
			Sb.Append(TEXT("        def PhysicsRigidBodyAPI \"Physics\"\n        {\n            bool physics:rigidBodyEnabled = 0\n        }\n"));
		}
		Sb.Append(TEXT("    }\n"));
	}
	Sb.Append(TEXT("}\n"));
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}

bool FBlockoutExportLib::ExportUSDZ(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludePhysics, bool bPBR)
{
	// USDZ = zip 容器（store，不压缩）包含 usda。用 PowerShell Compress-Archive 生成兼容 zip。
	FString UsdaPath = FPaths::ChangeExtension(FilePath, TEXT("usda"));
	if (!ExportUSD(Actors, UsdaPath, bIncludePhysics, bPBR)) return false;
	FString ZipCmd = FString::Printf(TEXT("powershell -NoProfile -Command \"Compress-Archive -Force -Path '%s' -DestinationPath '%s'\""),
		*UsdaPath, *FilePath);
	int32 Ret = 0;
	FPlatformProcess::ExecProcess(*ZipCmd, nullptr, &Ret, nullptr, nullptr);
	return Ret == 0;
}

// ---------------- 自定义格式（JSON + 网格内嵌） ----------------
bool FBlockoutExportLib::ExportCustom(const TArray<ABlockoutActor*>& Actors, const FString& FilePath)
{
	TArray<FExportMesh> Meshes;
	GatherMeshes(Actors, Meshes, true);
	TStringBuilder<8192> Sb;
	Sb.Append(TEXT("{\n  \"format\": \"BlockoutStudioScene\",\n  \"version\": 1,\n  \"meshes\": ["));
	for (int32 mi = 0; mi < Meshes.Num(); mi++)
	{
		const FExportMesh& M = Meshes[mi];
		Sb.Appendf(TEXT("%s{\n    \"name\": \"%s\",\n    \"color\": [%.4f,%.4f,%.4f,%.4f],\n    \"vertices\": ["),
			mi ? TEXT(",") : TEXT(""), *M.Name, M.Color.R, M.Color.G, M.Color.B, M.Color.A);
		for (int32 i = 0; i < M.Verts.Num(); i++)
		{
			Sb.Appendf(TEXT("%s[%.4f,%.4f,%.4f]"), i ? TEXT(",") : TEXT(""), M.Verts[i].X, M.Verts[i].Y, M.Verts[i].Z);
		}
		Sb.Append(TEXT("],\n    \"triangles\": ["));
		for (int32 i = 0; i < M.Tris.Num(); i++)
		{
			Sb.Appendf(TEXT("%s%d"), i ? TEXT(",") : TEXT(""), M.Tris[i]);
		}
		Sb.Append(TEXT("],\n    \"normals\": ["));
		for (int32 i = 0; i < M.Normals.Num(); i++)
		{
			Sb.Appendf(TEXT("%s[%.4f,%.4f,%.4f]"), i ? TEXT(",") : TEXT(""), M.Normals[i].X, M.Normals[i].Y, M.Normals[i].Z);
		}
		Sb.Append(TEXT("],\n    \"uvs\": ["));
		for (int32 i = 0; i < M.UVs.Num(); i++)
		{
			Sb.Appendf(TEXT("%s[%.4f,%.4f]"), i ? TEXT(",") : TEXT(""), M.UVs[i].X, M.UVs[i].Y);
		}
		Sb.Append(TEXT("]\n  }"));
	}
	Sb.Append(TEXT("]\n}\n"));
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}
