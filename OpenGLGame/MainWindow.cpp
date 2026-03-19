#include "pch.h"
#include "MainWindow.h"

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
}

void MainWindow::initializeScene()
{
    // 이 프로젝트는 2D 스프라이트 기반 테트리스를 그리므로 깊이 테스트와 컬링은 끄고,
    // 알파 블렌딩만 활성화해 블록/패널/오버레이가 의도한 순서대로 합성되게 만든다.
    glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // 렌더러, 오디오, 게임 상태를 한 번에 초기화해서 첫 프레임부터
    // "화면", "입력 가능한 상태", "사운드 이벤트 상태"가 서로 어긋나지 않게 맞춘다.
    renderer_.initialize();
    audio_.initialize();
    renderer_.onWindowSizeChanged(*this);
    game_.reset();
    audio_.reset();
    inputController_.reset();
    updateWindowTitle();
}

void MainWindow::renderScene()
{
    // 실제 게임 화면 구성은 TetrisRenderer가 담당하고,
    // MainWindow는 프레임 시작 시 버퍼를 비우는 역할만 맡는다.
    glClear(GL_COLOR_BUFFER_BIT);
    renderer_.render(*this, game_);
}

void MainWindow::updateScene()
{
    // 입력 컨트롤러가 false를 반환하는 경우는 창 닫기처럼
    // 이후 게임 업데이트가 더 이상 의미 없는 상황이다.
    if (!inputController_.process(*this, game_))
    {
        return;
    }

    // 한 프레임 안에서 게임 규칙 -> 연출 갱신 -> 오디오 반영 순서로 처리해
    // 같은 이벤트를 보고 화면과 소리가 동시에 반응하도록 유지한다.
    game_.update(getTimeDelta());
    renderer_.update(*this, game_, getTimeDelta());
    audio_.update(game_);
    updateWindowTitle();
}

void MainWindow::releaseScene()
{
    audio_.release();
    renderer_.release();
}

void MainWindow::onWindowSizeChanged(int width, int height)
{
    renderer_.onWindowSizeChanged(*this);
    updateWindowTitle();
}

void MainWindow::onMouseButtonPressed(int button, int action)
{
}

void MainWindow::updateWindowTitle() const
{
    // 창 제목은 디버그 HUD 성격도 겸하므로 현재 게임 상태를 빠르게 읽을 수 있게
    // 점수, 라인 수, 레벨, 조작 힌트를 함께 표시한다.
    std::ostringstream stream;
    stream << "OpenGL Tetris"
           << " | Score: " << game_.getScore()
           << " | Lines: " << game_.getLinesCleared()
           << " | Level: " << game_.getLevel();

    if (game_.isGameOver())
    {
        stream << " | Game Over (R to restart)";
    }
    else if (game_.isPaused())
    {
        stream << " | Paused (P to resume)";
    }
    else
    {
        stream << " | Arrows move, Up/Space rotate, Shift hold, C drop, P pause";
    }

    if (getWindow() != nullptr)
    {
        glfwSetWindowTitle(getWindow(), stream.str().c_str());
    }
}
