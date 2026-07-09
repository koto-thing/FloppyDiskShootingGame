#pragma once

class AudioService {
public:
	AudioService();
	~AudioService();

	bool Initialize();
	void Shutdown();
	void Play440HzSound();

private:
	struct Impl;
	Impl* impl;
};