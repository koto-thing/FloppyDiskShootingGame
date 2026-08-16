#pragma once

#include <string>

// オーディオ管理サービス
class AudioService {
public:
	AudioService();
	~AudioService();

	// 初期化と終了処理
	bool Initialize();
	void Shutdown();

	// テスト用ビープ音再生
	void Play440HzSound();

	// MML再生
	void PlayMMLBGM(const std::string& mml, bool loop = true);
	void PlayMMLSE(const std::string& mml);
	bool PlayMMLBGMFromFile(const std::string& filePath, bool loop = true);
	bool PlayMMLSEFromFile(const std::string& filePath);
	void StopBGM();

private:
	struct Impl;
	Impl* impl;
};