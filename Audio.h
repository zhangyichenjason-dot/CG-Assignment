#pragma once
#include <windows.h>
#include <xaudio2.h>     // DirectX Audio
#include <mfapi.h>       // Media Foundation
#include <mfidl.h>
#include <mfreadwrite.h> // Source Reader
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>


// 链接必要的库
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class AudioSystem
{
private:
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2MasteringVoice* pMasterVoice = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    
    // 音频数据缓存
    std::vector<BYTE> audioData;
    // 使用 EXTENSIBLE 结构以支持更复杂的格式，虽然 MP3 解码后通常是标准 PCM
    WAVEFORMATEXTENSIBLE wfx = { 0 };

public:
    AudioSystem()
    {
        // 1. 初始化 COM (Media Foundation 需要)
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        // 2. 初始化 Media Foundation
        hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to init Media Foundation.", "Audio Error", MB_OK | MB_ICONERROR);
            return;
        }

        // 3. 创建 XAudio2 引擎
        hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to init XAudio2 engine.", "Audio Error", MB_OK | MB_ICONERROR);
            return;
        }

        // 4. 创建主声音 (Mastering Voice)
        hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to create mastering voice.", "Audio Error", MB_OK | MB_ICONERROR);
        }
    }

    ~AudioSystem()
    {
        stopBGM();
        if (pMasterVoice) pMasterVoice->DestroyVoice();
        if (pXAudio2) pXAudio2->Release();
        
        // 反初始化 MF 和 COM
        MFShutdown();
        CoUninitialize();
    }

    // 播放 BGM (支持 MP3, WAV 等 Windows 原生支持的所有格式)
    void playBGM(const std::string& filename)
    {
        stopBGM();

        // 使用 Media Foundation 加载并解码音频
        if (!loadAudio(filename)) {
            return;
        }

        // 创建源声音 (Source Voice)
        // 注意：传入的是 WAVEFORMATEX 指针，wfx.Format 是其基类部分
        HRESULT hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx);
        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to create source voice.", "Audio Error", MB_OK | MB_ICONERROR);
            return;
        }

        // 提交音频数据缓冲区
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = (UINT32)audioData.size();
        buffer.pAudioData = audioData.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE; // 设置无限循环

        hr = pSourceVoice->SubmitSourceBuffer(&buffer);
        if (FAILED(hr)) {
            MessageBoxA(NULL, "Failed to submit buffer.", "Audio Error", MB_OK | MB_ICONERROR);
            return;
        }

        // 开始播放
        hr = pSourceVoice->Start(0);
    }

    void stopBGM()
    {
        if (pSourceVoice)
        {
            pSourceVoice->Stop();
            pSourceVoice->FlushSourceBuffers();
            pSourceVoice->DestroyVoice();
            pSourceVoice = nullptr;
        }
        // 清理旧数据
        audioData.clear();
        memset(&wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
    }

    // 设置音量 (0 - 1000)
    void setVolume(int volume)
    {
        if (pSourceVoice)
        {
            // 映射 0-1000 到 0.0-1.0
            float vol = (float)volume / 1000.0f;
            pSourceVoice->SetVolume(vol);
        }
    }

private:
    // 使用 Media Foundation 读取并解码音频文件
    bool loadAudio(const std::string& filename)
    {
        // 转换文件名为宽字符 (MF API 需要)
        int len = MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, NULL, 0);
        std::vector<wchar_t> wFilename(len);
        MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, wFilename.data(), len);

        IMFSourceReader* pReader = nullptr;
        
        // 1. 创建 Source Reader
        HRESULT hr = MFCreateSourceReaderFromURL(wFilename.data(), NULL, &pReader);
        if (FAILED(hr)) {
            std::string msg = "Cannot open file: " + filename + "\nEnsure path is correct.";
            MessageBoxA(NULL, msg.c_str(), "Load Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // 2. 选择第一个音频流
        pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, false);
        pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, true);

        // 3. 配置输出格式为 PCM (解码 MP3)
        IMFMediaType* pPartialType = nullptr;
        MFCreateMediaType(&pPartialType);
        pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        
        hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType);
        pPartialType->Release();

        if (FAILED(hr)) {
            pReader->Release();
            MessageBoxA(NULL, "Failed to set media type to PCM.", "Format Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // 4. 获取完整的解码后格式信息
        IMFMediaType* pUncompressedAudioType = nullptr;
        hr = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType);
        if (FAILED(hr)) {
            pReader->Release();
            return false;
        }

        // 将 MF 格式转换为 WAVEFORMATEX
        WAVEFORMATEX* pWfx = nullptr;
        UINT32 cbFormat = 0;
        MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType, &pWfx, &cbFormat);
        pUncompressedAudioType->Release();

        // 复制格式到成员变量
        if (cbFormat <= sizeof(WAVEFORMATEXTENSIBLE)) {
            memcpy(&wfx, pWfx, cbFormat);
        } else {
            // 格式太复杂，尝试只复制基础部分
            memcpy(&wfx, pWfx, sizeof(WAVEFORMATEX));
            wfx.Format.cbSize = 0;
        }
        CoTaskMemFree(pWfx);

        // 5. 读取所有音频数据
        while (true)
        {
            IMFSample* pSample = nullptr;
            DWORD flags = 0;
            hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &flags, NULL, &pSample);
            
            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
                break;
            }

            if (pSample)
            {
                IMFMediaBuffer* pBuffer = nullptr;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)))
                {
                    BYTE* pAudioData = nullptr;
                    DWORD cbCurrentLength = 0;
                    if (SUCCEEDED(pBuffer->Lock(&pAudioData, NULL, &cbCurrentLength)))
                    {
                        // 追加数据到 vector
                        size_t oldSize = audioData.size();
                        audioData.resize(oldSize + cbCurrentLength);
                        memcpy(audioData.data() + oldSize, pAudioData, cbCurrentLength);
                        pBuffer->Unlock();
                    }
                    pBuffer->Release();
                }
                pSample->Release();
            }
        }

        pReader->Release();
        return true;
    }
};