#pragma once

#ifndef TETRISAUDIO_H_
#define TETRISAUDIO_H_

#include "pch.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

class TetrisGame;

class TetrisAudio
{
public:
    TetrisAudio();
    ~TetrisAudio();

    void initialize();
    void release();
    void reset();
    void update(const TetrisGame& game);

private:
    struct Tone
    {
        double frequency = 0.0;
        DWORD durationMs = 0;
        float gain = 0.16f;
        float overtoneRatio = 2.0f;
        float overtoneGain = 0.04f;
    };

private:
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Tone> queue_;
    bool running_ = false;
    uint64_t lastLineClearEventId_ = 0;
    uint64_t lastHardDropEventId_ = 0;
    uint64_t lastHoldEventId_ = 0;
    uint64_t lastGameOverEventId_ = 0;
    uint64_t lastLevelUpEventId_ = 0;
    uint64_t lastResetEventId_ = 0;
    bool lastPaused_ = false;

private:
    void enqueue(const std::initializer_list<Tone>& tones);
    void playTone(const Tone& tone) const;
    void audioThreadMain();
};

#endif // !TETRISAUDIO_H_
