#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "ids.h"
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include "panadapter.h"

#ifndef BUILD_AIRSPY
#define BUILD_AIRSPY 1
#endif
#ifndef BUILD_LIME
#define BUILD_LIME 0
#endif
#ifndef BUILD_HACKRF
#define BUILD_HACKRF 0
#endif
#ifndef BUILD_RTLSDR
#define BUILD_RTLSDR 0
#endif

#include "utils.h"
#include <fstream>
#include <thread>
#include <complex>
#include <fftw3.h>
#include <dsp/agc.h>
#include <dsp/pipe.h>
#include <dsp/rational_resampler.h>

#if BUILD_AIRSPY
#include <libairspy/airspy.h>
#endif

#if BUILD_LIME
#include <lime/lms7_device.h>
#include <lime/Streamer.h>
#include <lime/ConnectionRegistry.h>
#endif

#if BUILD_HACKRF
#include <libhackrf/hackrf.h>
#endif

#if BUILD_RTLSDR
#include <rtl-sdr.h>
#include <mutex>
#endif

#define BUFFER_SIZE (1024)

class BasebandRecorderApp : public wxApp
{
private:
    virtual bool OnInit();
    PanadapterWidget *panadapterViewer;

    // Settings widgets
    wxStaticText *settingsLabel;

    wxStaticText *frequencyLabel;
    wxTextCtrl *frequencyEntry;
    wxButton *setFrequencyButton;

    wxStaticText *gainLabel;
    wxSlider *gainSlider;
    wxStaticText *gainValueLabel;

#if BUILD_AIRSPY || BUILD_HACKRF
    wxStaticText *biasLabel;
    wxCheckBox *biasCheckBox;
#endif

    wxCheckBox *agcCheckBox;

    // Recording control widgets
    wxStaticText *recordingLabel;
    wxStaticText *decimationLabel;
    wxSpinCtrl *decimationSpin;
    wxButton *startButton, *stopButton;
    wxStaticText *recordingStatusLabel;
    wxStaticText *recordingDataSizeLabel;

    wxPanel *sdrPanel, *settingsPanel, *recordingPanel;
    wxBoxSizer *mainSizer, *secondarySizer;

    wxStaticText *sdrLabel;

    wxStaticText *samplerateLabel;
    wxTextCtrl *samplerateEntry;
    wxButton *startSDRButton;
    wxButton *stopSDRButton;
    wxStaticText *scaleLabel;
    wxSlider *scaleSlider;

private:
    // Recording stuff
    std::ofstream data_out;
    bool recording = false;
    size_t datasize = 0;

    // SDR Settings and other values
    uint32_t frequency = 100e6;
    int samplerate = 6e6;
    int gain = 49;
#if BUILD_AIRSPY || BUILD_HACKRF
    bool bias = false;
#endif

private:
    // DSP Stuff
    std::thread dspThread;
    std::thread fileThread;
#if BUILD_LIME || BUILD_RTLSDR
    std::thread sdrThread;
#endif
    void startDSPThread();
    void initBuffers();

    std::shared_ptr<libdsp::AgcCC> agc;

private:
    int decimation = 1;
    std::shared_ptr<libdsp::RationalResamplerCCF> resampler;

private:
    float fft_vertical_scale;

private:
    // FIFOs
    libdsp::Pipe<std::complex<float>> sdrPipe;
    libdsp::Pipe<std::complex<float>> fileFifo;

private:
    void dspThreadFunction();
    void sdrThreadFunction();
    void fileThreadFunction();

private:
    // All buffers we use along the way
    std::complex<float> *sdr_read_buffer; /// = new std::complex<float>[BUFFER_SIZE];
    std::complex<float> *sample_buffer;   // = new std::complex<float>[BUFFER_SIZE];
    std::complex<float> *sample_buffer2;  // = new std::complex<float>[BUFFER_SIZE];
    std::complex<double> *buffer_fft_in;  // = new std::complex<double>[BUFFER_SIZE];
    std::complex<double> *buffer_fft_out; // = new std::complex<double>[BUFFER_SIZE];
    float *buffer_fft_final;              // = new float[BUFFER_SIZE];
    int16_t *buffer_i16;                  // = new int16_t[BUFFER_SIZE * 2];

private:
#if BUILD_AIRSPY
    struct airspy_device *dev;
#endif

#if BUILD_LIME
    lime::LMS7_Device *limeDevice;
    lime::StreamChannel *limeStream;
    lime::StreamChannel *limeStreamID;
    lime::StreamConfig limeConfig;
#endif

#if BUILD_HACKRF
    hackrf_device *hackrf_dev;
#endif

#if BUILD_RTLSDR
    rtlsdr_dev *rtl_sdr_dev;
    std::mutex rtlsdr_mutex;
#endif
};

class BasebandRecorderWindow : public wxFrame
{
public:
    BasebandRecorderWindow();
};
