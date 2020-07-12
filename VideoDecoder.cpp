
#include "VideoDecoder.h"
#include <cmath>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <stdint.h>

struct VideoDecoder::Private {
	AVFormatContext *format_context = nullptr;
	AVStream *video_stream = nullptr;
	AVCodec *video_codec = nullptr;
	AVCodecContext *codec_context = nullptr;
	SwsContext *sws_context = nullptr;

	int dest_width = 1920;
	int dest_height = 1080;

	int timer_id = 0;
	QDateTime last_time;
	double elapsed_time = 0;

	VideoPosition position;
	VideoStatus status = VideoStatus::Stop;
	bool playing = false;
};

VideoDecoder::VideoDecoder()
	: m(new Private)
{
//	av_register_all();
}

VideoDecoder::~VideoDecoder()
{
	stop();
}

VideoPosition const *VideoDecoder::position() const
{
	return &m->position;
}

void VideoDecoder::open(QString const &filename)
{
	try {
		stop();

		int ret;

		ret = avformat_open_input(&m->format_context, filename.toStdString().c_str(), nullptr, nullptr);
		if (ret < 0) throw "avformat_open_input";

		av_format_inject_global_side_data(m->format_context);

		// av_dump_format(m_formatContext, 0, nullptr, 0);

		ret = avformat_find_stream_info(m->format_context, nullptr);
		if (ret < 0) throw "avformat_find_stream_info";

		for (unsigned int i = 0; i < m->format_context->nb_streams; i++) {
			if (m->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				m->video_stream = m->format_context->streams[i];
				break;
			}
		}
		if (!m->video_stream) throw "video_stream is null";

		m->video_codec = avcodec_find_decoder(m->video_stream->codecpar->codec_id);
		if (!m->video_codec) throw "avcodec_find_decoder() returns null";

		m->codec_context = avcodec_alloc_context3(m->video_codec);
		if (!m->codec_context) throw "avcodec_alloc_context3 returns null";

		ret = avcodec_parameters_to_context(m->codec_context, m->video_stream->codecpar);
		if (ret < 0) throw "avcodec_parameters_to_context";

		AVDictionary *opts = nullptr;
		av_dict_set(&opts, "refcounted_frames", "1", 0);

		ret = avcodec_open2(m->codec_context, m->video_codec, &opts);
		if (ret < 0) throw "avcodec_open2";

		m->sws_context = sws_getContext(m->codec_context->width, m->codec_context->height, m->codec_context->pix_fmt,
										m->dest_width, m->dest_height, AV_PIX_FMT_RGB24,
										SWS_BICUBIC, nullptr, nullptr, nullptr);

		if (!m->sws_context) throw "sws_getContext returns null";

		m->position = VideoPosition();
		m->position.duration = m->video_stream->duration;
		m->position.time_base_num = m->video_stream->time_base.num;
		m->position.time_base_den = m->video_stream->time_base.den;

		m->playing = true;
		m->status = VideoStatus::Play;

	} catch (char const *s) {
		qDebug() << QString("Failed to open the file: %1; %2").arg(filename).arg(QString(s));
	}
}

void VideoDecoder::stopTimer()
{
	m->playing = false;
	m->status = VideoStatus::Stop;
	killTimer(m->timer_id);
	m->timer_id = 0;
}

bool VideoDecoder::start()
{
	m->timer_id = startTimer(3); // フレームレートにかかわらず、インターバルタイマの速度は固定
	return true;
}

void VideoDecoder::stop()
{
	stopTimer();
	m->last_time = QDateTime();
	m->elapsed_time = 0;

	sws_freeContext(m->sws_context);
	avcodec_free_context(&m->codec_context);
	avformat_close_input(&m->format_context);

	m->format_context = nullptr;
	m->video_stream = nullptr;
	m->video_codec = nullptr;
	m->codec_context = nullptr;
	m->sws_context = nullptr;
}

