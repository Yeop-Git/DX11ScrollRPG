# DirectX 11 2D Action Game

C++와 DirectX 11로 만든 2D 액션 게임 프로토타입입니다. 기반 게임 시스템은 직접 구현하며 학습했고, 2026년 9월 24일부터는 AI Sprint를 통해 성능 측정, 렌더링, 멀티스레드, 네트워크 기능을 AI와 함께 분석·설계·구현하고 있습니다.

## 목차

- [프로젝트 정보](#project-information)
- [조작법](#controls)
- [기본: 직접 구현하며 학습한 기반](#handmade-foundation)
  1. [DirectX 11 렌더링 파이프라인](#directx-rendering-pipeline)
  2. [Rendering, Resource, Animation 책임 분리](#system-responsibilities)
  3. [Entity, Physics, Collision 구조](#entity-physics-collision)
  4. [FSM 기반 Character 및 전투](#character-combat-fsm)
  5. [Active 상태 기반 객체 재사용](#object-reuse)
- [AI Sprint: AI와 함께 분석·설계·구현](#ai-sprint)
  1. [Profiler / Stress Test](#profiler-stress-test)
  2. [다음 Sprint 작업](#upcoming-sprint-work)
- [빌드 및 실행](#build-and-run)
- [프로젝트 구조](#project-structure)
- [에셋 크레딧](#asset-credits)
- [핵심 정리](#summary)

![게임 플레이 데모](Docs/output.gif)

<p align="center">
  <a href="https://github.com/Yeop-Git/DX11ScrollRPG/releases/tag/v0.1.0">
    <img src="https://img.shields.io/badge/Download-v0.1.0-2ea44f?style=for-the-badge&amp;logo=github&amp;logoColor=white" alt="Download v0.1.0">
  </a>
</p>

<a id="project-information"></a>
## 프로젝트 정보

| 구분 | 내용 |
| --- | --- |
| 플랫폼 | PC (Windows) |
| 장르 | 2D 액션 게임 |
| 개발 인원 | 1명 |
| 개발 기간 | 2026.08 ~ 2026.10 (AI Sprint 진행 중) |
| 기술 스택 | C++20, Win32 API, DirectX 11, HLSL, stb_image |
| 개발 목표 | 상용 엔진 없이 2D 게임 클라이언트의 렌더링 및 게임 시스템 구현 |

<a id="controls"></a>
## 조작법

| 키 | 동작 |
| --- | --- |
| `←` / `→` | 좌우 이동 |
| `Alt` | 점프 |
| `Ctrl` | 공격 |
| `R` | 사망 후 재시작 |
| `F1` | Profiler 패널 열기 / 닫기 및 Stress Test 버튼 표시 |

<a id="handmade-foundation"></a>
## 기본: 직접 구현하며 학습한 기반

- Win32 API 기반 윈도우 및 게임 루프 구현
- DirectX 11 기반 2D 스프라이트 렌더링 환경 구성
- HLSL 셰이더, 텍스처, 스프라이트 시트 애니메이션 시스템 구현
- `Transform` / `Physics` / `Collider` 기반 Entity 구조 설계
- FSM 기반 플레이어·몬스터 행동 및 전투 시스템 구현
- AABB 기반 충돌 판정과 중력·이동 물리 처리
- 몬스터·아이템 스폰 및 Active 상태 기반 객체 재사용

<a id="directx-rendering-pipeline"></a>
### 1. DirectX 11 렌더링 파이프라인

Unity와 같은 엔진 내부에서 처리되는 렌더링 과정을 직접 구현하며, 스프라이트 한 장을 출력하는 데 필요한 GPU Resource와 Pipeline State를 단계별로 구성했습니다.

```text
Vertex / Index Buffer
        ↓
Input Assembler
        ↓
Vertex Shader
        ↓
Rasterizer
        ↓
Pixel Shader + Texture
        ↓
Render Target
        ↓
SwapChain Present
```

#### 구현

- `D3D11CreateDeviceAndSwapChain`을 이용한 Device, DeviceContext, SwapChain 생성
- Back Buffer 기반 RenderTargetView 및 Viewport 구성
- Vertex Buffer, Index Buffer와 Input Layout 연결
- HLSL Vertex Shader 및 Pixel Shader 작성
- ShaderResourceView와 SamplerState를 이용한 Texture 출력
- Alpha Blending을 적용한 PNG 투명도 처리
- `ComPtr`를 이용한 DirectX COM Resource 수명 관리
- Point Sampling을 이용한 픽셀 아트 출력
- 스프라이트 시트 UV 계산, 프레임 애니메이션 및 좌우 반전

```text
Device           → GPU Resource 생성
DeviceContext    → Pipeline State 설정 및 Draw 명령 실행
RenderTargetView → 렌더링 결과 출력 대상
DrawIndexed      → 현재 Pipeline State를 이용한 Draw Call
```

#### 한계 및 개선점

- 현재는 스프라이트마다 Vertex Buffer를 갱신하고 개별 Draw Call을 사용합니다.
- 객체 수가 증가하면 Sprite Batch와 Texture Atlas 적용이 필요합니다.
- 고정 Viewport를 사용하므로 Window Resize 대응이 필요합니다.

<a id="system-responsibilities"></a>
### 2. Rendering, Resource, Animation 책임 분리

초기에는 `Application`이 윈도우 생성, 텍스처 관리, 게임 객체 갱신, 렌더링까지 대부분의 기능을 담당했습니다. 기능 증가에 따라 각 시스템의 변경 책임을 분리하고, 게임 로직이 DirectX 객체와 텍스처 경로를 직접 참조하지 않도록 구조를 개선했습니다.

```text
Application      → 초기화 및 Game Loop
GameWorld        → GameObject 및 게임 로직 관리
Renderer         → Pipeline State 설정 및 Draw Call
ResourceManager  → Texture 생성, 보관 및 조회
Animator         → Animation Frame 진행 및 종료 상태 관리
```

- `Application`은 시스템 초기화와 실행 순서만 관리합니다.
- 게임 객체는 `RenderInfo`를 통해 화면 출력에 필요한 데이터만 전달합니다.
- `Renderer`는 `RenderInfo`를 실제 DirectX Draw Call로 변환합니다.
- `ResourceManager`가 Texture와 ShaderResourceView의 생명주기를 관리합니다.
- `AnimationClip`은 프레임 수, 재생 시간, 스프라이트 크기, Offset을 보관합니다.
- `Animator`가 프레임 진행, Loop 여부 및 종료 상태를 공통 처리합니다.

```cpp
struct RenderInfo
{
    SpriteId spriteId = SpriteId::None;
    Vector2 position;
    Vector2 renderHalfSize;
    Vector2 offset;
    int frame = 0;
    int frameCount = 1;
    bool flipX = false;
};
```

#### 한계 및 개선점

- Animation 데이터가 Character 클래스 생성자에 직접 정의되어 있습니다.
- Animation 수가 증가하면 JSON 등의 외부 데이터나 Asset Table로 분리할 필요가 있습니다.
- 큰 `deltaTime` 입력 시 여러 Animation Frame을 한 번에 진행할 수 있도록 보완이 필요합니다.

<a id="entity-physics-collision"></a>
### 3. Entity, Physics, Collision 구조

Unity에서 접했던 `Transform`, `Physics`, 활성화 생명주기 같은 개념을 출발점으로 삼아, 엔진 기능을 그대로 가져오는 대신 현재 프로젝트에 필요한 역할과 데이터만 C++로 다시 구성했습니다. Entity 한 곳에 이동, 물리, 충돌, 렌더링 책임이 모이지 않도록 객체의 공통 상태와 동작을 나누고 필요한 구성 요소를 함께 사용합니다.

#### Vector2와 Transform

- `Vector2`는 위치, 속도, 방향 계산과 렌더링 좌표에 공통으로 사용하는 최소 2D 수학 타입입니다.
- 덧셈, 뺄셈, 스칼라 곱처럼 현재 게임에 필요한 연산만 직접 정의해 사용 범위를 작게 유지합니다.
- `Transform`은 월드 위치와 크기를 보관합니다. Collider는 Transform의 위치를 기준으로 충돌 영역을 계산하고, Renderer로 전달되는 `RenderInfo`도 같은 위치를 사용합니다.

#### Physics와 이동 흐름

- `Physics`는 `Transform`과 분리되어 Velocity, Gravity 사용 여부, 지면 상태 등 물리 시뮬레이션에 필요한 값을 보관합니다.
- Player와 Monster의 행동 로직은 이동 의도를 Velocity에 반영하고, `GameWorld::UpdatePhysics()`가 중력과 속도를 적용해 Transform 위치를 갱신합니다.
- 초기에는 객체별 업데이트에서 위치까지 직접 변경했지만, 이를 분리해 행동 결정과 공통 물리 갱신의 책임을 구분했습니다. 충돌 처리에서는 보정된 위치와 Velocity를 함께 갱신합니다.

이 구조를 만든 과정은 Unity에서 익숙했던 개념을 단순히 흉내 내는 데 그치지 않고, 각 개념이 어떤 문제를 해결하는지 살펴보는 학습 과정이기도 했습니다. 상태와 기능을 역할별로 분리하고 객체를 구성 요소의 조합으로 설계하는 이유를 직접 확인하면서, 현재 게임 규모에 필요한 범위와 엔진이 제공하는 범용 기능의 차이를 배웠습니다.

```text
GameObject
├─ Ground
└─ Entity
   ├─ WorldItem
   └─ Character
      ├─ Player
      └─ Monster
```

- `GameWorld`가 `unique_ptr<GameObject>`로 전체 객체의 수명을 소유합니다.
- Physics와 Collision 시스템은 필요한 객체를 비소유 포인터로 참조합니다.

```text
GameWorld
  │
  └─ gameObjects_
       ├─ unique_ptr<Player> ────────┐
       ├─ unique_ptr<Monster> ───────┼── 실제 객체 소유
       ├─ unique_ptr<Ground> ────────┤
       └─ unique_ptr<WorldItem> ─────┘

entities_ ────── Entity*
monsters_ ───── Monster*
grounds_ ────── Ground*
items_ ──────── WorldItem*
player_ ─────── Player*

↑ 모두 gameObjects_ 내부 객체를 참조만 함
```

- Player와 Monster는 입력 또는 AI에 따라 Velocity만 결정합니다.
- `GameWorld::UpdatePhysics()`에서 Gravity와 Position 갱신을 공통 처리합니다.
- AABB 충돌 판정을 Ground, Combat, Item 시스템에 공통 사용합니다.
- Ground 충돌 시 Entity를 지면 위로 보정하고 Y축 Velocity를 초기화합니다.

```text
Transform Position → 실제 객체 위치
Collider Offset    → 충돌 판정 위치
Render Offset      → Sprite 출력 위치
Animation Offset   → Animation별 Sprite 위치 보정
```

#### 한계 및 개선점

- 현재 충돌 처리는 Ground 착지를 가정한 단순 AABB 방식입니다.
- 벽, 천장 및 고속 이동 객체에 대한 충돌은 지원하지 않습니다.
- 동적 생성·삭제가 늘어나면 Handle 또는 ID 기반 참조 구조가 필요합니다.

<a id="character-combat-fsm"></a>
### 4. FSM 기반 Character 및 전투

여러 Boolean 조합 대신 한 시점에 하나의 명시적인 Character State만 유지하도록 FSM을 구성했습니다.

```text
Player  → Idle / Run / JumpStart / JumpEnd / Attack / Dead
Monster → Idle / Chase / Hurt / Dead
```

- Grounded 상태와 Velocity에 따라 Player 이동 Animation을 전환합니다.
- Attack과 Dead 상태에서는 일반 입력 및 상태 전환을 제한합니다.
- Monster는 Player와의 거리에 따라 Idle과 Chase 상태를 전환합니다.
- Body Collider와 별도의 Attack HitBox를 사용합니다.
- 실제 공격 프레임에서만 HitBox를 활성화합니다.
- 공격 한 번당 Damage가 한 번만 적용되도록 중복 Hit를 방지합니다.
- Player 피격 시 무적 시간, 점멸 및 Knockback을 적용합니다.

```cpp
bool Player::IsAttackFrameActive() const
{
    if (state_ != PlayerState::Attack)
        return false;

    const int frame = animator_.GetCurrentFrame();
    return frame >= 2 && frame <= 3;
}
```

#### 한계 및 개선점

- 상태가 늘어나면 하나의 `switch`가 비대해질 수 있습니다.
- 복잡한 Character에는 State Pattern 또는 별도 Controller 구조를 검토할 수 있습니다.
- 현재 공격당 하나의 Hit만 기록하므로 다중 대상 공격에는 제약이 있습니다.

<a id="object-reuse"></a>
### 5. Active 상태 기반 객체 재사용

Monster와 Item의 반복적인 생성·삭제에서 발생하는 메모리 할당을 줄이기 위해 필요한 수만큼 객체를 미리 생성하고 재사용합니다.

- `GameObject::SetActive()`가 상태가 실제로 바뀌는 경우에만 `OnEnable()` 또는 `OnDisable()`을 호출합니다.
- Monster, Coin, Potion을 최대 수만큼 미리 생성합니다.
- 사용 여부는 `Active` 상태로 관리합니다.
- `OnEnable()`에서 HP, Velocity, State 등 재사용에 필요한 상태를 초기화합니다.
- `OnDisable()`에서 전투, 애니메이션, 물리 상태를 정리해 이전 사용의 값이 다음 활성화에 남지 않도록 합니다.
- Monster 사망 Animation 종료 후 Respawn Queue에 등록합니다.
- Item 획득 후 비활성화하고 사용 가능한 Queue에 반환합니다.

이 활성화 생명주기도 Unity에서 익숙했던 `OnEnable` / `OnDisable` 개념을 참고해 직접 정의했습니다. 재사용할 때 초기화와 정리 시점을 명시해 각 객체가 풀링에 필요한 동작을 책임지게 하고, `GameWorld`와 시스템은 객체의 실제 수명 소유와 활성 상태에 따른 사용을 구분합니다.

```text
Monster Respawn Queue → 사망 후 다시 활성화될 Monster 관리
Item Pool Queue       → 즉시 사용할 수 있는 비활성 Item 관리
```

현재 프로젝트 규모에서는 범용 Pool 대신 각 시스템에 필요한 객체만 재사용합니다. 대상이 증가하면 `ObjectPool<T>` 형태의 공통 구조로 확장할 수 있습니다.

<a id="ai-sprint"></a>
## AI Sprint: AI와 함께 분석·설계·구현

2026년 9월 24일부터 10월 2일까지 기존 코드를 바탕으로 성능 최적화, 렌더링, 동시성, 네트워크 기능을 확장합니다. AI가 작성한 결과를 그대로 반영하지 않고, 호출 흐름 분석 → 설계 비교 → 최소 구현 → 코드 리뷰 → 직접 검증 순서로 진행합니다.

이 구역에는 AI와 함께 공부하며 구현한 내용, 선택한 설계, 검증 상태를 기록합니다. 아직 시작하지 않은 Sprint 항목은 구현 완료 내용과 구분해 예정 작업으로 표시합니다.

<a id="profiler-stress-test"></a>
### 1. Profiler / Stress Test — 2026.09.24 ~ 09.25

최적화 전 성능을 비교할 수 있도록 최근 프레임 평균 Profiler와 Monster 스트레스 테스트 환경을 추가하고 있습니다.

- `Application`이 Profiler를 소유하고 `ProfileScope`가 측정 구간 종료 시 시간을 기록합니다.
- 최근 최대 120프레임의 평균을 `ProfileSnapshot`으로 전달합니다.
- `UIManager`가 기존 하트 / Game Over HUD와 F1 Profiler 패널 상태 및 버튼 입력을 관리합니다. 실제 UI 그리기는 기존 `Renderer`를 사용합니다.
- 100 / 500 / 1000 / 5000 스트레스 Monster를 생성하며, 기존 `gameObjects_`가 실제 수명을 소유하고 테스트 목록은 비소유 참조로 관리합니다.
- 스트레스 Monster의 Collider와 Physics를 활성화해 AABB 검사를 수행합니다. 테스트 중 피해 처리와 이동은 비활성화합니다.
- `UIRender` 시간을 Scene Render와 분리하고, Profiler 패널 자체의 Draw Call은 게임 Sprite / Draw Call 카운터에서 제외합니다.

**현재 상태:** Profiler 패널이 표시되는 것을 확인했습니다. 스트레스 개체 수별 동작 검증과 Release x64 Baseline 실측값 기록은 남아 있습니다. 측정 조건 및 결과 표는 [Docs/ProfilerBaseline.md](Docs/ProfilerBaseline.md)에 있습니다.

<a id="upcoming-sprint-work"></a>
### 2. 다음 Sprint 작업

| 예정 작업 | 학습 및 검증 목표 | 상태 |
| --- | --- | --- |
| 09.25 Sprite Batch | Sprite별 Draw Call과 Batch 적용 후 비용 비교 | 예정 |
| 09.26 Offscreen Render Target / Post Processing | RTV·SRV 흐름과 Fullscreen Pass 이해 | 예정 |
| 09.27 HP Vignette | Gameplay 값을 Constant Buffer와 Pixel Shader로 전달 | 예정 |
| 09.28 Thread Pool / Job Queue | mutex, condition_variable, Worker 종료 흐름 검증 | 예정 |
| 09.29 IOCP Server / Packet Framing | Overlapped I/O, Session 수명, TCP 부분 패킷 처리 | 예정 |
| 09.30 2 Client Multiplayer | Spawn, 이동 동기화, Disconnect 처리 | 예정 |
| 10.01 ~ 10.02 Snapshot Interpolation / Server Authority | Remote 상태 보간과 Server 입력 기반 이동 비교 | 예정 |
| 10.02 Spatial Hash | Broad Phase 적용 전후 AABB 검사 수 비교 | 예정 |

AI Sprint의 세부 일정, 완료 기준과 AI 활용 평가 질문은 [AGENTS.md](AGENTS.md)에 정리했습니다.

<a id="build-and-run"></a>
## 빌드 및 실행

### 요구 사항

- Windows 10 이상
- Visual Studio 및 MSVC v145 Platform Toolset
- Windows 10 SDK
- DirectX 11 지원 GPU

### 실행 방법

1. 저장소를 복제합니다.

   ```bash
   git clone https://github.com/Yeop-Git/DX11ScrollRPG.git
   cd DX11ScrollRPG
   ```

2. Visual Studio에서 `DX11ScrollRPG.slnx`를 엽니다.
3. `x64`와 `Debug` 또는 `Release` 구성을 선택합니다.
4. 프로젝트를 빌드한 뒤 실행합니다.

셰이더와 텍스처는 상대 경로로 불러오므로, 실행 파일을 직접 실행할 때는 저장소 루트를 작업 디렉터리로 사용해야 합니다.

<a id="project-structure"></a>
## 프로젝트 구조

```text
DX11ScrollRPG/
├─ Assets/                 # 게임 텍스처
├─ Engine/
│  ├─ Core/                # Application, Window, Game Loop
│  ├─ Graphics/            # Renderer, ResourceManager, RenderInfo
│  ├─ Math/                # Vector2
│  └─ ThirdParty/          # stb_image
├─ Game/
│  ├─ Animation/           # AnimationClip, Animator
│  ├─ Collision/           # AABB
│  ├─ Entity/              # GameObject, Entity, Character, Physics
│  ├─ World/               # GameWorld, Ground, WorldItem
│  ├─ Player.cpp
│  └─ Monster.cpp
├─ Shaders/                # HLSL Vertex / Pixel Shader
└─ Main.cpp                # Windows 애플리케이션 진입점
```

<a id="asset-credits"></a>
## 에셋 크레딧

이 프로젝트의 그래픽 리소스에는 Anokolisa의 **[Legacy-Fantasy - High Forest 2.0](https://anokolisa.itch.io/sidescroller-pixelart-sprites-asset-pack-forest-16x16)** 에셋을 사용했습니다.

에셋의 저작권과 이용 조건은 원저작자의 배포 페이지를 따릅니다.

<a id="summary"></a>
## 핵심 정리

- DirectX 11 Pipeline State와 Draw Call 구성 과정을 직접 학습했습니다.
- 게임 로직과 그래픽 API 사이의 의존성을 `RenderInfo`로 분리했습니다.
- Rendering, Resource, Animation의 변경 책임을 분리했습니다.
- 객체 역할에 따라 `GameObject → Entity → Character` 계층을 구성했습니다.
- Behavior, Physics, Collision의 이동 책임을 분리했습니다.
- FSM과 Animation Frame을 연결해 전투 판정을 구현했습니다.
- Active State와 Lifecycle Hook을 이용해 객체를 재사용했습니다.

> 범용 게임 엔진 제작보다 C++ 객체 설계와 DirectX 11 렌더링 흐름을 직접 경험하는 데 초점을 둔 프로젝트입니다.
