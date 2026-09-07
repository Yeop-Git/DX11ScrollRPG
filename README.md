# DirectX 11 2D Action Game

C++와 DirectX 11을 활용해 렌더링 파이프라인부터 게임 로직까지 직접 구현한 2D 액션 게임 프로토타입입니다.

![게임 플레이 데모](Docs/output.gif)

<p align="center">
  <a href="https://github.com/Yeop-Git/DX11ScrollRPG/releases/tag/v0.1.0">
    <img src="https://img.shields.io/badge/Download-v0.1.0-2ea44f?style=for-the-badge&amp;logo=github&amp;logoColor=white" alt="Download v0.1.0">
  </a>
</p>

## 프로젝트 정보

| 구분 | 내용 |
| --- | --- |
| 플랫폼 | PC (Windows) |
| 장르 | 2D 액션 게임 |
| 개발 인원 | 1명 |
| 개발 기간 | 2026.08 ~ 2026.09 |
| 기술 스택 | C++20, Win32 API, DirectX 11, HLSL, stb_image |
| 개발 목표 | 상용 엔진 없이 2D 게임 클라이언트의 렌더링 및 게임 시스템 구현 |

## 조작법

| 키 | 동작 |
| --- | --- |
| `←` / `→` | 좌우 이동 |
| `Alt` | 점프 |
| `Ctrl` | 공격 |
| `R` | 사망 후 재시작 |

## 주요 구현 내용

- Win32 API 기반 윈도우 및 게임 루프 구현
- DirectX 11 기반 2D 스프라이트 렌더링 환경 구성
- HLSL 셰이더, 텍스처, 스프라이트 시트 애니메이션 시스템 구현
- `Transform` / `Physics` / `Collider` 기반 Entity 구조 설계
- FSM 기반 플레이어·몬스터 행동 및 전투 시스템 구현
- AABB 기반 충돌 판정과 중력·이동 물리 처리
- 몬스터·아이템 스폰 및 Active 상태 기반 객체 재사용

## 1. DirectX 11 렌더링 파이프라인

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

### 구현

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

### 한계 및 개선점

- 현재는 스프라이트마다 Vertex Buffer를 갱신하고 개별 Draw Call을 사용합니다.
- 객체 수가 증가하면 Sprite Batch와 Texture Atlas 적용이 필요합니다.
- 고정 Viewport를 사용하므로 Window Resize 대응이 필요합니다.

## 2. Rendering, Resource, Animation 책임 분리

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

### 한계 및 개선점

- Animation 데이터가 Character 클래스 생성자에 직접 정의되어 있습니다.
- Animation 수가 증가하면 JSON 등의 외부 데이터나 Asset Table로 분리할 필요가 있습니다.
- 큰 `deltaTime` 입력 시 여러 Animation Frame을 한 번에 진행할 수 있도록 보완이 필요합니다.

## 3. Entity, Physics, Collision 구조

Player와 Monster에서 반복되던 Gravity, Velocity, Position 갱신을 공통화하고, 물리 위치와 화면 출력 위치를 분리했습니다.

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

### 한계 및 개선점

- 현재 충돌 처리는 Ground 착지를 가정한 단순 AABB 방식입니다.
- 벽, 천장 및 고속 이동 객체에 대한 충돌은 지원하지 않습니다.
- 동적 생성·삭제가 늘어나면 Handle 또는 ID 기반 참조 구조가 필요합니다.

## 4. FSM 기반 Character 및 전투

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

### 한계 및 개선점

- 상태가 늘어나면 하나의 `switch`가 비대해질 수 있습니다.
- 복잡한 Character에는 State Pattern 또는 별도 Controller 구조를 검토할 수 있습니다.
- 현재 공격당 하나의 Hit만 기록하므로 다중 대상 공격에는 제약이 있습니다.

## 5. Active 상태 기반 객체 재사용

Monster와 Item의 반복적인 생성·삭제에서 발생하는 메모리 할당을 줄이기 위해 필요한 수만큼 객체를 미리 생성하고 재사용합니다.

- Monster, Coin, Potion을 최대 수만큼 미리 생성합니다.
- 사용 여부는 `Active` 상태로 관리합니다.
- `OnEnable()`에서 HP, Velocity, State 등 재사용에 필요한 값을 초기화합니다.
- `OnDisable()`에서 비활성 상태를 정리합니다.
- Monster 사망 Animation 종료 후 Respawn Queue에 등록합니다.
- Item 획득 후 비활성화하고 사용 가능한 Queue에 반환합니다.

```text
Monster Respawn Queue → 사망 후 다시 활성화될 Monster 관리
Item Pool Queue       → 즉시 사용할 수 있는 비활성 Item 관리
```

현재 프로젝트 규모에서는 범용 Pool 대신 각 시스템에 필요한 객체만 재사용합니다. 대상이 증가하면 `ObjectPool<T>` 형태의 공통 구조로 확장할 수 있습니다.

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

## 에셋 크레딧

이 프로젝트의 그래픽 리소스에는 Anokolisa의 **[Legacy-Fantasy - High Forest 2.0](https://anokolisa.itch.io/sidescroller-pixelart-sprites-asset-pack-forest-16x16)** 에셋을 사용했습니다.

에셋의 저작권과 이용 조건은 원저작자의 배포 페이지를 따릅니다.

## 핵심 정리

- DirectX 11 Pipeline State와 Draw Call 구성 과정을 직접 학습했습니다.
- 게임 로직과 그래픽 API 사이의 의존성을 `RenderInfo`로 분리했습니다.
- Rendering, Resource, Animation의 변경 책임을 분리했습니다.
- 객체 역할에 따라 `GameObject → Entity → Character` 계층을 구성했습니다.
- Behavior, Physics, Collision의 이동 책임을 분리했습니다.
- FSM과 Animation Frame을 연결해 전투 판정을 구현했습니다.
- Active State와 Lifecycle Hook을 이용해 객체를 재사용했습니다.

> 범용 게임 엔진 제작보다 C++ 객체 설계와 DirectX 11 렌더링 흐름을 직접 경험하는 데 초점을 둔 프로젝트입니다.
