#include "WaveEdit.hpp"
#include <fstream>

#include <iostream> // TODO: remove

// I am endebted to Thomas Arashikage, who graciously and generously provided
// his Perl source code for parsing and writing EFE files, without which I
// wouldn't have been able to add this feature. Thanks Thomas!


#define EFE_HEADERFILE "header.eps"
#define EFE_GIEBLERSIZE 512
#define EFE_INSTDATASIZE 656
#define EFE_WS_HEADERSIZE 288
#define EFE_EPS_BLOCKSIZE 512
#define EFE_LAYERSIZE 224
#define EFE_WS_INFO_OFFSET 238

static std::vector<char> efeBuffer;


void efeInit() {
	std::ifstream file(EFE_HEADERFILE, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	efeBuffer.resize(size);
	if (file.read(efeBuffer.data(), size)) {
			return;
	}
}


std::vector<char> wavToEfe(const char *wavFilename) { // filename to temporary, conformed WAV (could be internal stream?)
	// this assumes 16-bit, mono, 44.1kHz WAV data, big-endian byte order
	std::ifstream file(wavFilename, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> efe; // copy the template

	std::vector<char> wavBuffer(size);
	if (!file.read(wavBuffer.data(), size)) {
		std::cout << "could not read wav file " << wavFilename << ": " << size << " bytes" << std::endl;
		return efe;
	}

	std::string wsName(wavFilename);
	size_t aPos =
#if defined(_WIN32)
		wsName.find_last_of("\\");
#else
		wsName.find_last_of("/");
#endif
	if (aPos != std::string::npos) {
		wsName.erase(0, aPos + 1);
	}
	aPos = wsName.rfind("_tmp.wav");
	if (aPos != std::string::npos) wsName.erase(aPos);

	aPos = wsName.find_last_of(".");
	if (aPos != std::string::npos) wsName.erase(aPos);

	aPos = wsName.find_first_not_of("\t\n\v\f\r ");
	if (aPos != std::string::npos) wsName.erase(0, aPos);

	aPos = wsName.find_last_not_of("\t\n\v\f\r ");
	if (aPos != std::string::npos) wsName.erase(aPos + 1);

	if (wsName.size() > 12) {
		wsName.erase(12);
	}

	if (wsName.size() < 12) {
		wsName.append(12 - wsName.size(), ' ');
	}

	for (int i = 0; i < (int)wsName.size(); i++) {
		if (!isprint(wsName[i])) wsName[i] = '_';
		else wsName[i] = toupper(wsName[i]);
	}

	int rawByteSize = size - 44; // subtract header bytes

	// no need to handle truncation, maximum size will fit on an EPS, as well

	efe = efeBuffer;

	// wavesample

	int wsNum = 1;
	int wsOffsetPosition = EFE_GIEBLERSIZE + 0x0084 + (wsNum << 2);
	int wsOffsetPointer = (efe[wsOffsetPosition + 3] << 16)
												| (efe[wsOffsetPosition + 0] << 12)
												| (efe[wsOffsetPosition + 2] << 4); // yes, byte 1 is ignored
	int wsChunks = (int)std::ceil((float)rawByteSize / 16);
	int wsObjectSize = wsChunks * 16 + EFE_WS_HEADERSIZE;
	int wsObjectBlkSize = (int)std::ceil((float)wsObjectSize / EFE_EPS_BLOCKSIZE);
	int wsStart = 0;
	int wsEnd = rawByteSize;
	int wsLoopStart = 0;
	int wsLoopEnd = WAVE_LEN * 2; // value in bytes, not in samples

	int wsBlockSizeA = ((wsObjectSize << 4) & 0x0000FF00) >> 8;
	int wsBlockSizeB = ((wsObjectSize << 4) & 0x000000F0);
	int wsBlockSizeC = ((wsObjectSize << 8) & 0xFF000000) >> 24;
	int wsBlockSizeD = ((wsObjectSize << 8) & 0x00F00000) >> 16;

	int currentPosition = EFE_GIEBLERSIZE;
	currentPosition += wsOffsetPointer; // wave sample info

	efe[currentPosition++] = wsBlockSizeA;
	efe[currentPosition++] = wsBlockSizeB;
	efe[currentPosition++] = wsBlockSizeC;
	efe[currentPosition++] = wsBlockSizeD;
	currentPosition += 2;

	int wsFlink = 0;
	efe[currentPosition++] = wsFlink;
	efe[currentPosition++] = wsFlink;

	int wsBlink = 0;
	efe[currentPosition++] = wsBlink;
	efe[currentPosition++] = wsBlink;

	// name
	for (int i = 0; i < 12; i++) {
		efe[currentPosition++] = wsName[i];
		efe[currentPosition++] = 0;
	}

	currentPosition = EFE_GIEBLERSIZE + EFE_WS_INFO_OFFSET + wsOffsetPointer;

	int wsLoopFlags = 2; // loop forward
	efe[currentPosition++] = wsLoopFlags;
	efe[currentPosition++] = 0;

	int wsStartA = (wsStart & 0xFF0000) >> 16;
	int wsStartB = (wsStart & 0x00FF00) >> 8;
	int wsStartC = (wsStart & 0x0000FF);
	efe[currentPosition++] = wsStartA;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsStartB;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsStartC;
	efe[currentPosition++] = 0;
	currentPosition += 2;

	int wsEndA = (wsEnd & 0xFF0000) >> 16;
	int wsEndB = (wsEnd & 0x00FF00) >> 8;
	int wsEndC = (wsEnd & 0x0000FF);
	efe[currentPosition++] = wsEndA;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsEndB;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsEndC;
	efe[currentPosition++] = 0;
	currentPosition += 2;

	int wsLoopStartA = (wsLoopStart & 0xFF0000) >> 16;
	int wsLoopStartB = (wsLoopStart & 0x00FF00) >> 8;
	int wsLoopStartC = (wsLoopStart & 0x0000FF);
	efe[currentPosition++] = wsLoopStartA;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopStartB;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopStartC;
	efe[currentPosition++] = 0;
	currentPosition += 2;

	int wsLoopEndA = (wsLoopEnd & 0xFF0000) >> 16;
	int wsLoopEndB = (wsLoopEnd & 0x00FF00) >> 8;
	int wsLoopEndC = (wsLoopEnd & 0x0000FF);
	efe[currentPosition++] = wsLoopEndA;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopEndB;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopEndC;
	efe[currentPosition++] = 0;
	currentPosition += 2;

	int wsSampleRate = 1; // 44.1kHz
	efe[currentPosition++] = wsSampleRate;
	efe[currentPosition++] = 0;

	int wsLowKey = 0x24;
	int wsHighKey = 0x60;
	int wsLoopModSrc = 10; // WHEEL
	int wsPDelayModAmt = 0; // TODO
	int wsLoopModAmt = (int)std::floor(((float)BANK_LEN / BANK_LEN_MAX) * 127.);
	int wsLoopModAmt2 = 1; // 1 MG
	int wsModType = 7; // TRANSWAV
	int wsPDelay = 0; // TODO
	efe[currentPosition++] = wsLowKey;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsHighKey;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopModSrc;
	efe[currentPosition++] = wsPDelayModAmt;
	efe[currentPosition++] = wsLoopModAmt;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsLoopModAmt2;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsModType;
	efe[currentPosition++] = 0;
	efe[currentPosition++] = wsPDelay;

	// layer

	int layerTotal = 1;
	int instBlockSize = EFE_INSTDATASIZE + (EFE_LAYERSIZE * layerTotal) + wsObjectSize;
	int instBlockAmt = (int)std::ceil((float)instBlockSize / EFE_EPS_BLOCKSIZE);
	instBlockSize = instBlockAmt * EFE_EPS_BLOCKSIZE;

	int instBlockSizeA = ((instBlockSize << 4) & 0x0000FF00) >> 8;
	int instBlockSizeB = ((instBlockSize << 4) & 0x000000F0);
	int instBlockSizeC = ((instBlockSize << 8) & 0xFF000000) >> 24;
	int instBlockSizeD = ((instBlockSize << 8) & 0x00F00000) >> 16;

	currentPosition = EFE_GIEBLERSIZE;
	efe[currentPosition++] = instBlockSizeA;
	efe[currentPosition++] = instBlockSizeB;
	efe[currentPosition++] = instBlockSizeC;
	efe[currentPosition++] = instBlockSizeD;

	currentPosition = EFE_GIEBLERSIZE + 10;
	for (int i = 0; i < 12; i++) {
		efe[currentPosition++] = wsName[i];
		efe[currentPosition++] = 0;
	}

	currentPosition = EFE_GIEBLERSIZE + 40;
	int instBlockAmtA = (instBlockAmt & 0xFF00) >> 8;
	int instBlockAmtB = (instBlockAmt & 0x00FF);
	efe[currentPosition++] = instBlockAmtA;
	efe[currentPosition++] = instBlockAmtB;

	currentPosition = EFE_GIEBLERSIZE + 60;
	efe[currentPosition++] = 0; // call it an EPS for now; EPS16 = 0xFFFF, ASR = 0xFFFE
	efe[currentPosition++] = 0;

	currentPosition = 18;
	for (int i = 0; i < 12; i++) {
		efe[currentPosition++] = wsName[i];
	}

	currentPosition = 52;
	efe[currentPosition++] = instBlockAmtA;
	efe[currentPosition++] = instBlockAmtB;
	efe[currentPosition++] = instBlockAmtA;
	efe[currentPosition++] = instBlockAmtB;

	// header is done!

	efe.insert(efe.end(), wavBuffer.begin() + 44, wavBuffer.end());

	int wavPaddingBytes = wavBuffer.size() % 16;
	for (int i = 0; i < wavPaddingBytes; i++) {
		efe.push_back(0);
	}

	int paddingBytes = efe.size() % 512;
	if (paddingBytes) {
		int paddingChunks = (512 - paddingBytes) / 16;
		// first chunk
		efe.push_back(paddingChunks);
		efe.push_back(0x10);
		efe.push_back(0x00);
		efe.push_back(0x00);
		efe.push_back(0x08);
		efe.push_back(0x40);
		for (int i = 0; i < 10; i++) {
			efe.push_back(0);
		}
		//
		for (int i = 1; i < paddingChunks; i++) {
			for (int ii = 0; ii < 16; ii++) {
				efe.push_back(0);
			}
		}
	}

	return efe;
}