void VideoDecoder::seek(unsigned int msec, bool backward)
{
	unsigned int pos = m->position.timebase_from_msec(msec);
	int flags = 0;
	if (backward) flags |= AVSEEK_FLAG_BACKWARD;
	av_seek_frame(m->format_context, m->video_stream->index, pos, flags);

	if (m->status == VideoStatus::EndOfFile) {
		m->status = VideoStatus::Play;
	}
}

void VideoDecoder::doEndOfFile()
{
	// ビデオの終端に達した
	m->status = VideoStatus::EndOfFile;
	m->position.position = m->position.duration; // 現在位置を終了位置に合わせる
//	emit endOfFile(); // 再生情報表示を更新する
}

FramePtr VideoDecoder::nextFrame()
{
	FramePtr frame;
	if (m->status == VideoStatus::Play || m->status == VideoStatus::PlayOne) {
		// thru
	} else {
		return frame;
	}

	AVFrame *av_frame = av_frame_alloc();
	AVPacket *av_pkt = av_packet_alloc();

	while (1) {
		int ret, pret;
		pret = av_read_frame(m->format_context, av_pkt);
		if (pret >= 0) {
			if (av_pkt->stream_index == m->video_stream->index) {
				ret = avcodec_send_packet(m->codec_context, av_pkt);
				if (ret == AVERROR(EAGAIN)) {
					qDebug() << "EAGAIN";
					goto done;
				}
				if (ret < 0) {
					qDebug() << "E: avcodec_send_packet";
					goto done;
				}
			}
		}
		av_packet_unref(av_pkt);
		ret = AVERROR(EAGAIN);
		do {
			ret = avcodec_receive_frame(m->codec_context, av_frame);
			if (ret == AVERROR_EOF) {
				doEndOfFile();
				goto done;
			}
			if (ret >= 0) {
				m->position.position = av_frame->pts; // 現在位置
				// 画像を取得
				QImage image(m->dest_width, m->dest_height, QImage::Format_RGB888);
				uint8_t *data[] = { image.bits() };
				int linesize[] = { image.bytesPerLine() };

				sws_scale(m->sws_context, av_frame->data, av_frame->linesize, 0, av_frame->height, data, linesize);

				frame = std::make_shared<Frame>();
				frame->setOriginalImage(image);

				av_frame_unref(av_frame);

				if (m->status == VideoStatus::PlayOne) {
					m->status = VideoStatus::Pause;
				}
				goto done;
			}
		} while (ret != AVERROR(EAGAIN));
		if (pret == AVERROR_EOF) {
			doEndOfFile();
			goto done;
		}
	}
done:;
	av_frame_free(&av_frame);
	av_packet_free(&av_pkt);
	return frame;
}

void VideoDecoder::timerEvent(QTimerEvent *)
{
	// およそ3ms間隔でここに来る（リアルタイムの精度は気にしない）
	if (m->format_context && m->codec_context && m->video_stream) {
		{ //
			bool skip = false;
			QDateTime now = QDateTime::currentDateTime(); // 現在の時間
			if (m->last_time.isValid()) {
				m->elapsed_time += m->last_time.msecsTo(now); // 前回からの経過時間を足す
				AVRational framerate = m->video_stream->avg_frame_rate;
				double ms = 1000.0 * framerate.den / framerate.num; // フレームレートに基づく間隔
				if (m->elapsed_time < ms) {
					skip = true; // デコードのタイミングに達していなければスキップ
				} else {
					m->elapsed_time -= ms; // デコードするので、経過時間から引く
				}
			}
			m->last_time = now; // 現在の時間を更新
			if (skip) return; // デコードしない
		}

		FramePtr frame = nextFrame();
		if (frame) {
			emit receiveFrame(frame);
		}
	}
}

void VideoDecoder::pauseAtNext()
{
	m->status = VideoStatus::PlayOne;
}

void VideoDecoder::pause(bool f)
{
	if (f) {
		m->status = VideoStatus::Pause;
	} else {
		m->status = m->playing ? VideoStatus::Play : VideoStatus::Stop;
	}
}

bool VideoDecoder::isPaused() const
{
	return m->status == VideoStatus::Pause || m->status == VideoStatus::PlayOne || m->status == VideoStatus::EndOfFile;
}

