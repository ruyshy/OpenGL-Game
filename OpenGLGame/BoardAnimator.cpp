#include "pch.h"
#include "BoardAnimator.h"

vector<TileAnimation>& BoardAnimator::animations()
{
    return _animations;
}

const vector<TileAnimation>& BoardAnimator::animations() const
{
    return _animations;
}

vector<EffectBurst>& BoardAnimator::effects()
{
    return _effects;
}

const vector<EffectBurst>& BoardAnimator::effects() const
{
    return _effects;
}

void BoardAnimator::clearAll()
{
    _animations.clear();
    _effects.clear();
}

void BoardAnimator::beginSwap(const Tile& firstTile, const Tile& secondTile, const vec2& firstPosition, const vec2& secondPosition, float duration)
{
    // 스왑은 항상 두 타일이 동시에 반대 위치로 움직이는 애니메이션 2개로 표현한다.
    _animations.clear();
    _animations.push_back({ firstTile, firstPosition, secondPosition, 0.0f, duration, false });
    _animations.push_back({ secondTile, secondPosition, firstPosition, 0.0f, duration, false });
}

void BoardAnimator::addFlash(const Tile& tile, const vec2& position, float duration)
{
    _animations.push_back({ tile, position, position, 0.0f, duration, true });
}

void BoardAnimator::addFall(const Tile& tile, const vec2& start, const vec2& end, float duration)
{
    _animations.push_back({ tile, start, end, 0.0f, duration, false });
}

void BoardAnimator::addEffect(const EffectBurst& effect)
{
    _effects.push_back(effect);
}

bool BoardAnimator::updateAnimations(double deltaTime)
{
    if (_animations.empty())
    {
        return false;
    }

    // 하나라도 끝나지 않은 애니메이션이 있으면 아직 다음 단계로 넘어가면 안 된다.
    bool allFinished = true;
    for (TileAnimation& animation : _animations)
    {
        animation.elapsed += static_cast<float>(deltaTime);
        if (animation.elapsed < animation.duration)
        {
            allFinished = false;
        }
    }

    if (!allFinished)
    {
        return false;
    }

    _animations.clear();
    // 모든 애니메이션이 끝났다는 신호를 호출부에 반환한다.
    return true;
}

void BoardAnimator::updateEffects(double deltaTime)
{
    for (EffectBurst& effect : _effects)
    {
        effect.elapsed += static_cast<float>(deltaTime);
    }

    _effects.erase(remove_if(_effects.begin(), _effects.end(), [](const EffectBurst& effect)
    {
        return effect.elapsed >= effect.duration;
    }), _effects.end());
}
