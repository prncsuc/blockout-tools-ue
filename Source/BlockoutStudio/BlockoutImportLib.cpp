// BlockoutStudio - D4: 导入 OBJ / glTF / GLB 作为参考体网格
#include "BlockoutImportLib.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	// ---------------- OBJ ----------------
	bool ImportOBJ(const FString& Content, TArray<FVector>& OutVerts, TArray<int32>& OutTris, FString& OutError)
	{
		TArray<FString> Lines;
		Content.ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			const FString Trim = Line.TrimStartAndEnd();
			if (Trim.StartsWith(TEXT("v ")))
			{
				TArray<FString> Parts;
				Trim.ParseIntoArrayWS(Parts);
				if (Parts.Num() >= 4)
				{
					OutVerts.Add(FVector(
						FCString::Atof(*Parts[1]),
						FCString::Atof(*Parts[2]),
						FCString::Atof(*Parts[3])));
				}
			}
			else if (Trim.StartsWith(TEXT("f ")))
			{
				TArray<FString> Parts;
				Trim.ParseIntoArrayWS(Parts);
				TArray<int32> Idx;
				for (int32 i = 1; i < Parts.Num(); ++i)
				{
					// 形如 v / vt / vn，仅取 v；支持负索引
					FString VStr = Parts[i];
					int32 Slash;
					if (VStr.FindChar(TEXT('/'), Slash)) VStr = VStr.Left(Slash);
					int32 V = FCString::Atoi(*VStr);
					if (V < 0) V = OutVerts.Num() + V + 1; // 负索引（1-based）
					Idx.Add(V - 1);                        // 转 0-based
				}
				// 扇形三角化
				for (int32 i = 1; i + 1 < Idx.Num(); ++i)
				{
					if (Idx[0] >= 0 && Idx[0] < OutVerts.Num() &&
						Idx[i] >= 0 && Idx[i] < OutVerts.Num() &&
						Idx[i + 1] >= 0 && Idx[i + 1] < OutVerts.Num())
					{
						OutTris.Add(Idx[0]);
						OutTris.Add(Idx[i]);
						OutTris.Add(Idx[i + 1]);
					}
				}
			}
		}
		if (OutTris.Num() == 0)
		{
			OutError = TEXT("OBJ 中未找到有效三角面");
			return false;
		}
		return true;
	}

	// ---------------- glTF / GLB 通用（基于 JSON） ----------------
	bool ReadBufferBytes(const FJsonObject& Root, const FString& BaseDir, int32 BufferIdx, TArray<uint8>& OutBytes, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Buffers = nullptr;
		if (!Root.TryGetArrayField(TEXT("buffers"), Buffers) || !Buffers->IsValidIndex(BufferIdx))
		{
			OutError = TEXT("glTF 缺少 buffers");
			return false;
		}
		const TSharedPtr<FJsonObject> Buf = (*Buffers)[BufferIdx]->AsObject();
		if (!Buf.IsValid()) { OutError = TEXT("glTF buffer 无效"); return false; }

		FString Uri;
		if (!Buf->TryGetStringField(TEXT("uri"), Uri))
		{
			OutError = TEXT("GLB 内部 buffer 需在 GLB 结构中解析（当前不支持该文件布局）");
			return false;
		}
		if (Uri.StartsWith(TEXT("data:")))
		{
			int32 Comma;
			if (!Uri.FindChar(TEXT(','), Comma)) { OutError = TEXT("data URI 无效"); return false; }
			const FString B64 = Uri.Mid(Comma + 1);
			if (!FBase64::Decode(B64, OutBytes)) { OutError = TEXT("data URI base64 解码失败"); return false; }
			return true;
		}
		// 外部 .bin
		const FString BinPath = BaseDir / Uri;
		if (!FFileHelper::LoadFileToArray(OutBytes, *BinPath))
		{
			OutError = FString::Printf(TEXT("无法读取 buffer 文件: %s"), *BinPath);
			return false;
		}
		return true;
	}

	bool ImportGLTFJson(const TSharedPtr<FJsonObject>& Root, const FString& BaseDir, TArray<FVector>& OutVerts, TArray<int32>& OutTris, FString& OutError)
	{
		if (!Root.IsValid()) { OutError = TEXT("glTF JSON 解析失败"); return false; }

		// buffers 缓存
		TArray<TArray<uint8>> BufferCache;
		{
			const TArray<TSharedPtr<FJsonValue>>* Buffers = nullptr;
			int32 Num = 0;
			if (Root->TryGetArrayField(TEXT("buffers"), Buffers)) Num = Buffers->Num();
			BufferCache.SetNum(FMath::Max(Num, 1));
			for (int32 i = 0; i < Num; ++i)
			{
				if (!ReadBufferBytes(*Root, BaseDir, i, BufferCache[i], OutError)) return false;
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* Accessors = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* BufferViews = nullptr;
		Root->TryGetArrayField(TEXT("accessors"), Accessors);
		Root->TryGetArrayField(TEXT("bufferViews"), BufferViews);

		// 读取 VEC3/SCALAR accessor
		auto ReadAccessor = [&](int32 AccessorIdx, TArray<float>& OutFloats, TArray<int32>& OutInts) -> bool
		{
			if (!Accessors || !Accessors->IsValidIndex(AccessorIdx)) return false;
			const TSharedPtr<FJsonObject> Acc = (*Accessors)[AccessorIdx]->AsObject();
			if (!Acc.IsValid()) return false;
			int32 BufferViewIdx = -1, ByteOffset = 0, Count = 0, ComponentType = 5126;
			Acc->TryGetNumberField(TEXT("bufferView"), BufferViewIdx);
			Acc->TryGetNumberField(TEXT("byteOffset"), ByteOffset);
			Acc->TryGetNumberField(TEXT("count"), Count);
			Acc->TryGetNumberField(TEXT("componentType"), ComponentType);
			FString Type;
			Acc->TryGetStringField(TEXT("type"), Type);

			if (!BufferViews || !BufferViews->IsValidIndex(BufferViewIdx)) return false;
			const TSharedPtr<FJsonObject> Bv = (*BufferViews)[BufferViewIdx]->AsObject();
			if (!Bv.IsValid()) return false;
			int32 BufIdx = 0, BvOffset = 0, BvLen = 0;
			Bv->TryGetNumberField(TEXT("buffer"), BufIdx);
			Bv->TryGetNumberField(TEXT("byteOffset"), BvOffset);
			Bv->TryGetNumberField(TEXT("byteLength"), BvLen);
			if (!BufferCache.IsValidIndex(BufIdx)) return false;
			const TArray<uint8>& Bytes = BufferCache[BufIdx];

			const int32 Base = BvOffset + ByteOffset;
			const int32 CompCount = (Type == TEXT("VEC3")) ? 3 : (Type == TEXT("VEC2")) ? 2 : 1;

			if (ComponentType == 5126) // float
			{
				for (int32 i = 0; i < Count * CompCount; ++i)
				{
					const int32 Off = Base + i * 4;
					if (Off + 4 > Bytes.Num()) return false;
					float V;
					FMemory::Memcpy(&V, &Bytes[Off], 4);
					OutFloats.Add(V);
				}
			}
			else // 索引类
			{
				for (int32 i = 0; i < Count * CompCount; ++i)
				{
					const int32 Off = Base + i * (ComponentType == 5125 ? 4 : ComponentType == 5123 ? 2 : 1);
					if (Off + (ComponentType == 5125 ? 4 : ComponentType == 5123 ? 2 : 1) > Bytes.Num()) return false;
					int32 V = 0;
					if (ComponentType == 5125) { uint32 T; FMemory::Memcpy(&T, &Bytes[Off], 4); V = (int32)T; }
					else if (ComponentType == 5123) { uint16 T; FMemory::Memcpy(&T, &Bytes[Off], 2); V = (int32)T; }
					else { V = (int32)Bytes[Off]; }
					OutInts.Add(V);
				}
			}
			return true;
		};

		// 遍历 meshes[].primitives[]
		const TArray<TSharedPtr<FJsonValue>>* Meshes = nullptr;
		if (!Root->TryGetArrayField(TEXT("meshes"), Meshes) || Meshes->Num() == 0)
		{
			OutError = TEXT("glTF 中没有 meshes");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& MeshVal : *Meshes)
		{
			const TSharedPtr<FJsonObject> MeshObj = MeshVal->AsObject();
			if (!MeshObj.IsValid()) continue;
			const TArray<TSharedPtr<FJsonValue>>* Prims = nullptr;
			if (!MeshObj->TryGetArrayField(TEXT("primitives"), Prims)) continue;
			for (const TSharedPtr<FJsonValue>& PrimVal : *Prims)
			{
				const TSharedPtr<FJsonObject> Prim = PrimVal->AsObject();
				if (!Prim.IsValid()) continue;
				const TSharedPtr<FJsonObject>* Attrs = nullptr;
				if (!Prim->TryGetObjectField(TEXT("attributes"), Attrs)) continue;
				int32 PosAcc = -1, IdxAcc = -1;
				(*Attrs)->TryGetNumberField(TEXT("POSITION"), PosAcc);
				Prim->TryGetNumberField(TEXT("indices"), IdxAcc);
				if (PosAcc < 0) continue;

				TArray<float> Pos;
				TArray<int32> Dummy;
				if (!ReadAccessor(PosAcc, Pos, Dummy)) continue;

				const int32 VBase = OutVerts.Num();
				for (int32 i = 0; i + 2 < Pos.Num(); i += 3)
				{
					OutVerts.Add(FVector(Pos[i], Pos[i + 1], Pos[i + 2]));
				}

				if (IdxAcc >= 0)
				{
					TArray<float> DummyF;
					TArray<int32> Idx;
					if (ReadAccessor(IdxAcc, DummyF, Idx))
					{
						for (int32 i = 0; i + 2 < Idx.Num(); i += 3)
						{
							OutTris.Add(VBase + Idx[i]);
							OutTris.Add(VBase + Idx[i + 1]);
							OutTris.Add(VBase + Idx[i + 2]);
						}
					}
				}
				else
				{
					const int32 VCount = Pos.Num() / 3;
					for (int32 i = 0; i + 2 < VCount; i += 3)
					{
						OutTris.Add(VBase + i);
						OutTris.Add(VBase + i + 1);
						OutTris.Add(VBase + i + 2);
					}
				}
			}
		}

		if (OutTris.Num() == 0)
		{
			OutError = TEXT("glTF 中未找到有效三角面");
			return false;
		}
		return true;
	}
}

bool FBlockoutImportLib::ImportMesh(const FString& FilePath, TArray<FVector>& OutVerts, TArray<int32>& OutTris, FString& OutName, FString& OutError)
{
	OutVerts.Reset();
	OutTris.Reset();
	OutName = FPaths::GetBaseFilename(FilePath);

	if (!FPaths::FileExists(FilePath))
	{
		OutError = FString::Printf(TEXT("文件不存在: %s"), *FilePath);
		return false;
	}

	const FString Ext = FPaths::GetExtension(FilePath).ToLower();

	if (Ext == TEXT("obj"))
	{
		FString Content;
		if (!FFileHelper::LoadFileToString(Content, *FilePath))
		{
			OutError = TEXT("读取 OBJ 失败");
			return false;
		}
		if (!ImportOBJ(Content, OutVerts, OutTris, OutError)) return false;
	}
	else if (Ext == TEXT("gltf") || Ext == TEXT("glb"))
	{
		TSharedPtr<FJsonObject> Root;
		if (Ext == TEXT("glb"))
		{
			TArray<uint8> Bytes;
			if (!FFileHelper::LoadFileToArray(Bytes, *FilePath) || Bytes.Num() < 20)
			{
				OutError = TEXT("读取 GLB 失败");
				return false;
			}
			// GLB: 12 字节头 + JSON chunk（0x4E4F534A）
			uint32 Offset = 12;
			bool bJsonFound = false;
			FString JsonStr;
			while (Offset + 8 <= (uint32)Bytes.Num())
			{
				uint32 ChunkLen, ChunkType;
				FMemory::Memcpy(&ChunkLen, &Bytes[Offset], 4);
				FMemory::Memcpy(&ChunkType, &Bytes[Offset + 4], 4);
				Offset += 8;
				if (Offset + ChunkLen > (uint32)Bytes.Num()) break;
				if (ChunkType == 0x4E4F534A)
				{
					FUTF8ToTCHAR Conv((const ANSICHAR*)&Bytes[Offset], ChunkLen);
					JsonStr = FString(Conv.Length(), Conv.Get());
					bJsonFound = true;
					break;
				}
				Offset += ChunkLen;
			}
			if (!bJsonFound) { OutError = TEXT("GLB 未找到 JSON 块"); return false; }
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
			{
				OutError = TEXT("GLB JSON 解析失败");
				return false;
			}
			// GLB 内嵌 buffer 以无 uri 的 buffer 表示；此处附加 GLB BIN chunk 为 buffer0 兜底
			const TArray<TSharedPtr<FJsonValue>>* Buffers = nullptr;
			if (Root->TryGetArrayField(TEXT("buffers"), Buffers) && Buffers->Num() > 0)
			{
				bool bHasUri = false;
				FString UriVal;
				bHasUri = (*Buffers)[0]->AsObject()->TryGetStringField(TEXT("uri"), UriVal) && !UriVal.IsEmpty();
				if (!bHasUri)
				{
					// 退化处理：不支持 GLB 内嵌二进制，直接报错说明
					OutError = TEXT("GLB 内嵌 buffer 暂不支持，请使用 .gltf（含 data URI 或外部 .bin）");
					return false;
				}
			}
		}
		else
		{
			FString Content;
			if (!FFileHelper::LoadFileToString(Content, *FilePath))
			{
				OutError = TEXT("读取 glTF 失败");
				return false;
			}
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
			{
				OutError = TEXT("glTF JSON 解析失败");
				return false;
			}
		}
		if (!ImportGLTFJson(Root, FPaths::GetPath(FilePath), OutVerts, OutTris, OutError)) return false;

		// glTF 为米制 + Y-up，转 UE uu + Z-up：(x,y,z)→(x,-z,y)*100
		for (FVector& V : OutVerts)
		{
			const FVector S = V * 100.f;
			V = FVector(S.X, -S.Z, S.Y);
		}
	}
	else
	{
		OutError = FString::Printf(TEXT("不支持的格式: %s（仅 OBJ/glTF/GLB）"), *Ext);
		return false;
	}

	return true;
}
