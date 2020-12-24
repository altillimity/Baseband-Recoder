# Baseband Recoder

A simple minimalistic (maybe even too much?) baseband recoder. Still kinda WIP!

# Building

This will require :
 - [libdsp](https://github.com/altillimity/libdsp)
 - [wxwidgets](https://www.wxwidgets.org/)
 - [fftw3](http://fftw.org/)
 - Your SDR API, eg, libhackrf, libairspy, librtlsdr, etc

 Then, building is done by cmake :
 ```
git clone https://github.com/altillimity/Baseband-Recorder.git
cd Baseband-Recorder
mkdir build && cd build
cmake -DBUILD_AIRSPY=1 .. # Replace -DBUILD_AIRSPY with your selected SDR if needed, eg, -DBUILD_LIME=1, -DBUILD_RTLSDR=1, -DBUILD_HACKRF=1
make -j2
```

Currently, support SDRs are :
- Airspy R2 / Mini
- RTLSDR
- LimeSDR and LimeSDR Mini
- HackRF