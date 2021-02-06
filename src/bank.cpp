#include "WaveEdit.hpp"
#include <string.h>
#include <sndfile.h>


int userBankLen = BANK_LEN_MAX;
int bankGridWidth = 8;
int bankGridHeight = 8;

void Bank::setBankLen(int newLen) {
	// TODO: save samples and restore?
	// userBankLen = BANK_LEN_MAX;
	// clear();
	userBankLen = (newLen > BANK_LEN_MAX) ? BANK_LEN_MAX : (newLen < 1) ? 1 : newLen;
	bankGridHeight = (userBankLen / 8);
	if (userBankLen % 8) bankGridHeight++;
}


int Bank::getBankLen() {
	return BANK_LEN;
}


void Bank::clear() {
	// The lazy way
	memset(this, 0, sizeof(Bank));

	for (int i = 0; i < BANK_LEN; i++) {
		waves[i].commitSamples();
	}
}


void Bank::swap(int i, int j) {
	Wave tmp = waves[i];
	waves[i] = waves[j];
	waves[j] = tmp;
}


void Bank::shuffle() {
	for (int j = BANK_LEN - 1; j >= 3; j--) {
		int i = rand() % j;
		swap(i, j);
	}
}


void Bank::setSamples(const float *in) {
	for (int j = 0; j < BANK_LEN; j++) {
		memcpy(waves[j].samples, &in[j * WAVE_LEN], sizeof(float) * WAVE_LEN);
		waves[j].commitSamples();
	}
}


void Bank::getPostSamples(float *out) {
	for (int j = 0; j < BANK_LEN; j++) {
		memcpy(&out[j * WAVE_LEN], waves[j].postSamples, sizeof(float) * WAVE_LEN);
	}
}


void Bank::duplicateToAll(int waveId) {
	for (int j = 0; j < BANK_LEN; j++) {
		if (j != waveId)
			waves[j] = waves[waveId];
		// No need to commit the wave because we're copying everything
	}
}


void Bank::save(const char *filename) {
	FILE *f = fopen(filename, "wb");
	if (!f)
		return;
	fwrite(this, sizeof(*this), 1, f);
	fclose(f);
}


void Bank::load(const char *filename) {
	clear();

	FILE *f = fopen(filename, "rb");
	if (!f)
		return;
	fread(this, sizeof(*this), 1, f);
	fclose(f);

	for (int j = 0; j < BANK_LEN; j++) {
		waves[j].commitSamples();
	}
}


void Bank::saveWAV(const char *filename) {
	SF_INFO info;
	info.samplerate = 44100;
	info.channels = 1;
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
	SNDFILE *sf = sf_open(filename, SFM_WRITE, &info);
	if (!sf)
		return;

	for (int j = 0; j < BANK_LEN; j++) {
		sf_write_float(sf, waves[j].postSamples, WAVE_LEN);
	}
	sf_write_float(sf, waves[0].postSamples, 1); // + 1 sample from the start

	sf_close(sf);
}


void Bank::saveEFE(const char *filename) {
	SF_INFO info;
	info.samplerate = 44100;
	info.channels = 1;
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
	SNDFILE *sf = sf_open(filename, SFM_WRITE, &info);
	if (!sf)
		return;

	for (int j = 0; j < BANK_LEN; j++) {
		sf_write_float(sf, waves[j].postSamples, WAVE_LEN);
	}
	sf_write_float(sf, waves[0].postSamples, 1); // + 1 sample from the start

	sf_close(sf);

	// now convert to EFE using ...
}


void Bank::loadWAV(const char *filename) {
	clear();

	SF_INFO info;
	SNDFILE *sf = sf_open(filename, SFM_READ, &info);
	if (!sf)
		return;

	for (int i = 0; i < BANK_LEN; i++) {
		sf_read_float(sf, waves[i].samples, WAVE_LEN);
		waves[i].commitSamples();
	}

	sf_close(sf);
}


void Bank::saveWaves(const char *dirname) {
	for (int b = 0; b < BANK_LEN; b++) {
		char filename[1024];
		snprintf(filename, sizeof(filename), "%s/%02d.wav", dirname, b);

		waves[b].saveWAV(filename);
	}
}
