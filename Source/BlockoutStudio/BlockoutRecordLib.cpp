// BlockoutStudio - 轨迹录制与视频导出实现
#include "BlockoutRecordLib.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

void FBlockoutRecordLib::StartRecording(FBlockoutRecordSession& Session, double WorldTime, int32 Fps)
{
	Session.Reset();
	Session.StartTime = WorldTime;
	Session.Fps = FMath::Clamp(Fps, 10, 60);
	Session.bRecording = true;
}

void FBlockoutRecordLib::StopRecording(FBlockoutRecordSession& Session, double WorldTime)
{
	Session.bRecording = false;
}

void FBlockoutRecordLib::AddFrame(FBlockoutRecordSession& Session, const FBlockoutRecordFrame& Frame)
{
	if (Session.bRecording)
	{
		Session.Frames.Add(Frame);
	}
}

void FBlockoutRecordLib::AddJpegFrame(FBlockoutRecordSession& Session, const TArray<uint8>& JpegData)
{
	if (Session.bRecording && JpegData.Num() > 0)
	{
		Session.JpegFrames.Add(JpegData);
	}
}

bool FBlockoutRecordLib::SaveTrajectoryJSON(const FBlockoutRecordSession& Session, const FString& Path)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("fps"), Session.Fps);
	Root->SetNumberField(TEXT("frameCount"), Session.Frames.Num());

	TArray<TSharedPtr<FJsonValue>> JsonFrames;
	for (const FBlockoutRecordFrame& F : Session.Frames)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("t"), F.Time);
		Obj->SetNumberField(TEXT("lx"), F.Location.X);
		Obj->SetNumberField(TEXT("ly"), F.Location.Y);
		Obj->SetNumberField(TEXT("lz"), F.Location.Z);
		Obj->SetNumberField(TEXT("rx"), F.Rotation.Pitch);
		Obj->SetNumberField(TEXT("ry"), F.Rotation.Yaw);
		Obj->SetNumberField(TEXT("rz"), F.Rotation.Roll);
		Obj->SetBoolField(TEXT("crouch"), F.bCrouch);
		Obj->SetNumberField(TEXT("fps"), F.bFPS ? 1 : 0);
		Obj->SetNumberField(TEXT("hx"), F.HeadBoneOffset.X);
		Obj->SetNumberField(TEXT("hy"), F.HeadBoneOffset.Y);
		Obj->SetNumberField(TEXT("hz"), F.HeadBoneOffset.Z);
		JsonFrames.Add(MakeShared<FJsonValueObject>(Obj));
	}
	Root->SetArrayField(TEXT("frames"), JsonFrames);

	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return FFileHelper::SaveStringToFile(Out, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool FBlockoutRecordLib::LoadTrajectoryJSON(FBlockoutRecordSession& Session, const FString& Path)
{
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *Path)) return false;

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

	Session.Reset();
	Session.Fps = (int32)Root->GetNumberField(TEXT("fps"));
	const TArray<TSharedPtr<FJsonValue>>* JsonFrames = nullptr;
	if (!Root->TryGetArrayField(TEXT("frames"), JsonFrames)) return false;

	for (const TSharedPtr<FJsonValue>& V : *JsonFrames)
	{
		const TSharedPtr<FJsonObject>& Obj = V->AsObject();
		if (!Obj.IsValid()) continue;
		FBlockoutRecordFrame F;
		F.Time = Obj->GetNumberField(TEXT("t"));
		F.Location = FVector(Obj->GetNumberField(TEXT("lx")), Obj->GetNumberField(TEXT("ly")), Obj->GetNumberField(TEXT("lz")));
		F.Rotation = FRotator(Obj->GetNumberField(TEXT("rx")), Obj->GetNumberField(TEXT("ry")), Obj->GetNumberField(TEXT("rz")));
		F.bCrouch = Obj->GetBoolField(TEXT("crouch"));
		F.bFPS = Obj->GetNumberField(TEXT("fps")) != 0;
		F.HeadBoneOffset = FVector(Obj->GetNumberField(TEXT("hx")), Obj->GetNumberField(TEXT("hy")), Obj->GetNumberField(TEXT("hz")));
		Session.Frames.Add(F);
	}
	return Session.Frames.Num() > 0;
}

// ---------- MJPEG-AVI 手工容器 ----------

void FBlockoutRecordLib::WriteChunkHeader(FArchive& Ar, const char* Id, uint32 Size)
{
	Ar.Serialize((void*)Id, 4);
	Ar << Size;
}

