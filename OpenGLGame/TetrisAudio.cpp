#include "pch.h"
#include "TetrisAudio.h"

#include "TetrisGame.h"

namespace
{
    constexpr int kSampleRate = 44100;
    constexpr double kPi = 3.14159265358979323846;
}

TetrisAudio::TetrisAudio()
{
}

TetrisAudio::~TetrisAudio()
{
    release();
}

void TetrisAudio::initialize()
{
    if (running_)
    {
        return;
    }

    running_ = true;
    worker_ = std::thread(&TetrisAudio::audioThreadMain, this);
}

void TetrisAudio::release()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        queue_.clear();
    }
    condition_.notify_all();

    if (worker_.joinable())
    {
        worker_.join();
    }
}

void TetrisAudio::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    lastLineClearEventId_ = 0;
    lastHardDropEventId_ = 0;
    lastHoldEventId_ = 0;
    lastGameOverEventId_ = 0;
    lastLevelUpEventId_ = 0;
    lastResetEventId_ = 0;
    lastPaused_ = false;
}

void TetrisAudio::update(const TetrisGame& game)
{
    // 오디오는 "키를 눌렀는가"가 아니라 "게임에서 어떤 이벤트가 확정됐는가"를 기준으로 반응한다.
    // 이렇게 해야 홀드, 라인 클리어 지연, 재시작처럼 입력과 실제 결과 사이에 시간차가 있어도
    // 화면/게임 상태/사운드가 정확히 같은 타이밍을 바라보게 된다.
    if (game.getResetEventId() != lastResetEventId_)
    {
        lastResetEventId_ = game.getResetEventId();
        reset();
        lastResetEventId_ = game.getResetEventId();
    }

    if (game.getHoldEventId() != lastHoldEventId_)
    {
        lastHoldEventId_ = game.getHoldEventId();
        enqueue({ { 523.25, 36, 0.12f, 2.0f, 0.03f }, { 659.25, 52, 0.13f, 2.0f, 0.03f } });
    }

    if (game.getHardDropEventId() != lastHardDropEventId_)
    {
        lastHardDropEventId_ = game.getHardDropEventId();
        enqueue({ { 196.00, 34, 0.18f, 1.5f, 0.02f }, { 146.83, 44, 0.14f, 2.0f, 0.015f } });
    }

    if (game.getLineClearEventId() != lastLineClearEventId_)
    {
        lastLineClearEventId_ = game.getLineClearEventId();
        switch (game.getLastClearedRowCount())
        {
        case 4:
            enqueue({ { 523.25, 46, 0.13f, 2.0f, 0.04f }, { 659.25, 52, 0.14f, 2.0f, 0.04f }, { 783.99, 72, 0.16f, 2.0f, 0.05f } });
            break;
        case 3:
            enqueue({ { 493.88, 40, 0.12f, 2.0f, 0.04f }, { 659.25, 58, 0.14f, 2.0f, 0.04f } });
            break;
        case 2:
            enqueue({ { 440.00, 38, 0.11f, 2.0f, 0.03f }, { 587.33, 50, 0.13f, 2.0f, 0.04f } });
            break;
        default:
            enqueue({ { 392.00, 46, 0.11f, 2.0f, 0.03f } });
            break;
        }
    }

    if (game.getLevelUpEventId() != lastLevelUpEventId_)
    {
        lastLevelUpEventId_ = game.getLevelUpEventId();
        enqueue({ { 523.25, 36, 0.11f, 2.0f, 0.03f }, { 659.25, 40, 0.12f, 2.0f, 0.04f }, { 880.00, 64, 0.14f, 2.0f, 0.05f } });
    }

    if (game.getGameOverEventId() != lastGameOverEventId_)
    {
        lastGameOverEventId_ = game.getGameOverEventId();
        enqueue({ { 329.63, 72, 0.10f, 1.5f, 0.02f }, { 246.94, 92, 0.10f, 1.5f, 0.02f }, { 164.81, 150, 0.11f, 1.5f, 0.015f } });
    }

    if (game.isPaused() != lastPaused_)
    {
        lastPaused_ = game.isPaused();
        enqueue(lastPaused_ ? std::initializer_list<Tone>{ { 349.23, 70, 0.09f, 2.0f, 0.02f } }
                            : std::initializer_list<Tone>{ { 523.25, 38, 0.10f, 2.0f, 0.03f }, { 659.25, 30, 0.10f, 2.0f, 0.03f } });
    }
}

