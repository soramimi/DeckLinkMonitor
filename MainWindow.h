#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DeckLinkMonitor.h"
#include "VideoDecoder.h"

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	VideoDecoder video_decoder_;
	Ui::MainWindow *ui;
public:
	DeckLinkMonitor dlm;
private slots:
	void receiveFrame(FramePtr frame);
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:
	void putFrame(const Image &image);
};

#endif // MAINWINDOW_H
