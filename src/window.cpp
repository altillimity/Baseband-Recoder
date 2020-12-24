#include "window.h"

bool BasebandRecorderApp::OnInit()
{
    initBuffers();

    BasebandRecorderWindow *frame = new BasebandRecorderWindow();
    frame->SetSize(1024, 600);

    // Constellation viewer
    panadapterViewer = new PanadapterWidget((wxFrame *)frame);
    panadapterViewer->SetSize(1024, 200);
    panadapterViewer->SetPosition(wxPoint(0, 0));

    settingsPanel = new wxPanel((wxFrame *)frame);
    settingsPanel->Show(true);

    recordingPanel = new wxPanel((wxFrame *)frame);
    recordingPanel->Show(true);

    sdrPanel = new wxPanel((wxFrame *)frame);
    sdrPanel->Show(true);

    sdrLabel = new wxStaticText((wxFrame *)sdrPanel, 0, "Radio", wxPoint(5, 10));
    {
        wxFont font = sdrLabel->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        sdrLabel->SetFont(font);
    }
    samplerateLabel = new wxStaticText((wxFrame *)sdrPanel, 0, "Samplerate (Hz)", wxPoint(5, 30));
#if BUILD_AIRSPY || BUILD_HACKRF || BUILD_LIME
    samplerateEntry = new wxTextCtrl((wxFrame *)sdrPanel, 0, "6000000", wxPoint(5, 55));
#endif
#if BUILD_RTLSDR
    samplerateEntry = new wxTextCtrl((wxFrame *)sdrPanel, 0, "2400000", wxPoint(5, 55));
#endif
    startSDRButton = new wxButton((wxFrame *)sdrPanel, START_SDR_BTN_ID, _T("Start"), wxPoint(5, 85), wxDefaultSize, 0);
    stopSDRButton = new wxButton((wxFrame *)sdrPanel, STOP_SDR_BTN_ID, _T("Stop"), wxPoint(95, 85), wxDefaultSize, 0);
    stopSDRButton->Disable();

    settingsLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "Settings", wxPoint(15, 10));
    {
        wxFont font = settingsLabel->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        settingsLabel->SetFont(font);
    }
    frequencyLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "Frequency (MHz) :", wxPoint(15, 40));
    frequencyEntry = new wxTextCtrl((wxFrame *)settingsPanel, 0, "100.0", wxPoint(135, 35));
    setFrequencyButton = new wxButton((wxFrame *)settingsPanel, SET_FREQ_BTN_ID, _T("Set"), wxPoint(235, 35), wxDefaultSize, 0);

    gainLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "Gain (dB) :", wxPoint(15, 70));
#if BUILD_AIRSPY
    gainValueLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "22", wxPoint(85, 70));
    gainSlider = new wxSlider((wxFrame *)settingsPanel, GAIN_SLIDER_ID, 22, 0, 22, wxPoint(105, 70), wxSize(200, 20));
#include <libairspy/airspy.h>
#endif

#if BUILD_LIME
    gainValueLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "49", wxPoint(85, 70));
    gainSlider = new wxSlider((wxFrame *)settingsPanel, GAIN_SLIDER_ID, 49, 0, 49, wxPoint(105, 70), wxSize(200, 20));
#endif

#if BUILD_HACKRF || BUILD_RTLSDR
    gainValueLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "49", wxPoint(85, 70));
    gainSlider = new wxSlider((wxFrame *)settingsPanel, GAIN_SLIDER_ID, 49, 0, 49, wxPoint(105, 70), wxSize(200, 20));
#endif

#if BUILD_AIRSPY || BUILD_HACKRF
    biasLabel = new wxStaticText((wxFrame *)settingsPanel, 0, "Bias-tee :", wxPoint(15, 100));
    biasCheckBox = new wxCheckBox((wxFrame *)settingsPanel, BIAS_CHECKBOX_ID, "Enable", wxPoint(105, 97));