void FBlockoutRecordLib::WriteListHeader(FArchive& Ar, const char* ListType, uint32 SizeAfter)
{
	Ar.Serialize((void*)"LIST", 4);
	Ar << SizeAfter;
	Ar.Serialize((void*)ListType, 4);
}

bool FBlockoutRecordLib::WriteMjpegAVI(const FBlockoutRecordSession& Session, const FString& Path)
{
	const int32 FrameCount = Session.JpegFrames.Num();
	if (FrameCount == 0) return false;

	const uint32 W = Session.Width > 0 ? Session.Width : 1280;
	const uint32 H = Session.Height > 0 ? Session.Height : 720;
	const uint32 Fps = Session.Fps > 0 ? Session.Fps : 30;

	// movi 内容：每帧 '00dc' + size + jpeg（偶数补齐）
	TArray<uint8> Movi;
	uint32 Idx1Size = 0;
	TArray<uint8> Idx1;
	TArray<uint32> FrameOffsets; // 相对 movi list 起点

	for (const TArray<uint8>& Jpeg : Session.JpegFrames)
	{
		const uint32 Payload = (uint32)Jpeg.Num();
		const uint32 Padded = Payload + (Payload & 1);
		const uint32 OffsetInMovi = 4 + (uint32)Movi.Num(); // 4 = "movi" 四字符后开始偏移（含在list中）
		FrameOffsets.Add(OffsetInMovi);

		const uint32 Bytes = Padded + 8;
		Movi.SetNumUninitialized(Movi.Num() + (int32)Bytes);
		uint8* Ptr = Movi.GetData() + Movi.Num() - (int32)Bytes;
		FMemory::Memcpy(Ptr, "00dc", 4);
		FMemory::Memcpy(Ptr + 4, &Payload, 4);
		FMemory::Memcpy(Ptr + 8, Jpeg.GetData(), Jpeg.Num());
		if (Payload & 1) Ptr[8 + Jpeg.Num()] = 0;
	}

	// idx1 条目：每帧 16 字节
	for (uint32 Offset : FrameOffsets)
	{
		const int32 Base = Idx1.Num();
		Idx1.SetNumUninitialized(Base + 16);
		uint8* P = Idx1.GetData() + Base;
		FMemory::Memcpy(P, "00dc", 4);
		uint32 Flags = 0x10; // AVIIF_KEYFRAME
		FMemory::Memcpy(P + 4, &Flags, 4);
		FMemory::Memcpy(P + 8, &Offset, 4);
		uint32 Size = (uint32)Session.JpegFrames[FrameOffsets.Find(Offset)].Num();
		Size += Size & 1;
		FMemory::Memcpy(P + 12, &Size, 4);
	}
	Idx1Size = (uint32)Idx1.Num();

	// hdrl
	uint8 Avih[56] = { 0 };
	const uint32 Uspf = 1000000 / Fps;
	FMemory::Memcpy(Avih + 0, &Uspf, 4);
	uint32 MaxBytesPerSec = 0;
	for (const TArray<uint8>& J : Session.JpegFrames) MaxBytesPerSec += J.Num();
	MaxBytesPerSec = MaxBytesPerSec / FMath::Max(1, (int32)Fps);
	FMemory::Memcpy(Avih + 4, &MaxBytesPerSec, 4);
	uint32 Pad = 0; FMemory::Memcpy(Avih + 8, &Pad, 4);
	uint32 Flags = 0x10; FMemory::Memcpy(Avih + 12, &Flags, 4);
	uint32 Total = (uint32)FrameCount; FMemory::Memcpy(Avih + 16, &Total, 4);
	uint32 InitFrames = 0; FMemory::Memcpy(Avih + 20, &InitFrames, 4);
	uint32 Streams = 1; FMemory::Memcpy(Avih + 24, &Streams, 4);
	uint32 BufSize = MaxBytesPerSec; FMemory::Memcpy(Avih + 28, &BufSize, 4);
	FMemory::Memcpy(Avih + 32, &W, 4);
	FMemory::Memcpy(Avih + 36, &H, 4);

	// strh (56 bytes)
	uint8 Strh[56] = { 0 };
	FMemory::Memcpy(Strh + 0, "vids", 4);
	FMemory::Memcpy(Strh + 4, "MJPG", 4);
	uint32 SFlags = 0; FMemory::Memcpy(Strh + 8, &SFlags, 4);
	uint16 Prio = 0, Lang = 0; FMemory::Memcpy(Strh + 12, &Prio, 2); FMemory::Memcpy(Strh + 14, &Lang, 2);
	uint32 Init = 0; FMemory::Memcpy(Strh + 16, &Init, 4);
	uint32 Scale = 1, Rate = Fps;
	FMemory::Memcpy(Strh + 20, &Scale, 4);
	FMemory::Memcpy(Strh + 24, &Rate, 4);
	uint32 Start = 0; FMemory::Memcpy(Strh + 28, &Start, 4);
	FMemory::Memcpy(Strh + 32, &Total, 4);
	uint32 SBuf = MaxBytesPerSec; FMemory::Memcpy(Strh + 36, &SBuf, 4);
	uint32 Qual = 0xFFFFFFFF; FMemory::Memcpy(Strh + 40, &Qual, 4);
	uint32 SampleSize = 0; FMemory::Memcpy(Strh + 44, &SampleSize, 4);
	uint16 Rc[4] = { 0, 0, (uint16)W, (uint16)H }; FMemory::Memcpy(Strh + 48, Rc, 8);

	// strf BITMAPINFOHEADER (40 bytes)
	uint8 Strf[40] = { 0 };
	uint32 StrfSize = 40; FMemory::Memcpy(Strf + 0, &StrfSize, 4);
	FMemory::Memcpy(Strf + 4, &W, 4);
	int32 SignedH = (int32)H; FMemory::Memcpy(Strf + 8, &SignedH, 4);
	uint16 Planes = 1; FMemory::Memcpy(Strf + 12, &Planes, 2);
	uint16 BitCount = 24; FMemory::Memcpy(Strf + 14, &BitCount, 2);
	FMemory::Memcpy(Strf + 16, "MJPG", 4);
	uint32 ImageSize = W * H * 3; FMemory::Memcpy(Strf + 20, &ImageSize, 4);

	// 组装
	TArray<uint8> File;
	FArchive& Ar = *(new FMemoryWriter(File));

	const uint32 StrlSize = 4 + 8 + 56 + 8 + 40;                 // strl list 内容
	const uint32 HdrlContent = 4 + 8 + 56 + (8 + StrlSize);      // avih + strl
	const uint32 MoviContent = 4 + (uint32)Movi.Num();
	const uint32 RiffSize = 4 + (8 + HdrlContent) + (8 + MoviContent) + (8 + Idx1Size);

	WriteChunkHeader(Ar, "RIFF", RiffSize);
	Ar.Serialize((void*)"AVI ", 4);

	WriteListHeader(Ar, "hdrl", HdrlContent);
	WriteChunkHeader(Ar, "avih", 56);
	Ar.Serialize(Avih, 56);
	WriteListHeader(Ar, "strl", StrlSize);
	WriteChunkHeader(Ar, "strh", 56);
	Ar.Serialize(Strh, 56);
	WriteChunkHeader(Ar, "strf", 40);
	Ar.Serialize(Strf, 40);

	WriteListHeader(Ar, "movi", MoviContent);
	Ar.Serialize(Movi.GetData(), Movi.Num());

	WriteChunkHeader(Ar, "idx1", Idx1Size);
	Ar.Serialize(Idx1.GetData(), Idx1.Num());

	Ar.Close();
	delete &Ar;

	return FFileHelper::SaveArrayToFile(File, *Path);
}

