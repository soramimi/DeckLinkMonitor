#include "ImageUtil.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(&video_decoder_, &VideoDecoder::receiveFrame, this, &MainWindow::receiveFrame);
	video_decoder_.open("/home/soramimi/a.avi");
	video_decoder_.start();
}

MainWindow::~MainWindow()
{
	video_decoder_.stop();
	delete ui;
}

void MainWindow::receiveFrame(FramePtr frame)
{
	putFrame(frame->image);
}

void MainWindow::putFrame(Image const &image)
{
	QImage img = ImageUtil::qimage(image).convertToFormat(QImage::Format_RGB32);
	dlm.putFrame(img);

}
