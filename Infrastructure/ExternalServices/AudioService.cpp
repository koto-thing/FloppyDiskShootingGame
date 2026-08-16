#include "AudioService.h"
#include "MMLParser.h"
#include "SynthWaveGenerator.h"

#include <windows.h>
#include <xaudio2.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>

#pragma comment(lib, "xaudio2.lib")

// 内部実装
struct AudioService::Impl {
	IXAudio2* xAudio2 = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;

	IXAudio2SourceVoice* bgmVoice = nullptr;
	std::vector<int16_t> bgmPcmBuffer;

	IXAudio2SourceVoice* seVoice = nullptr;
	std::vector<int16_t> sePcmBuffer;

	WAVEFORMATEX waveFormat = {};

	Impl() {
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = 1;
		waveFormat.nSamplesPerSec = 44100;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	}
};

AudioService::AudioService() : impl(new Impl()) {}

AudioService::~AudioService() {
	Shutdown();
	delete impl;
}

bool AudioService::Initialize() {
	if (FAILED(XAudio2Create(&impl->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		return false;
	}
	if (FAILED(impl->xAudio2->CreateMasteringVoice(&impl->masteringVoice))) {
		return false;
	}
	return true;
}

void AudioService::Shutdown() {
	StopBGM();

	if (impl->seVoice) {
		impl->seVoice->Stop(0);
		impl->seVoice->DestroyVoice();
		impl->seVoice = nullptr;
	}
	if (impl->masteringVoice) {
		impl->masteringVoice->DestroyVoice();
		impl->masteringVoice = nullptr;
	}
	if (impl->xAudio2) {
		impl->xAudio2->Release();
		impl->xAudio2 = nullptr;
	}
}

void AudioService::StopBGM() {
	if (impl->bgmVoice) {
		impl->bgmVoice->Stop(0);
		impl->bgmVoice->FlushSourceBuffers();
		impl->bgmVoice->DestroyVoice();
		impl->bgmVoice = nullptr;
	}
	impl->bgmPcmBuffer.clear();
}

void AudioService::PlayMMLBGM(const std::string& mml, bool loop) {
	if (!impl->xAudio2) return;

	StopBGM();

	Audio::MMLParser parser;
	Audio::MMLSequence sequence = parser.Parse(mml);

	Audio::SynthWaveGenerator generator;
	impl->bgmPcmBuffer = generator.GeneratePCM(sequence, 44100);

	if (impl->bgmPcmBuffer.empty()) return;

	if (FAILED(impl->xAudio2->CreateSourceVoice(&impl->bgmVoice, &impl->waveFormat))) {
		return;
	}

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(impl->bgmPcmBuffer.size() * sizeof(int16_t));
	buffer.pAudioData = reinterpret_cast<const BYTE*>(impl->bgmPcmBuffer.data());
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

	if (SUCCEEDED(impl->bgmVoice->SubmitSourceBuffer(&buffer))) {
		impl->bgmVoice->Start(0);
	}
}

void AudioService::PlayMMLSE(const std::string& mml) {
	if (!impl->xAudio2) return;

	if (impl->seVoice) {
		impl->seVoice->Stop(0);
		impl->seVoice->FlushSourceBuffers();
		impl->seVoice->DestroyVoice();
		impl->seVoice = nullptr;
	}

	Audio::MMLParser parser;
	Audio::MMLSequence sequence = parser.Parse(mml);

	Audio::SynthWaveGenerator generator;
	impl->sePcmBuffer = generator.GeneratePCM(sequence, 44100);

	if (impl->sePcmBuffer.empty()) return;

	if (FAILED(impl->xAudio2->CreateSourceVoice(&impl->seVoice, &impl->waveFormat))) {
		return;
	}

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(impl->sePcmBuffer.size() * sizeof(int16_t));
	buffer.pAudioData = reinterpret_cast<const BYTE*>(impl->sePcmBuffer.data());
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	if (SUCCEEDED(impl->seVoice->SubmitSourceBuffer(&buffer))) {
		impl->seVoice->Start(0);
	}
}

bool AudioService::PlayMMLBGMFromFile(const std::string& filePath, bool loop) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		file.open("../" + filePath);
	}
	if (!file.is_open()) {
		return false;
	}
	std::string mml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	if (mml.empty()) {
		return false;
	}

	PlayMMLBGM(mml, loop);
	return true;
}

bool AudioService::PlayMMLSEFromFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		file.open("../" + filePath);
	}
	if (!file.is_open()) {
		return false;
	}
	std::string mml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	if (mml.empty()) {
		return false;
	}

	PlayMMLSE(mml);
	return true;
}

void AudioService::Play440HzSound() {
	if (!impl->xAudio2) return;

	IXAudio2SourceVoice* source = nullptr;
	if (FAILED(impl->xAudio2->CreateSourceVoice(&source, &impl->waveFormat))) {
		return;
	}

	constexpr int sampleRate = 44100;
	constexpr float frequency = 440.0f;
	constexpr float duration = 0.5f;

	int sampleCount = static_cast<int>(sampleRate * duration);
	std::vector<int16_t> samples(sampleCount);
	constexpr double PI = 3.14159265358979323846;

	for (int i = 0; i < sampleCount; i++) {
		double t = static_cast<double>(i) / sampleRate;
		double s = sin(2.0 * PI * frequency * t);
		samples[i] = static_cast<int16_t>(s * 30000);
	}

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(samples.size() * sizeof(int16_t));
	buffer.pAudioData = reinterpret_cast<const BYTE*>(samples.data());
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	source->SubmitSourceBuffer(&buffer);
	source->Start(0);
}