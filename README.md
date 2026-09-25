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
  2. [Sprite Batch 연구와 구현](#sprite-batch)
  3. [다음 Sprint 작업](#upcoming-sprint-work)
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
Output Merger
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
DeviceContext    → Pipeline State 설정 및 Draw 명령 발행
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

- `Application`은 시스템 초기화와 Game Loop 실행 순서를 관리합니다. 현재는 일부 UI 렌더링 연결도 담당합니다.
- 게임 객체는 DirectX API와 텍스처 경로를 직접 다루지 않고 `RenderInfo`로 화면 출력 데이터를 전달합니다.
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
- 일부 UI 렌더링 연결이 아직 `Application`에 남아 있어, UI 규모가 커지면 별도 UI 렌더링 책임으로 옮길 수 있습니다.

<a id="entity-physics-collision"></a>
### 3. Entity, Physics, Collision 구조

Unity에서 접했던 `Transform`, `Physics`, 활성화 생명주기 같은 개념을 출발점으로 삼아, 엔진 기능을 그대로 가져오는 대신 현재 프로젝트에 필요한 역할과 데이터만 C++로 다시 구성했습니다. Entity 한 곳에 이동, 물리, 충돌, 렌더링 책임이 모이지 않도록 객체의 공통 상태와 동작을 나누고 필요한 구성 요소를 함께 사용합니다.

#### GameObject / Entity / Character 계층

```text
GameObject
├─ Ground
└─ Entity
   ├─ WorldItem
   └─ Character
      ├─ Player
      └─ Monster
```

- `GameObject`는 월드 Transform과 Collider를 가진 객체의 공통 기반입니다.
- 프레임마다 동작 갱신이 필요한 객체는 `Entity`, HP와 전투 동작을 공유하는 객체는 `Character`로 구분합니다.
- `GameObject`와 `Entity`는 상속으로 역할을 확장하고, Transform·Collider·Physics 상태는 필요한 객체에 구성 요소로 둡니다.

#### Vector2와 Transform

- `Vector2`는 위치, 속도, 오프셋 등 2D 값을 표현하는 최소 수학 타입입니다.
- 덧셈, 뺄셈, 스칼라 곱처럼 현재 게임에 필요한 연산만 직접 정의해 사용 범위를 작게 유지합니다.
- `Transform`은 월드 위치와 스케일을 보관합니다. 렌더링은 월드 위치를 기준으로 하고, Sprite에는 `RenderInfo`의 offset을 별도로 적용합니다.
- 현재 AABB 크기는 `Transform::scale`이 아니라 `Collider::halfSize`로 결정됩니다.

#### Physics와 Collision

- `Physics`는 `Transform`과 분리되어 Velocity, 중력 적용 여부와 지면 상태 등 이동 시뮬레이션 상태를 보관합니다.
- Player와 Monster의 행동 로직은 입력이나 AI에 따라 Velocity를 결정하고, `GameWorld::UpdatePhysics()`가 중력과 속도를 적용해 위치를 갱신합니다.
- `Collider::GetBounds()`는 Transform 위치와 Collider의 offset, halfSize를 이용해 AABB를 계산합니다. `Intersects()`는 두 AABB의 겹침 여부만 판정합니다.
- `GameWorld`가 지면·전투·아이템 충돌 대상을 순회해 `Intersects()`를 호출하고, 지면 충돌 시 위치와 Y축 Velocity를 보정합니다.

Unity에서 접한 개념을 그대로 옮기기보다, 각 개념이 해결하는 문제를 살펴보고 현재 게임에 필요한 형태로 다시 구성했습니다. 중복되던 물리 갱신을 월드에 모으고, 객체 상태는 역할별 구성 요소로 나눠 책임을 분리했습니다.

- `GameWorld`가 `unique_ptr<GameObject>`로 전체 객체의 수명을 소유합니다.
- `entities_`, `grounds_`, `monsters_`, `items_`, `player_`는 `gameObjects_`가 소유한 객체를 가리키는 비소유 포인터입니다.
- `GameWorld`가 활성 Entity를 순회하며 객체 업데이트, 물리 이동, 충돌 처리를 수행합니다.

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

#### 한계 및 개선점

- 현재 충돌 반응은 주로 Ground 착지를 대상으로 하며, 벽·천장 및 고속 이동 객체 처리는 지원하지 않습니다.
- 실제 객체 삭제 시 비소유 포인터 목록도 함께 갱신해야 합니다. 누락하면 Dangling Pointer가 발생할 수 있습니다.

<a id="character-combat-fsm"></a>
### 4. FSM 기반 Character 및 전투

이동, 점프, 공격, 피격, 사망 상태를 여러 Boolean으로 조합할 때 생기는 복잡성과 상충 상태를 줄이기 위해, 한 시점에 하나의 명시적인 Character State만 유지하도록 FSM을 구성했습니다. 몸체 충돌과 공격 범위를 별도로 조절할 수 있도록 Attack HitBox도 분리했습니다.

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

Monster와 Item을 반복 생성·삭제하는 대신 필요한 수만큼 미리 생성해 재사용합니다. 재사용 대상과 흐름이 Monster 리스폰 및 Item 획득으로 정해져 있어, 범용 Pool 클래스를 만들지 않고 Active 상태와 목적별 Queue로 관리합니다.

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

최적화 전 성능을 비교할 수 있도록 최근 프레임 평균 Profiler와 Monster 스트레스 테스트 환경을 추가했습니다.

- `Application`이 Profiler를 소유하고 `ProfileScope`가 측정 구간 종료 시 시간을 기록합니다.
- 최근 최대 120프레임의 평균을 `ProfileSnapshot`으로 전달합니다.
- `UIManager`가 기존 하트 / Game Over HUD와 F1 Profiler 패널 상태 및 버튼 입력을 관리합니다. 실제 UI 그리기는 기존 `Renderer`를 사용합니다.
- 1000 / 5000 / 10000 / 50000 스트레스 Monster를 생성하며, 기존 `gameObjects_`가 실제 수명을 소유하고 테스트 목록은 비소유 참조로 관리합니다.
- 스트레스 Monster의 Collider와 Physics를 활성화해 AABB 검사를 수행합니다. 테스트 중 피해 처리와 이동은 비활성화합니다.
- `UIRender` 시간을 Scene Render와 분리하고, Profiler 패널 자체의 Draw Call은 게임 Sprite / Draw Call 카운터에서 제외합니다.

**현재 상태:** Profiler 패널 및 100 / 500 / 1000 / 5000 / 10000 / 50000 스트레스 구간을 확인했고, 기존 및 고부하 Baseline 측정값과 스크린샷을 기록했습니다. 캡처 당시 빌드 구성과 PC 사양은 확인되지 않았습니다. 측정 조건 및 결과 표는 [Docs/ProfilerBaseline.md](Docs/ProfilerBaseline.md)에 있습니다.

Sprite Batch 전후 측정표는 [Docs/ProfilerBaseline.md](Docs/ProfilerBaseline.md)에, 기준선 및 적용 후 원본 스크린샷은 각각 [Docs/Profiler/Baseline](Docs/Profiler/Baseline/)과 [Docs/Profiler/SpriteBatchAfter](Docs/Profiler/SpriteBatchAfter/)에 보관합니다.

<a id="sprite-batch"></a>
### 2-1. Sprite Batch 연구와 구현 — 2026.09.25

#### 연구 내용

기존 경로는 Sprite마다 정점을 GPU 버퍼에 기록하고 `DrawIndexed`를 호출합니다. Sprite가 50,000개면 게임 Sprite 제출도 대략 50,000회가 되어 CPU가 GPU에 명령을 전달하는 비용이 커집니다. Sprite Batch는 렌더 요청 순서를 유지하면서 연속된 호환 Sprite의 정점을 모아 하나의 Draw Call로 제출합니다.

```text
RenderInfo
   ↓
DrawSprite: 사각형 정점 4개를 CPU 임시 목록에 추가
   ↓
텍스처 / Profiler 계측 상태가 바뀌거나 Batch 용량에 도달
   ↓
Flush: Dynamic Vertex Buffer에 누적 정점을 기록
   ↓
누적 Sprite 수 × 6 인덱스로 DrawIndexed
```

- **Batch 경계:** 현재 렌더러는 셰이더와 렌더 상태가 고정되어 있어 텍스처, Profiler 계측 여부, 최대 용량을 경계로 삼습니다. 경계가 바뀌면 앞서 모은 정점을 먼저 제출합니다.
- **순서 보존:** 요청을 전체 프레임 단위로 모아 재정렬하지 않습니다. 기존 투명 Sprite의 그리기 순서를 지키고, 인접한 호환 요청만 묶습니다.
- **Dynamic Vertex Buffer:** CPU에서 만든 정점들을 한 번에 Map(`WRITE_DISCARD`)하여 GPU 버퍼에 복사합니다. `WRITE_DISCARD`는 이전 버퍼 내용을 보존할 필요가 없는 전체 교체 방식입니다. GPU가 이전 프레임 데이터를 읽는 중이어도 드라이버가 새 저장 공간을 제공할 수 있어 덮어쓰기 동기화를 줄이는 데 도움을 줍니다.
- **용량 제한:** Batch 하나는 최대 2,048 Sprite, 8,192 Vertex를 보유합니다. 현재 `Vertex` 크기 기준 정점 버퍼 용량은 약 288 KiB입니다. 용량이 꽉 차면 Flush하고 다음 Batch를 시작합니다.
- **Index Buffer 재사용:** 사각형마다 정점 4개와 인덱스 6개를 사용합니다. 최대 용량에 맞춘 인덱스를 한 번 생성해 재사용하며, Draw 시 현재 Sprite 수에 해당하는 인덱스만 소비합니다.
- **Profiler 기준:** Sprite 수와 Draw Call 수는 큐에 넣은 시점이 아니라 실제 Batch 제출에 성공한 시점에 집계합니다. UI 패널의 자체 그리기는 게임 카운터에서 제외합니다.
- **프레임 경계:** Scene 순회 직후와 UI 렌더링 직후에 각각 Flush하여 해당 렌더 시간 구간에 제출 비용이 포함되게 합니다.

#### Render Queue와의 차이

현재 Sprite Batch는 렌더 요청이 들어오는 순서대로 누적하고, 상태가 달라질 때 제출합니다. Render Queue를 추가하면 프레임의 요청을 먼저 모은 뒤 정렬해 같은 텍스처를 더 오래 묶을 수 있습니다. 다만 투명 Sprite는 앞뒤 순서에 따라 결과가 달라지므로 임의 재정렬이 화면을 바꿀 수 있습니다. 이후 달팽이와 플레이어를 번갈아 배치하는 테스트에서 텍스처 전환이 자주 일어나면, 순서를 지키는 현재 방식의 Batch 분할 한계를 측정한 뒤 안전한 정렬 조건을 따로 설계할 수 있습니다.

#### 현재 측정 결과

아래 비교판은 기존 Baseline과 Batch 적용 후를 같은 Sprite 수 기준으로 4행에 배치했습니다. 각 칸에서 Profiler 옆에 같은 캡처의 게임 화면 일부를 두어 측정값과 실제 렌더 장면을 함께 볼 수 있습니다. 수치는 스크린샷에서 옮겼으며, 기기 사양과 두 캡처의 빌드 조건은 확인되지 않았습니다. 상세 측정표와 원본은 [Profiler Baseline 문서](Docs/ProfilerBaseline.md)를 참고하세요.

![1,000·5,000·10,000·50,000 Sprite의 Baseline 및 Sprite Batch Profiler 비교](Docs/Profiler/SpriteBatchComparison.png)

| Sprite 수 | Draw Call 전 → 후 | Scene Render 전 → 후 | 관찰 |
| ---: | ---: | ---: | --- |
| 1,000 | 1,022 → 6 | 1.00 → 1.85 ms | 호출 수는 크게 줄었지만 Render 시간은 증가 |
| 5,000 | 5,022 → 8 | 4.98 → 6.60 ms | 호출 수는 크게 줄었지만 Render 시간은 증가 |
| 10,000 | 10,022 → 10 | 8.44 → 13.57 ms | FPS 36.1 → 30.0, Frame 27.73 → 33.35 ms |
| 50,000 | 50,022 → 30 | 41.04 → 66.97 ms | FPS 7.1 → 6.0, Frame 140.21 → 166.11 ms |

Draw Call은 약 99.4~99.9% 줄었지만 Scene Render는 네 구간 모두 증가했습니다. 따라서 이번 결과는 Draw Call 감소만으로 렌더링 전체 비용이 줄어드는 것은 아님을 보여줍니다. CPU 정점 생성·목록 추가·복사 경로, Flush 횟수 및 측정 환경 차이를 후속 조사 항목으로 남깁니다. VSync가 걸리는 구간에서는 FPS가 상한에 묶일 수 있으므로 Scene Render 시간과 Draw Call을 함께 비교해야 합니다.

<a id="upcoming-sprint-work"></a>
### 3. 다음 Sprint 작업

| 예정 작업 | 학습 및 검증 목표 | 상태 |
| --- | --- | --- |
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
