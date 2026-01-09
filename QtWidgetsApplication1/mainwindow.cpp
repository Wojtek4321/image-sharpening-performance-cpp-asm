#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QImageReader>
#include <QThread>
#include <thread>
#include <chrono>
#include <numeric>
#include <QMessageBox>
#include <QApplication>

#include "sharpen_avx.h"  

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    QWidget* central = new QWidget(this); //widget okna
    setCentralWidget(central);

    // etykiety obrazow 
    labelBefore = new QLabel("Before");
    labelBefore->setFixedSize(400, 300);
    labelBefore->setAlignment(Qt::AlignCenter);
    labelBefore->setStyleSheet("background: #ddd; border: 1px solid #888;");
    labelAfter = new QLabel("After");
    labelAfter->setFixedSize(400, 300);
    labelAfter->setAlignment(Qt::AlignCenter);
    labelAfter->setStyleSheet("background: #ddd; border: 1px solid #888;");

    // kontrolki

	//suwak intensywnoœci
    sliderIntensity = new QSlider(Qt::Horizontal);
    sliderIntensity->setRange(0, 200);
    sliderIntensity->setValue(100);

    //liczba w¹tków
    spinThreads = new QSpinBox();
    spinThreads->setRange(1, 64);
    int defaultThreads = std::thread::hardware_concurrency();
    if (defaultThreads < 1) defaultThreads = 1;
    spinThreads->setValue(defaultThreads);

	//przxyciski
    btnLoad = new QPushButton("Load Image");
    btnApply = new QPushButton("Apply Sharpen (C++)");
    btnApplyASM = new QPushButton("Apply Sharpen (ASM)");
    btnBenchmark = new QPushButton("Benchmark (choose up to 3 files)");
    btnSave = new QPushButton("Save Result");

    //informacje sttusu
    labelInfo = new QLabel("Info: ready");

    // Layouty
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->addWidget(labelBefore);
    leftCol->addWidget(new QLabel("Intensity (%)"));
    leftCol->addWidget(sliderIntensity);

    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->addWidget(labelAfter);
    rightCol->addWidget(new QLabel("Threads"));
    rightCol->addWidget(spinThreads);

    QHBoxLayout* top = new QHBoxLayout();
    top->addLayout(leftCol);
    top->addLayout(rightCol);

    QHBoxLayout* controls = new QHBoxLayout();
    controls->addWidget(btnLoad);
    controls->addWidget(btnApply);
    controls->addWidget(btnApplyASM);
    controls->addWidget(btnBenchmark);
    controls->addWidget(btnSave);

    QVBoxLayout* mainLay = new QVBoxLayout(central);
    mainLay->addLayout(top);
    mainLay->addLayout(controls);
    mainLay->addWidget(labelInfo);

	// po³¹czenia sygna³ów 
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(btnApply, &QPushButton::clicked, this, &MainWindow::onApply);
    connect(btnApplyASM, &QPushButton::clicked, this, &MainWindow::onApplyASM);
    connect(btnBenchmark, &QPushButton::clicked, this, &MainWindow::onBenchmark);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::onSave);

    setWindowTitle("Image Sharpen");
    resize(850, 700);
}

//wczytanie obrazu
void MainWindow::onLoad()
{
    QString file = QFileDialog::getOpenFileName(this, "Open Image");
    if (file.isEmpty()) 
        return;
    QImageReader reader(file);
    QImage img = reader.read();
    if (img.isNull()) 
    {
        QMessageBox::warning(this, "Error", "Cannot load image");
        return;
    }
	//konwersja do formatu RGB32
    imgSrc = img.convertToFormat(QImage::Format_RGB32);
    imgResult = imgSrc;
    
    //wyœwietlaenie obrazów
    labelBefore->setPixmap(QPixmap::fromImage(imgSrc).scaled(labelBefore->size(), Qt::KeepAspectRatio));
    labelAfter->setPixmap(QPixmap::fromImage(imgResult).scaled(labelAfter->size(), Qt::KeepAspectRatio));
    labelInfo->setText(QString("Loaded: %1 x %2").arg(imgSrc.width()).arg(imgSrc.height()));
}

