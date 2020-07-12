#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QImage>
#include <QMetaType>
#include <QObject>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class Frame {
public:
	QImage image;
	void setOriginalImage(QImage const &image)
	{
		this->image = image;
	}
};
using FramePtr = std::shared_ptr<Frame>;
Q_DECLARE_METATYPE(FramePtr)

enum class VideoStatus {
	Stop,
	Pause,
	Play,
	PlayOne, // 1フレームだけ再生してPauseに遷移する
	EndOfFile,
};

struct VideoPosition {
	unsigned int time_base_num = 0;
	unsigned int time_base_den = 0;
	unsigned int duration = 0;
	unsigned int position = 0;

	unsigned int duration_in_msec() const
	{
		if (time_base_den == 0) return 0;
		return (unsigned int)(1000ULL * duration * time_base_num / time_base_den);
	}
	unsigned int position_in_msec() const
	{
		if (time_base_den == 0) return 0;
		return (unsigned int)(1000ULL * position * time_base_num / time_base_den);
	}
	unsigned int timebase_from_msec(unsigned int msec) const
	{
		if (time_base_num == 0) return 0;
		return (unsigned int)((unsigned long long)msec * time_base_den / time_base_num / 1000);
	}
};

class VideoDecoder : public QObject {
	Q_OBJECT
private:
	struct Private;
	Private *m;
	void stopTimer();
	FramePtr nextFrame();
	void doEndOfFile();
public:
	VideoDecoder();
	~VideoDecoder();

	void open(QString const &filename);
	bool start();
	void stop();

	void seek(unsigned int msec, bool backward);

	void pauseAtNext();
	void pause(bool f);
	bool isPaused() const;

	const VideoPosition *position() const;
protected:
	void timerEvent(QTimerEvent *);
signals:
	void receiveFrame(FramePtr frame);
};


#endif // VIDEODECODER_H
