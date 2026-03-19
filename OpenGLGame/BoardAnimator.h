#pragma once

#ifndef BOARDANIMATOR_H_
#define BOARDANIMATOR_H_

#include "PuzzleTypes.h"

// 화면에 보여줄 이동/플래시/버스트 목록을 저장하고 시간을 흘려보내는 모듈이다.
class BoardAnimator
{
public:
    vector<TileAnimation>& animations();
    const vector<TileAnimation>& animations() const;
    vector<EffectBurst>& effects();
    const vector<EffectBurst>& effects() const;

    void clearAll();
    void beginSwap(const Tile& firstTile, const Tile& secondTile, const vec2& firstPosition, const vec2& secondPosition, float duration);
    void addFlash(const Tile& tile, const vec2& position, float duration);
    void addFall(const Tile& tile, const vec2& start, const vec2& end, float duration);
    void addEffect(const EffectBurst& effect);
    bool updateAnimations(double deltaTime);
    void updateEffects(double deltaTime);

private:
    vector<TileAnimation> _animations;
    vector<EffectBurst> _effects;
};

#endif // !BOARDANIMATOR_H_
