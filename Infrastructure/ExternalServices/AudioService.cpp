#include "AudioService.h"

#include <windows.h>
#include <xaudio2.h>

#include <cmath>
#include <cstdint>
#include <vector>

#pragma comment(lib, "xaudio2.lib")

struct AudioService::Impl {
	IXAudio2* xAudio2;
	IXAudio2MasteringVoice* masteringVoice;
	IXAudio2SourceVoice* sourceVoice;
	std::vector<int16_t> buffer;
	WAVEFORMATEX waveFormat;
	Impl() : xAudio2(nullptr), masteringVoice(nullptr), sourceVoice(nullptr) {}
};

AudioService::AudioService() : impl(new Impl()) {}

AudioService::~AudioService() {
	Shutdown();
	delete impl;
}

bool AudioService::Initialize() {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	if (FAILED(XAudio2Create(&impl->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		return false;
	}
	if (FAILED(impl->xAudio2->CreateMasteringVoice(&impl->masteringVoice))) {
		return false;
	}

	return true;
}

void AudioService::Shutdown() {
	if (impl->sourceVoice) {
		impl->sourceVoice->DestroyVoice();
		impl->sourceVoice = nullptr;
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

void AudioService::Play440HzSound() {
	WAVEFORMATEX format = {};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = 44100;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	IXAudio2SourceVoice* source = nullptr;

	impl->xAudio2->CreateSourceVoice(&source, &format);

	constexpr int sampleRate = 44100;
	constexpr float frequency = 440.0f;
	constexpr float duration = 1.0f;

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

	source->Start();

	Sleep(1100);

	source->DestroyVoice();
}