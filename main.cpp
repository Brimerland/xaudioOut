#include <windows.h>
#include <xaudio2.h>

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#pragma comment ( lib, "Xaudio2")
#pragma comment ( lib, "winmm")

int sampleRate = 48000;
int channelCount = 1;
int bytesPerChannel = 2;

class VoiceCallbacks : public IXAudio2VoiceCallback
{
    XAUDIO2_BUFFER xaBuffers[2];

    void InitXAudioBuffer(XAUDIO2_BUFFER &b)
    {
        b.Flags = 0;
        b.AudioBytes = 960 * channelCount * bytesPerChannel;
        b.pAudioData = new BYTE[b.AudioBytes];
        b.LoopBegin = 0;
        b.LoopCount = 0;
        b.LoopLength = 0;
        b.PlayBegin = 0;
        b.PlayLength = 0;
        b.pContext = 0;
    }

    void DestroyXAudioBuffer(XAUDIO2_BUFFER &b)
    {
        if (b.pAudioData)
            delete b.pAudioData;
    }

    XAUDIO2_BUFFER *GetFreeBuffer()
    {
        XAUDIO2_BUFFER *result = nullptr;

        int i = sizeof(xaBuffers) / sizeof(*xaBuffers);
        while (i-- && !result)
        {
            result = xaBuffers + i;
            if (result->pContext)
                result = nullptr;
        }

        return result;
    }

    unsigned int sampleNr;
    XAUDIO2_BUFFER *PrepareNextBuffer()
    {
        auto result = GetFreeBuffer();
        if (result)
        {
            int sampleCount = result->AudioBytes / (channelCount * bytesPerChannel);
            switch (bytesPerChannel)
            {
            case 1:
                {
                    auto b = (BYTE*)result->pAudioData;
                    for (int i = 0; i < sampleCount; i++)
                    {
                        b[0] = (BYTE) (128 + 127 * sin(frequency * 2 * M_PI * (sampleNr++ / (float)sampleRate)));

                        for (int j = channelCount - 1; j > 0; j--)
                            b[j] = b[0];
                        
                        b += channelCount;
                    }
                }
                break;
            case 2:
                {
                    auto b = (WORD*)result->pAudioData;
                    for (int i = 0; i < sampleCount; i++)
                    {
                        b[0] = (WORD)(32767 * sin(frequency * 2 * M_PI * (sampleNr++ / (float)sampleRate)));

                        for (int j = channelCount - 1; j > 0; j--)
                            b[j] = b[0];

                        b += channelCount;
                    }
                }
                break;
            default:
                result = nullptr;
            }
            
            if (result)
                result->pContext = result;
        }

        return result;
    }

    void SubmitNextBuffer()
    {
        if (voice)
        {
            auto buffer = PrepareNextBuffer();
            if (buffer)
                voice->SubmitSourceBuffer(buffer);
        }
    }

public:
    VoiceCallbacks()
    {
        sampleNr = 0;
        voice = nullptr;
        frequency = 440;
        InitXAudioBuffer(xaBuffers[0]);
        InitXAudioBuffer(xaBuffers[1]);
    }

    ~VoiceCallbacks()
    {
        DestroyXAudioBuffer(xaBuffers[0]);
        DestroyXAudioBuffer(xaBuffers[1]);
    }

    void STDAPICALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired)
    {
        //printf("passstart %d\n", BytesRequired);

        SubmitNextBuffer();
    }

    void STDAPICALLTYPE OnVoiceProcessingPassEnd()
    {
        //printf("passend\n");
    }

    void STDAPICALLTYPE OnStreamEnd()
    {
        //printf("streamend\n");
    }

    void STDAPICALLTYPE OnBufferStart(void* pBufferContext)
    {
        //printf("bufferstart\n");
    }

    void STDAPICALLTYPE OnBufferEnd(void* pBufferContext)
    {
        //printf("bufferend\n");
        ((XAUDIO2_BUFFER*)pBufferContext)->pContext = nullptr;
        SubmitNextBuffer();
    }

    void STDAPICALLTYPE OnLoopEnd(void* pBufferContext)
    {
        //printf("loopEnd\n");
    }

    void STDAPICALLTYPE OnVoiceError(void* pBufferContext, HRESULT error)
    {
        printf("error\n");
    }

    IXAudio2SourceVoice *voice;
    float frequency;
};

int main()
{
    // initialize com
    if (SUCCEEDED(CoInitializeEx(NULL, COINITBASE_MULTITHREADED)))
    {
        // create xaudio object
        IXAudio2 *xaudio2;
        if (SUCCEEDED(XAudio2Create(&xaudio2)))
        {
            // create mastering voice and source voice
            IXAudio2MasteringVoice *masteringVoice;
            IXAudio2SourceVoice *sourceVoice;

            WAVEFORMATEX waveFormat;
            waveFormat.cbSize = sizeof(waveFormat);
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = channelCount;
            waveFormat.nSamplesPerSec = sampleRate;
            waveFormat.wBitsPerSample = bytesPerChannel * 8;
            waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample / 8);
            waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

            VoiceCallbacks voiceCB;

            if (SUCCEEDED(xaudio2->CreateMasteringVoice(&masteringVoice)) 
                && SUCCEEDED(xaudio2->CreateSourceVoice(&sourceVoice, &waveFormat, 0, 1.0f, &voiceCB)))
            {
                voiceCB.voice = sourceVoice;
                sourceVoice->Start();

                getchar();
            }

            xaudio2->Release();
        }

        CoUninitialize();
    }

    return 0;
}