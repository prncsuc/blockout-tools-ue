// BlockoutStudio - 轨迹录制与视频导出库（R1-R3，MJPEG-AVI）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"

/**
 * 录制会话：相机轨迹帧 + 可选 JPEG 视频帧
 */
struct BLOCKOUTSTUDIO_API FBlockoutRecordSession
{
	TArray<FBlockoutRecordFrame> Frames;
	TArray<TArray<uint8>> JpegFrames; // 逐帧 JPEG（MJPEG-AVI 用）
	int32 Width = 0;
	int32 Height = 0;
	int32 Fps = 30;
	double StartTime = 0.0;
	bool bRecording = false;

	void Reset()
	{
		Frames.Reset();
		JpegFrames.Reset();
		bRecording = false;
	}
};

class BLOCKOUTSTUDIO_API FBlockoutRecordLib
{
public:
	// R1：开始/停止录制
	static void StartRecording(FBlockoutRecordSession& Session, double WorldTime, int32 Fps = 30);
	static void StopRecording(FBlockoutRecordSession& Session, double WorldTime);

	// 每帧采集：轨迹帧（含头部骨骼偏移用于第一人称回放）
	static void AddFrame(FBlockoutRecordSession& Session, const FBlockoutRecordFrame& Frame);

	// 每帧采集：JPEG 视频帧（MJPEG-AVI）
	static void AddJpegFrame(FBlockoutRecordSession& Session, const TArray<uint8>& JpegData);

	// R3：轨迹 JSON 存取
	static bool SaveTrajectoryJSON(const FBlockoutRecordSession& Session, const FString& Path);
	static bool LoadTrajectoryJSON(FBlockoutRecordSession& Session, const FString& Path);

	// R3：MJPEG-AVI 导出（纯手工 RIFF 容器，无第三方依赖）
	static bool WriteMjpegAVI(const FBlockoutRecordSession& Session, const FString& Path);

	// R2：视频封装 MP4/WebM（ffmpeg AVI→MP4(H.264)/WebM(VP8) 转码封装）
	// 查找顺序：exe 同目录 → 项目 ThirdParty/ffmpeg → 系统 PATH；找不到返回空
	static FString FindFFmpeg();
	static bool ConvertToMp4(const FString& AviPath, const FString& Mp4Path);
	static bool ConvertToWebM(const FString& AviPath, const FString& WebMPath);
	// 按输出扩展名(.mp4/.webm)自动选择编码；ffmpeg 缺失时保留 AVI 并返回 false
	static bool ConvertVideo(const FString& AviPath, const FString& OutPath);

	// R2：回放：按轨迹时间采样当前相机变换
	static bool SamplePlayback(const TArray<FBlockoutRecordFrame>& Frames, double PlayTime, FBlockoutRecordFrame& OutFrame);

private:
	static void WriteChunkHeader(FArchive& Ar, const char* Id, uint32 Size);
	static void WriteListHeader(FArchive& Ar, const char* ListType, uint32 SizeAfter);
};
