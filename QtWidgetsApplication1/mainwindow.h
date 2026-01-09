#pragma once


//Aplikacja do wyostrzania obrazu z wykorzystaniem C++ oraz ASM
//Semestr zimowy 2025/2026
//Wojciech Pêdziwiatr

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QImage>
#include <vector>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
	void onLoad(); //wczytanie obrazu
    void onApply(); //wyostrzanie w c++
	void onApplyASM(); //wyostrzanie w ASM
	void onSave(); // zapis obrazu
	void onBenchmark(); //test wydajnoœci

private:
	QLabel* labelBefore; // obraz przed wyostrzeniem 
	QLabel* labelAfter; // obraz po wyostrzeniu
	QLabel* labelInfo; // informacje o obrazie
	QSlider* sliderIntensity; // intensywnoœæ wyostrzania
	QSpinBox* spinThreads; // liczba w¹tków
    QPushButton* btnLoad, * btnApply, * btnApplyASM, * btnBenchmark, * btnSave;

	QImage imgSrc; //obraz Ÿród³owy
    QImage imgResult; //obraz wynikowy
    std::vector<QString> benchmarkFiles;

    int clamp(int v);
	//filtr uœrednij¹cy 3x3 
    void boxBlur3x3_worker(const QImage& src, QImage& dst, int y0, int y1);

	//wyostrzenie obrazu z wykorzystaniem C++
    QImage sharpenImage(const QImage& src, double amount, int threads);

	//wyostrzenie obrazu z wykorzystaniem ASM
    QImage sharpenImageASM(const QImage& src, double amount, int threads);

};
