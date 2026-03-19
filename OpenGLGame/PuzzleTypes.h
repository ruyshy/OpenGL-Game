#pragma once

#ifndef PUZZLETYPES_H_
#define PUZZLETYPES_H_

constexpr int PuzzleColumnCount = 8;
constexpr int PuzzleRowCount = 8;
constexpr int PuzzleTileTypeCount = 5;
constexpr float PuzzleRoundTimeSeconds = 60.0f;

// 보드 위의 한 칸을 가리키는 가장 기본적인 좌표 구조체다.
// column은 x축, row는 y축으로 사용하며 렌더링과 로직 모두 같은 기준을 쓴다.
struct Cell
{
    int column = -1;
    int row = -1;

    bool operator==(const Cell& other) const = default;
};

enum class SpecialType
{
    None,
    RowClear,
    Bomb,
    ColorBomb
};

// 퍼즐 보드에 저장되는 실제 타일 데이터다.
// color는 일반 색상 종류를, special은 특수 블록의 동작 방식을 나타낸다.
struct Tile
{
    int color = -1;
    SpecialType special = SpecialType::None;
};

// 스왑, 제거, 낙하처럼 화면에 움직임이 있어야 할 때 사용하는 애니메이션 단위다.
// flash가 true이면 이동 대신 제자리에서 사라지는 연출로 취급한다.
struct TileAnimation
{
    Tile tile{};
    vec2 start = vec2(0.0f);
    vec2 end = vec2(0.0f);
    float elapsed = 0.0f;
    float duration = 0.0f;
    bool flash = false;
};

// 특수 블록이 발동할 때 잠깐 번쩍이는 시각 효과를 표현한다.
struct EffectBurst
{
    vec2 center = vec2(0.0f);
    vec2 size = vec2(0.0f);
    vec4 color = vec4(1.0f);
    float elapsed = 0.0f;
    float duration = 0.0f;
};

// 매치가 끝난 뒤 특정 위치에 새 특수 블록을 생성해야 할 때 사용하는 예약 정보다.
struct SpecialSpawn
{
    Cell cell{};
    Tile tile{};
};

// 한 번의 매치 판정 결과를 담는다.
// cells는 제거 대상, spawns는 새로 생성될 특수 블록, triggeredSpecials는
// "이번 제거에서 일반 블록을 임시 특수 블록처럼 취급"해야 하는 경우를 표현한다.
struct MatchResolution
{
    vector<Cell> cells;
    vector<SpecialSpawn> spawns;
    vector<SpecialSpawn> triggeredSpecials;
};

// 실제 제거 단계에서 특수 블록 연쇄를 모두 확장한 최종 결과다.
struct ExpandedClearResult
{
    vector<Cell> clearedCells;
    vector<SpecialSpawn> activatedSpecials;
};

// 보드가 현재 어떤 단계에 있는지를 나타내는 상태 열거형이다.
// 입력 가능 여부와 애니메이션 완료 후 다음 처리 흐름이 이 값으로 결정된다.
enum class BoardState
{
    WaitingForInput,
    AnimatingSwap,
    AnimatingInvalidSwapForward,
    AnimatingInvalidSwapBack,
    AnimatingClear,
    AnimatingFall,
    TimeUp
};

// 퍼즐 보드 전체를 저장하는 8x8 고정 배열이다.
using PuzzleGrid = array<array<Tile, PuzzleColumnCount>, PuzzleRowCount>;

#endif // !PUZZLETYPES_H_
