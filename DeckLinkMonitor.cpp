#include "DeckLinkMonitor.h"

#include <stdio.h>
#include <QImage>
#include <QPainter>
#include <QDebug>


void DeckLinkMonitor::putFrame(QImage const image)
{
	int frame_w = playbackFrame->GetWidth();
	int frame_h = playbackFrame->GetHeight();
	uint8_t *bits = nullptr;
	if (playbackFrame->GetBytes((void**)&bits) == S_OK && bits) {
		int stride = playbackFrame->GetRowBytes();
		for (int y = 0; y < frame_h; y++) {
			uint8_t *dst = bits + stride * y;
			uint8_t const *src = image.scanLine(y);
			memcpy(dst, src, frame_w * 4);
		}
	}
	{
		HRESULT result = selectedDeckLinkOutput->DisplayVideoFrameSync(playbackFrame);
		if (result != S_OK) {
			qDebug() << "Unable to display video output\n";
			return;
		}
	}
}

DeckLinkMonitor::~DeckLinkMonitor()
{
	stop();
}

bool DeckLinkMonitor::start()
{
	exitStatus = 1;

	// Configuration flags
	bool ok = true;

	int deckLinkIndex = -1;
	int displayModeIndex = 8;

	BMDDisplayMode selectedDisplayMode = bmdModeNTSC;
	std::string selectedDisplayModeName;

	std::vector<std::string> deckLinkDeviceNames;

	deckLinkIterator = nullptr;
	{
		HRESULT result = GetDeckLinkIterator(&deckLinkIterator);
		if (result != S_OK) return false;
	}

	// Obtain the required DeckLink device

	selectedDeckLinkOutput = nullptr;
	{
		int index = 0;
		while (1) {
			IDeckLink *decklink = nullptr;
			HRESULT r = deckLinkIterator->Next(&decklink);
			if (r != S_OK) break;

			QString devicename;

			{
				DLString name;
				r = decklink->GetDisplayName(&name);
				if (r == S_OK) {
					devicename = name;
					deckLinkDeviceNames.push_back(name);
				}
			}

			int64_t ioSupportAttribute = 0;
			{
				IDeckLinkProfileAttributes *atts = nullptr;
				r = decklink->QueryInterface(IID_IDeckLinkProfileAttributes, (void **)&atts);
				if (r != S_OK) {
					fprintf(stderr, "Unable to get IDeckLinkAttributes interface\n");
					return false;
				}

				if (atts->GetInt(BMDDeckLinkVideoIOSupport, &ioSupportAttribute) != S_OK) {
					ioSupportAttribute = 0;
				}

				atts->Release();
			}

			if (ioSupportAttribute & bmdDeviceSupportsPlayback) {
				deckLinkIndex = index;
				r = decklink->QueryInterface(IID_IDeckLinkOutput, (void **)&selectedDeckLinkOutput);
				if (r != S_OK) {
					fprintf(stderr, "Unable to get IDeckLinkOutput interface\n");
					return false;
				}
				decklink_ = decklink;
				break;
			}
			index++;

			decklink->Release();
		}
	}

	// Get display modes from the selected decklink output
	if (selectedDeckLinkOutput) {
		{
			IDeckLinkDisplayModeIterator *displayModeIterator = nullptr;
			HRESULT result = selectedDeckLinkOutput->GetDisplayModeIterator(&displayModeIterator);
			if (result != S_OK) {
				fprintf(stderr, "Unable to get IDeckLinkDisplayModeIterator interface\n");
				return false;
			}
			{
				IDeckLinkDisplayMode *displayMode = nullptr;
				while (displayModeIterator->Next(&displayMode) == S_OK) {
					DLString displayModeName;
					HRESULT result = displayMode->GetName(&displayModeName);
					std::string name;
					if (result == S_OK) {
						name = displayModeName;
					}
					if (displayMode->GetDisplayMode() == bmdModeHD1080i5994) {
						displayModeIndex = dispmodes.size();
					}
					dispmodes.emplace_back(displayMode, name);
				}
			}
			displayModeIterator->Release();
		}

		if (displayModeIndex < 0 || displayModeIndex > (int)dispmodes.size()) {
			fprintf(stderr, "You must select a valid display mode\n");
			ok = false;
		} else {
			selectedDisplayMode = dispmodes[displayModeIndex].mode->GetDisplayMode();
			selectedDisplayModeName = dispmodes[displayModeIndex].name;

			{
				HRESULT result = dispmodes[displayModeIndex].mode->GetFrameRate(&frameDuration, &frameTimescale);
				if (result != S_OK) return false;
			}

			// Check display mode is supported with given options
			// Passing pixel format = 0 to represent any pixel format
			BOOL dispmodesupported;
			HRESULT result = selectedDeckLinkOutput->DoesSupportVideoMode(bmdVideoConnectionUnspecified, selectedDisplayMode, bmdFormatUnspecified, bmdSupportedVideoModeDefault, nullptr, &dispmodesupported);
			if (result != S_OK || !dispmodesupported) {
				fprintf(stderr, "The display mode %s is not supported by device\n", selectedDisplayModeName.c_str());
				ok = false;
			}
		}

	}

	if (!ok) {
		return false;
	}

	// Set the video output mode
	{
		HRESULT result = selectedDeckLinkOutput->EnableVideoOutput(selectedDisplayMode, bmdVideoOutputFlagDefault);
		if (result != S_OK) {
			fprintf(stderr, "Unable to enable video output\n");
			return false;
		}
	}

	// Create video frame for playback, as we are outputting frame synchronously,
	// then we can reuse without waiting on callback
	{
		IDeckLinkDisplayMode *mode = dispmodes[displayModeIndex].mode;
		HRESULT result = selectedDeckLinkOutput->CreateVideoFrame((int32_t)mode->GetWidth(),
																  (int32_t)mode->GetHeight(),
																  (int32_t)mode->GetWidth() * 4,
																  bmdFormat8BitBGRA,
																  bmdFrameFlagDefault,
																  &playbackFrame);
		if (result != S_OK) {
			fprintf(stderr, "Unable to create video frame\n");
			return false;
		}
	}

	// OK to start playback - print configuration
	fprintf(stderr, "Output with the following configuration:\n"
					" - Playback device: %s\n"
					" - Video mode: %s\n"
			,
			deckLinkDeviceNames[deckLinkIndex].c_str(),
			selectedDisplayModeName.c_str()
			);

	exitStatus = 0;
	return true;
}

void DeckLinkMonitor::stop()
{
	if (running && !interrupted_) {
		interrupted_ = true;
	}

	for (DisplayMode &dm : dispmodes) {
		dm.mode->Release();
	}
	dispmodes.clear();

	if (playbackFrame) {
		playbackFrame->Release();
		playbackFrame = nullptr;
	}

	if (selectedDeckLinkOutput) {
		selectedDeckLinkOutput->DisableVideoOutput();
		selectedDeckLinkOutput->Release();
		selectedDeckLinkOutput = nullptr;
	}

	if (decklink_) {
		decklink_->Release();
		decklink_ = nullptr;
	}

	if (deckLinkIterator) {
		deckLinkIterator->Release();
		deckLinkIterator = nullptr;
	}
}