//ograniczenie wartoœci do zakresu 0-255
int MainWindow::clamp(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

//rozmycie pude³kowe 3x3
void MainWindow::boxBlur3x3_worker(const QImage& src, QImage& dst, int y0, int y1)
{
    int w = src.width();
    int h = src.height();

	//przetwarzanie tylko wyznaczonego zakresu wierszy
    for (int y = y0; y < y1; ++y) {
        for (int x = 0; x < w; ++x) {
            int r = 0, g = 0, b = 0;
            //maska 3x3
            for (int ky = -1; ky <= 1; ++ky) {
                int sy = std::min(std::max(y + ky, 0), h - 1);
                const QRgb* row = reinterpret_cast<const QRgb*>(src.constScanLine(sy));

                for (int kx = -1; kx <= 1; ++kx) {
                    int sx = std::min(std::max(x + kx, 0), w - 1);
                    QRgb p = row[sx];
                    r += qRed(p); g += qGreen(p); b += qBlue(p);
                }
            }

			//srednia wartoœæ
            r /= 9; g /= 9; b /= 9;
            reinterpret_cast<QRgb*>(dst.scanLine(y))[x] = qRgb(clamp(r), clamp(g), clamp(b));
        }
    }
}

//wyostrzanie obrazu c++
QImage MainWindow::sharpenImage(const QImage& src, double amount, int threads)
{
    //szerokoœæ i wysokoœæ obrazu
    int w = src.width(),h = src.height();
    
	//obrazy pomocnicze oraz wynikowy
    QImage blur(w, h, QImage::Format_RGB32);
    QImage dst(w, h, QImage::Format_RGB32);


    // box blur 3x3
    std::vector<std::thread> workers; 

    //podzial obrazu na pasy poziome
	int base = h / threads; //liczba wierszy na w¹tek
    int rem = h % threads; //reszta
    int y = 0;

    for (int t = 0; t < threads; ++t) {
		//ka¿dy w¹tek dostaje podobn¹ liczbê wierszy
        int chunk = base + (t < rem ? 1 : 0);
        int y0 = y, y1 = y + chunk;
        y = y1;
        //uruchomienie w¹tku rozmywaj¹cego fragment obrazu
        workers.emplace_back([this, &src, &blur, y0, y1]() { boxBlur3x3_worker(src, blur, y0, y1); });
    }

	//oczekiwanie na zakoñczenie wszystkich w¹tków
    for (auto& th : workers) th.join();

	//wyostrzanie obrazu
    // dst = src + amount*(src - blur)
    workers.clear(); y = 0;
    for (int t = 0; t < threads; ++t) {
        int chunk = base + (t < rem ? 1 : 0);
        int y0 = y, y1 = y + chunk;
        y = y1;
        workers.emplace_back([this, &src, &blur, &dst, amount, y0, y1]() {
            int w = src.width();

			//przetwarzanie wyznaczonego zakresu wierszy
            for (int yy = y0; yy < y1; ++yy) {
                const QRgb* srow = reinterpret_cast<const QRgb*>(src.constScanLine(yy));
                const QRgb* brow = reinterpret_cast<const QRgb*>(blur.constScanLine(yy));
                QRgb* drow = reinterpret_cast<QRgb*>(dst.scanLine(yy));

                for (int x = 0; x < w; ++x) {
                    //sk³adowe oryginalne
                    int sr = qRed(srow[x]), sg = qGreen(srow[x]), sb = qBlue(srow[x]);
					//sk³adowe rozmyte
                    int br = qRed(brow[x]), bg = qGreen(brow[x]), bb = qBlue(brow[x]);
					//wzór wyostrzania
                    int rr = clamp(static_cast<int>(sr + amount * (sr - br)));
                    int rg = clamp(static_cast<int>(sg + amount * (sg - bg)));
                    int rb = clamp(static_cast<int>(sb + amount * (sb - bb)));
                    drow[x] = qRgb(rr, rg, rb);
                }
            }
            });
    }
	//synchronizacja w¹tków
    for (auto& th : workers) th.join();
    return dst;
}

//wyostrzanie obrazu ASM
QImage MainWindow::sharpenImageASM(const QImage& src, double amount, int threads)
{
    int w = src.width(), h = src.height();
	int pixels = w * h; //ca³kowita liczba pikseli

    QImage blur(w, h, QImage::Format_RGB32);
    QImage dst(w, h, QImage::Format_RGB32);


	// BOX BLUR 3x3 wielow¹tkowo
    std::vector<std::thread> workers;
    int base = h / threads;
    int rem = h % threads;
    int y = 0;

    workers.reserve(threads);

    for (int t = 0; t < threads; ++t) {
        int chunk = base + (t < rem ? 1 : 0);
        int y0 = y, y1 = y + chunk;
        y = y1;
        workers.emplace_back([=, &src, &blur]() {
            boxBlur3x3_worker(src, blur, y0, y1);
            });
    }
    for (auto& th : workers) th.join();


    // wostrzanie AVX 
    // obraz Ÿród³owy
    // obraz rozmyty
    //obraz wynikowy 
    //liczba pikseli
	//intensywnoœæ wyostrzania
    sharpen_avx(
		(unsigned char*)src.bits(), 
		(unsigned char*)blur.bits(), 
		(unsigned char*)dst.bits(), 
        pixels, 
        amount 
    );

    return dst;
}

//obsluga przycisku C++
void MainWindow::onApply()
{
    if (imgSrc.isNull()) { QMessageBox::information(this, "Info", "Load an image first"); return; }
    double amount = sliderIntensity->value() / 100.0;
    int threads = spinThreads->value();
    labelInfo->setText("Processing C++...");
    QApplication::processEvents();
    auto t0 = std::chrono::high_resolution_clock::now();
    imgResult = sharpenImage(imgSrc, amount, threads);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    labelAfter->setPixmap(QPixmap::fromImage(imgResult).scaled(labelAfter->size(), Qt::KeepAspectRatio));
    labelInfo->setText(QString("C++ Done - time: %1 ms (threads: %2)").arg(ms, 0, 'f', 2).arg(threads));
}

//Obs³uga przycisku ASM
void MainWindow::onApplyASM()
{
    if (imgSrc.isNull()) { QMessageBox::information(this, "Info", "Load an image first"); return; }
    double amount = sliderIntensity->value() / 100.0;
    int threads = spinThreads->value();
    labelInfo->setText("Processing ASM...");
    QApplication::processEvents();
    auto t0 = std::chrono::high_resolution_clock::now();
    imgResult = sharpenImageASM(imgSrc, amount, threads);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    labelAfter->setPixmap(QPixmap::fromImage(imgResult).scaled(labelAfter->size(), Qt::KeepAspectRatio));
    labelInfo->setText(QString("ASM Done - time: %1 ms (threads: %2)").arg(ms, 0, 'f', 2).arg(threads));
    
}
//obs³uga przycisku zapisu
void MainWindow::onSave()
{
    if (imgResult.isNull()) { QMessageBox::information(this, "Info", "No result to save"); return; }
    QString file = QFileDialog::getSaveFileName(this, "Save Result", "", "PNG Files (*.png);;JPEG Files (*.jpg *.jpeg)");
    if (file.isEmpty()) return;
    imgResult.save(file);
}
//obs³uga przycisku testu wydajnoœci
void MainWindow::onBenchmark() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select up to 3 images for benchmark");
    if (files.isEmpty()) return;
    if (files.size() > 3) {
        QMessageBox::information(this, "Info", "Please select up to 3 files");
        return;
    }

    benchmarkFiles.clear();
    for (auto& f : files) benchmarkFiles.push_back(f);

    std::vector<int> threadCounts = { 1,2,4,8,16,32,64 };
    int maxThreads = std::thread::hardware_concurrency();
    if (maxThreads < 1) maxThreads = 1;

    labelInfo->setText("Benchmark running...");
    QApplication::processEvents();

    QString report;
    report += QString("Hardware threads: %1\n").arg(maxThreads);

    for (const QString& f : benchmarkFiles) {
        QImage img(f);
        if (img.isNull()) {
            report += QString("Cannot load %1\n").arg(f);
            continue;
        }

        QImage src = img.convertToFormat(QImage::Format_RGB32);
        report += QString("\nFile: %1 (%2x%3)\n").arg(f).arg(src.width()).arg(src.height());

        // Benchmark C++ version
        report += "=== C++ sharpenImage ===\n";
        for (int TC : threadCounts) {
            if (TC > 64) continue;
            std::vector<double> times;
            for (int run = 0; run < 5; ++run) {
                auto t0 = std::chrono::high_resolution_clock::now();
                QImage out = sharpenImage(src, sliderIntensity->value() / 100.0, TC);
                auto t1 = std::chrono::high_resolution_clock::now();
                times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
            }
            double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            report += QString("Threads %1: %2 ms (avg of 5)\n").arg(TC).arg(avg, 0, 'f', 2);
            QApplication::processEvents();
        }

        // Benchmark ASM version
        report += "=== ASM sharpenImageASM ===\n";
        for (int TC : threadCounts) {
            if (TC > 64) continue;
            std::vector<double> times;
            for (int run = 0; run < 5; ++run) {
                auto t0 = std::chrono::high_resolution_clock::now();
                QImage out = sharpenImageASM(src, sliderIntensity->value() / 100.0, TC);
                auto t1 = std::chrono::high_resolution_clock::now();
                times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
            }
            double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            report += QString("Threads %1: %2 ms (avg of 5)\n").arg(TC).arg(avg, 0, 'f', 2);
            QApplication::processEvents();
        }
    }

    QMessageBox msg(this);
    msg.setWindowTitle("Benchmark Results");
    msg.setText(report);
    msg.exec();
    labelInfo->setText("Benchmark finished");
}