bool FBlockoutRecordLib::SamplePlayback(const TArray<FBlockoutRecordFrame>& Frames, double PlayTime, FBlockoutRecordFrame& OutFrame)
{
	if (Frames.Num() == 0) return false;
	if (PlayTime <= Frames[0].Time) { OutFrame = Frames[0]; return true; }
	if (PlayTime >= Frames.Last().Time) { OutFrame = Frames.Last(); return true; }

	for (int32 i = 1; i < Frames.Num(); ++i)
	{
		if (Frames[i].Time >= PlayTime)
		{
			const FBlockoutRecordFrame& A = Frames[i - 1];
			const FBlockoutRecordFrame& B = Frames[i];
			const double Span = B.Time - A.Time;
			const double Alpha = Span > KINDA_SMALL_NUMBER ? (PlayTime - A.Time) / Span : 0.0;
			OutFrame.Time = PlayTime;
			OutFrame.Location = FMath::Lerp(A.Location, B.Location, Alpha);
			OutFrame.Rotation = FMath::Lerp(A.Rotation, B.Rotation, (float)Alpha);
			OutFrame.bCrouch = B.bCrouch;
			OutFrame.bFPS = B.bFPS;
			OutFrame.HeadBoneOffset = FMath::Lerp(A.HeadBoneOffset, B.HeadBoneOffset, Alpha);
			return true;
		}
	}
	OutFrame = Frames.Last();
	return true;
}
