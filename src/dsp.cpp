#include "window.h"

#if BUILD_AIRSPY
static int _rx_callback(airspy_transfer *t)
{
    ((libdsp::Pipe<std::complex<float>> *)t->ctx)->push((std::complex<float> *)t->samples, t->sample_count);
    return 0;
};
#endif

#if BUILD_HACKRF
std::complex<float> *hackrf_read_buffer = new std::complex<float>[BUFFER_SIZE * 1000];

static int _hackrf_rx_callback(hackrf_transfer *t)
{
    for (int i = 0; i < t->valid_length / 2; ++i)
    {
        hackrf_read_buffer[i] = std::complex<float>(*((int8_t *)&t->buffer[i * 2 + 0]), *((int8_t *)&t->buffer[i * 2 + 1]));
    }

    ((libdsp::Pipe<std::complex<float>> *)t->rx_ctx)->push((std::complex<float> *)hackrf_read_buffer, t->valid_length / 2);
    return 0;
}
#endif

#if BUILD_RTLSDR
std::complex<float> *rtlsdr_read_buffer = new std::complex<float>[BUFFER_SIZE * 1000];

static void _rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    for (int i = 0; i < len / 2; ++i)
    {
        rtlsdr_read_buffer[i] = std::complex<float>(*((int8_t *)&buf[i * 2 + 0]), *((int8_t *)&buf[i * 2 + 1]));
    }

    ((libdsp::Pipe<std::complex<float>> *)ctx)->push((std::complex<float> *)rtlsdr_read_buffer, len / 2);
}
#endif

void BasebandRecorderApp::initBuffers()
{
    sdr_read_buffer = new std::complex<float>[BUFFER_SIZE];
    sample_buffer = new std::complex<float>[BUFFER_SIZE];
    sample_buffer2 = new std::complex<float>[BUFFER_SIZE];
    buffer_fft_in = new std::complex<double>[BUFFER_SIZE];
    buffer_fft_out = new std::complex<double>[BUFFER_SIZE];
    buffer_fft_final = new float[BUFFER_SIZE];
    buffer_i16 = new int16_t[BUFFER_SIZE * 2];
}

void BasebandRecorderApp::startDSPThread()
{
    // Pull all config values
    frequency = std::stof((std::string)frequencyEntry->GetValue()) * 1e6;
    samplerate = std::stof((std::string)samplerateEntry->GetValue());
    gain = gainSlider->GetValue();
#if BUILD_AIRSPY || BUILD_HACKRF
    bias = biasCheckBox->GetValue();
#endif

    // Init device and start streaming

#if BUILD_AIRSPY
    if (airspy_open(&dev) != AIRSPY_SUCCESS)
    {
        std::cout << "Could not open Airspy device!" << std::endl;
    }

    std::cout << "Opened Airspy device!" << std::endl;

    airspy_set_sample_type(dev, AIRSPY_SAMPLE_FLOAT32_IQ);
    airspy_set_samplerate(dev, samplerate);
    airspy_set_freq(dev, frequency);
    airspy_set_rf_bias(dev, bias);
    airspy_set_linearity_gain(dev, gain);

    airspy_start_rx(dev, &_rx_callback, &sdrPipe);
#endif

#if BUILD_LIME
    limeDevice = lime::LMS7_Device::CreateDevice(lime::ConnectionRegistry::findConnections()[0]);
    std::cout << limeDevice->GetInfo()->deviceName << std::endl;
    limeDevice->Init();

    limeDevice->EnableChannel(false, 0, true);
    limeDevice->SetRate(samplerate, 0);
    limeDevice->SetFrequency(false, 0, frequency);
    limeDevice->SetClockFreq(LMS_CLOCK_SXR, frequency, 0);
    limeDevice->SetGain(false, 0, gain);

    limeConfig.align = false;
    limeConfig.isTx = false;
    limeConfig.performanceLatency = 0.5;
    limeConfig.bufferLength = 0; //auto
    limeConfig.format = lime::StreamConfig::FMT_FLOAT32;
    limeConfig.channelID = 0;

    limeStreamID = limeDevice->SetupStream(limeConfig);

    if (limeStreamID == 0)
    {
        std::cout << "Could not open Lime device! " << std::endl;
    }

    std::cout << "Opened Lime device!" << std::endl;

    limeStream = limeStreamID;
    limeStream->Start();

    sdrThread = std::thread(&BasebandRecorderApp::sdrThreadFunction, this);
#endif

#if BUILD_HACKRF
    hackrf_init();

    hackrf_device_list_t *list;
    list = hackrf_device_list();

    if (list->devicecount <= 0)
    {
        std::cout << "Could not open HackRF device!" << std::endl;
    }

    hackrf_device_list_open(list, 0, &hackrf_dev);

    hackrf_set_sample_rate(hackrf_dev, samplerate);
    hackrf_set_freq(hackrf_dev, frequency);
    hackrf_set_amp_enable(hackrf_dev, 1);
    hackrf_set_baseband_filter_bandwidth(hackrf_dev, samplerate);
    hackrf_set_lna_gain(hackrf_dev, gain);
    hackrf_set_antenna_enable(hackrf_dev, bias);

    if (hackrf_start_rx(hackrf_dev, _hackrf_rx_callback, &sdrPipe) != HACKRF_SUCCESS)
    {
        std::cout << "Could not open HackRF device!" << std::endl;
    }

    std::cout << "Opened HackRF device!" << std::endl;
#endif

#if BUILD_RTLSDR
    if (rtlsdr_open(&rtl_sdr_dev, 0) <= 0)
    {
        std::cout << "Could not open RTLSDR device!" << std::endl;
    }

    rtlsdr_set_center_freq(rtl_sdr_dev, frequency);
    rtlsdr_set_sample_rate(rtl_sdr_dev, samplerate);
    rtlsdr_set_tuner_gain_mode(rtl_sdr_dev, 1);
    rtlsdr_set_tuner_gain(rtl_sdr_dev, gain * 10);

    rtlsdr_reset_buffer(rtl_sdr_dev);
    sdrThread = std::thread(&BasebandRecorderApp::sdrThreadFunction, this);
    std::cout << "Opened RTLSDR device!" << std::endl;
#endif

    // Start all the stuff!
    dspThread = std::thread(&BasebandRecorderApp::dspThreadFunction, this);
    fileThread = std::thread(&BasebandRecorderApp::fileThreadFunction, this);
}