#endif

    recordingLabel = new wxStaticText((wxFrame *)recordingPanel, 0, "Recording", wxPoint(15, 10));
    {
        wxFont font = recordingLabel->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        recordingLabel->SetFont(font);
    }
    startButton = new wxButton((wxFrame *)recordingPanel, START_RECORD_BTN_ID, _T("Start"), wxPoint(15, 85), wxDefaultSize, 0);
    stopButton = new wxButton((wxFrame *)recordingPanel, STOP_RECORD_BTN_ID, _T("Stop"), wxPoint(105, 85), wxDefaultSize, 0);
    recordingStatusLabel = new wxStaticText((wxFrame *)recordingPanel, 0, "State : IDLE", wxPoint(15, 40));
    recordingDataSizeLabel = new wxStaticText((wxFrame *)recordingPanel, 0, "Data out : 0.00 MB", wxPoint(15, 60));
    stopButton->Disable();

    mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(panadapterViewer, 1, wxEXPAND);
    secondarySizer = new wxBoxSizer(wxHORIZONTAL);
    secondarySizer->Add(sdrPanel, 1, wxEXPAND);
    secondarySizer->Add(settingsPanel, 1, wxEXPAND);
    secondarySizer->Add(recordingPanel, 1, wxEXPAND);
    mainSizer->Add(secondarySizer, 0.5, wxEXPAND);
    frame->SetSizer(mainSizer);
    frame->SetAutoLayout(true);

    // Bind buttons events
    Bind(wxEVT_BUTTON, [&](wxCommandEvent &event) {
        if (event.GetId() == SET_FREQ_BTN_ID)
        {
            frequency = std::stof((std::string)frequencyEntry->GetValue()) * 1e6;
#if BUILD_AIRSPY
            airspy_set_freq(dev, frequency);
#endif
#if BUILD_LIME
            limeDevice->SetFrequency(false, 0, frequency);
#endif
#if BUILD_HACKRF
            hackrf_set_freq(hackrf_dev, frequency);
#endif
#if BUILD_RTLSDR
            rtlsdr_cancel_async(rtl_sdr_dev);
            rtlsdr_mutex.lock();
            int tries = 0;
            while (frequency != rtlsdr_get_center_freq(rtl_sdr_dev) && tries < 10)
            {
                tries++;
                rtlsdr_set_center_freq(rtl_sdr_dev, frequency);
            }
            rtlsdr_mutex.unlock();
#endif
        }
        else if (event.GetId() == START_RECORD_BTN_ID)
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp =
                (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

            std::string filename = timestamp + "_" + std::to_string(frequency) + "Hz.wav";

            data_out = std::ofstream(filename, std::ios::binary);

            recordingStatusLabel->SetLabelText("State : REC " + filename);

            datasize = 0;

            startButton->Disable();
            stopButton->Enable();
            recording = true;
        }
        else if (event.GetId() == STOP_RECORD_BTN_ID)
        {
            recording = false;

            std::this_thread::sleep_for(std::chrono::seconds(1));

            data_out.close();

            stopButton->Disable();
            startButton->Enable();

            recordingStatusLabel->SetLabelText("State : IDLE ");
        }
        else if (event.GetId() == STOP_SDR_BTN_ID)
        {
        }
        else if (event.GetId() == START_SDR_BTN_ID)
        {
            startDSPThread();

            startSDRButton->Disable();
            samplerateEntry->Disable();
        }
    });

    // Bind slider events
    Bind(wxEVT_SLIDER, [&](wxCommandEvent &event) {
        if (event.GetId() == GAIN_SLIDER_ID)
        {
            gain = gainSlider->GetValue();
#if BUILD_AIRSPY
            airspy_set_linearity_gain(dev, gain);
#endif
#if BUILD_LIME
            limeDevice->SetGain(false, 0, gain);
#endif
#if BUILD_HACKRF
            hackrf_set_lna_gain(hackrf_dev, gain);
#endif
#if BUILD_RTLSDR
            rtlsdr_set_tuner_gain(rtl_sdr_dev, gain * 10);
#endif
            gainValueLabel->SetLabelText(std::to_string(gain));
        }
    });

    // Bind checkbox events
    Bind(wxEVT_CHECKBOX, [&](wxCommandEvent &event) {
#if BUILD_AIRSPY
        if (event.GetId() == BIAS_CHECKBOX_ID)
        {
            bias = biasCheckBox->GetValue();
            airspy_set_rf_bias(dev, bias);
        }
#endif
#if BUILD_HACKRF
        if (event.GetId() == BIAS_CHECKBOX_ID)
        {
            bias = biasCheckBox->GetValue();
            hackrf_set_antenna_enable(hackrf_dev, bias);
        }
#endif
    });

    frame->Show(true);

    return true;
}

BasebandRecorderWindow::BasebandRecorderWindow() : wxFrame(NULL, wxID_ANY, "Baseband Recorder (by Aang23)")
{
}