void TetrisAudio::enqueue(const std::initializer_list<Tone>& tones)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const Tone& tone : tones)
        {
            queue_.push_back(tone);
        }
    }
    condition_.notify_one();
}

void TetrisAudio::audioThreadMain()
{
    while (true)
    {
        Tone tone;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() { return !running_ || !queue_.empty(); });

            if (!running_ && queue_.empty())
            {
                return;
            }

            // 오디오 장치 열기와 샘플 전송은 환경에 따라 순간적으로 느릴 수 있다.
            // 별도 스레드에서 재생하면 메인 게임 루프가 끊기지 않는다.
            tone = queue_.front();
            queue_.pop_front();
        }

        playTone(tone);
    }
}

void TetrisAudio::playTone(const Tone& tone) const
{
    if (tone.frequency <= 0.0 || tone.durationMs == 0)
    {
        ::Sleep(tone.durationMs);
        return;
    }

    const int sampleCount = std::max(1, static_cast<int>((static_cast<double>(tone.durationMs) / 1000.0) * kSampleRate));
    std::vector<short> samples(static_cast<size_t>(sampleCount));

    const int attackSamples = std::max(1, static_cast<int>(sampleCount * 0.12));
    const int releaseSamples = std::max(1, static_cast<int>(sampleCount * 0.24));

    // 기본 파형은 사인파로 두고, 약한 배음과 어택/릴리즈 엔벨로프를 더한다.
    // 이렇게 하면 Windows Beep 같은 딱딱한 시스템음보다 훨씬 부드럽고 게임 사운드에 가깝게 들린다.
    for (int index = 0; index < sampleCount; ++index)
    {
        const double time = static_cast<double>(index) / static_cast<double>(kSampleRate);
        const double base = std::sin(2.0 * kPi * tone.frequency * time);
        const double overtone = std::sin(2.0 * kPi * (tone.frequency * tone.overtoneRatio) * time);

        double envelope = 1.0;
        if (index < attackSamples)
        {
            envelope = static_cast<double>(index) / static_cast<double>(attackSamples);
        }
        else if (index > sampleCount - releaseSamples)
        {
            envelope = static_cast<double>(sampleCount - index) / static_cast<double>(releaseSamples);
        }

        envelope = std::clamp(envelope, 0.0, 1.0);
        const double value = (base * tone.gain + overtone * tone.overtoneGain) * envelope;
        samples[static_cast<size_t>(index)] = static_cast<short>(std::clamp(value, -1.0, 1.0) * 32767.0);
    }

    WAVEFORMATEX waveFormat{};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = kSampleRate;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = static_cast<WORD>((waveFormat.nChannels * waveFormat.wBitsPerSample) / 8);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    HANDLE doneEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (doneEvent == nullptr)
    {
        return;
    }

    HWAVEOUT waveOutHandle = nullptr;
    MMRESULT openResult = ::waveOutOpen(&waveOutHandle, WAVE_MAPPER, &waveFormat, reinterpret_cast<DWORD_PTR>(doneEvent), 0, CALLBACK_EVENT);
    if (openResult != MMSYSERR_NOERROR)
    {
        ::CloseHandle(doneEvent);
        return;
    }

    WAVEHDR header{};
    header.lpData = reinterpret_cast<LPSTR>(samples.data());
    header.dwBufferLength = static_cast<DWORD>(samples.size() * sizeof(short));

    // samples 버퍼는 이 함수 스택에 올라가 있으므로, 재생이 끝나기 전에 해제되면 안 된다.
    // 따라서 WOM_DONE 신호를 기다린 뒤 헤더를 정리하고 장치를 닫는다.
    if (::waveOutPrepareHeader(waveOutHandle, &header, sizeof(header)) == MMSYSERR_NOERROR)
    {
        if (::waveOutWrite(waveOutHandle, &header, sizeof(header)) == MMSYSERR_NOERROR)
        {
            ::WaitForSingleObject(doneEvent, tone.durationMs + 250);
        }

        ::waveOutUnprepareHeader(waveOutHandle, &header, sizeof(header));
    }

    ::waveOutReset(waveOutHandle);
    ::waveOutClose(waveOutHandle);
    ::CloseHandle(doneEvent);
}
