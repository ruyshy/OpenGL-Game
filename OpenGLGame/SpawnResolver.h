#pragma once

#ifndef SPAWNRESOLVER_H_
#define SPAWNRESOLVER_H_

#include "PuzzleTypes.h"

// 초기 보드 생성과 리필 타일 생성 책임을 분리한 모듈이다.
class SpawnResolver
{
public:
    static void buildInitialBoard(PuzzleGrid& tiles, mt19937& rng);
    static Tile randomRefillTile(mt19937& rng);
};

#endif // !SPAWNRESOLVER_H_