void BasebandRecorderApp::sdrThreadFunction()
{
#if BUILD_LIME
    lime::StreamChannel::Metadata md;

    while (1)
    {
        int cnt = limeStream->Read(sdr_read_buffer, BUFFER_SIZE, &md);
        sdrPipe.push(sdr_read_buffer, cnt);
    }
#endif

#if BUILD_RTLSDR
    while (1)
    {
        rtlsdr_mutex.lock();
        rtlsdr_read_async(rtl_sdr_dev, _rtlsdr_callback, &sdrPipe, 0, 0);
        rtlsdr_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
}

void BasebandRecorderApp::fileThreadFunction()
{
    while (1)
    {
        int cnt = fileFifo.pop(sample_buffer2, BUFFER_SIZE, 1000);
        if (recording)
        {
            for (int i = 0; i < cnt; i++)
            {
                buffer_i16[i * 2 + 0] = sample_buffer2[i].real() * 1000;
                buffer_i16[i * 2 + 1] = sample_buffer2[i].imag() * 1000;
            }

            data_out.write((char *)buffer_i16, cnt * 4);
            datasize += cnt;
            data_out.flush();
        }
    }
}

void BasebandRecorderApp::dspThreadFunction()
{
    fftw_plan p = fftw_plan_dft_1d(BUFFER_SIZE, (fftw_complex *)buffer_fft_in, (fftw_complex *)buffer_fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
    libdsp::AgcCC agc = libdsp::AgcCC(0.00001, 1.0, 1.0, 1e5);

    int refresh_per_second = 20;
    int runs_to_wait = (samplerate / BUFFER_SIZE) / (refresh_per_second * 3);

    int i = 0, y = 0, cnt = 0;

    while (1)
    {
        cnt = sdrPipe.pop(sample_buffer, BUFFER_SIZE, 1000);

        agc.work(sample_buffer, cnt, sample_buffer);

        fileFifo.push(sample_buffer, cnt);

        if (y % runs_to_wait == 0)
        {
            for (int i = 0; i < BUFFER_SIZE; i++)
                buffer_fft_in[i] = sample_buffer[i];

            fftw_execute(p);

            for (int i = 0, iMax = BUFFER_SIZE / 2; i < iMax; i++)
            {
                float a = buffer_fft_out[i].real();
                float b = buffer_fft_out[i].imag();
                float c = sqrt(a * a + b * b);

                float x = buffer_fft_out[BUFFER_SIZE / 2 + i].real();
                float y = buffer_fft_out[BUFFER_SIZE / 2 + i].imag();
                float z = sqrt(x * x + y * y);

                buffer_fft_final[i] = (z)*1.7;
                buffer_fft_final[BUFFER_SIZE / 2 + i] = (c)*1.7;
            }

            for (int i = 0; i < 1024; i++)
            {
                panadapterViewer->constellation_buffer[i] = (buffer_fft_final[i] * 1 + panadapterViewer->constellation_buffer[i] * 9) / 10;
            }

            if (i % 3 == 0)
            {
                wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter([=]() {
                    panadapterViewer->Refresh();
                    recordingDataSizeLabel->SetLabelText("Data out : " + (datasize * 4 > 1e9 ? to_string_with_precision<float>(datasize * 4 / 1e9, 2) + " GB" : to_string_with_precision<float>(datasize * 4 / 1e6, 2) + " MB"));
                });
            }

            i++;

            if (i == 10000000)
                i = 0;
        }

        if (y == 10000000)
            y = 0;
        y++;
    }
}