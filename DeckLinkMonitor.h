#ifndef DECKLINKMONITOR_H
#define DECKLINKMONITOR_H

//#include <thread>
//#include <mutex>
#include <condition_variable>
#include "DeckLinkAPI.h"

class DeckLinkMonitor {
private:
	bool interrupted_ = false;
	bool running = false;
public:

	IDeckLink *decklink_ = nullptr;
	IDeckLinkMutableVideoFrame *playbackFrame = nullptr;
	IDeckLinkOutput *selectedDeckLinkOutput = nullptr;
	IDeckLinkIterator *deckLinkIterator = nullptr;
	BMDTimeValue frameDuration = 1001;
	BMDTimeValue frameTimescale = 30000;
	bool loopPlayback = false;
	int exitStatus = 1;

	struct DisplayMode {
		IDeckLinkDisplayMode *mode;
		std::string name;
		DisplayMode() = default;
		DisplayMode(IDeckLinkDisplayMode *mode, std::string const &name)
			: mode(mode)
			, name(name)
		{
		}
	};

	std::vector<DisplayMode> dispmodes;

public:
	DeckLinkMonitor() = default;
	~DeckLinkMonitor();

	bool start();
	void stop();

	void putFrame(const QImage image);
};

#endif // DECKLINKMONITOR_H
