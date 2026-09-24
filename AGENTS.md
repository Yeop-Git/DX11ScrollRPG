# AGENTS.md

## 프로젝트 개발 및 AI 활용 목적

이 저장소의 `README.md`에 정리된 현재 구현 내용은 포트폴리오 개발 과정에서 직접 손코딩으로 구현한 결과를 기준으로 작성했습니다.

현재까지 직접 구현한 핵심 내용은 다음과 같습니다.

- Win32 API 기반 윈도우 및 게임 루프
- DirectX 11 기반 2D 스프라이트 렌더링
- HLSL Vertex / Pixel Shader
- `Renderer`, `ResourceManager`, `Animator` 책임 분리
- `GameObject → Entity → Character` 구조
- `Transform`, `Physics`, `Collider`
- AABB 충돌 처리
- FSM 기반 플레이어 / 몬스터 행동
- 공격 프레임과 HitBox를 연결한 전투 처리
- `std::unique_ptr<GameObject>` 기반 객체 생명주기 관리
- 비소유 포인터를 활용한 시스템별 객체 참조
- Active 상태 및 Queue를 이용한 Monster / Item 재사용

2026년 9월 24일부터 10월 2일까지는 기존 코드를 기반으로 추가적인 그래픽스, 최적화, 멀티스레드, 네트워크 기능을 확장합니다.

이 단계부터는 단순히 AI에게 코드를 생성시키는 것이 아니라 **Codex와 함께 현재 코드를 분석하고, 설계안을 비교하고, 구현 결과를 직접 검증하며 AI 활용 역량을 학습하는 과정**으로 진행합니다.

이번 스프린트의 목적은 두 가지입니다.

1. 게임 클라이언트 포트폴리오를 C++ / DirectX 11 / 성능 최적화 / 네트워크까지 확장한다.
2. 개발 직군 AI 활용 역량평가를 대비해 AI가 생성한 코드와 설계를 직접 검토, 수정, 테스트할 수 있는 역량을 기른다.

모든 AI 생성 코드는 그대로 사용하는 것을 목표로 하지 않습니다.

**최종적으로 저장소에 반영되는 코드의 구조, 동작 원리, 메모리 소유권, 스레드 동기화, 렌더링 흐름, 네트워크 흐름을 직접 설명할 수 있는 수준까지 이해한 뒤 반영합니다.**

---

# 개발 원칙

## 1. 기존 코드 이해를 우선한다

Codex에게 코드를 수정시키기 전에 현재 코드의 호출 흐름과 책임을 먼저 분석합니다.

예시:

```text
현재 Renderer에서 Sprite 하나가 어떤 과정을 거쳐 DrawIndexed까지 도달하는지 분석한다.
아직 코드를 수정하지 않는다.
```

## 2. 설계안을 먼저 비교한다

바로 구현을 요청하기보다 가능한 설계안을 비교합니다.

```text
방법 A / B / C
→ 장점
→ 단점
→ 현재 프로젝트에 적합한 방식
```

최종 선택은 직접 판단합니다.

## 3. 최소 변경으로 구현한다

이번 스프린트에서는 불필요한 대규모 리팩터링을 피합니다.

기존 구조를 최대한 유지하며 필요한 기능을 작은 단위로 추가합니다.

## 4. AI 결과를 반드시 검증한다

Codex가 작성한 코드는 다음 관점에서 다시 검토합니다.

- Lifetime
- Ownership
- Null / Invalid State
- Race Condition
- Deadlock
- Buffer Overflow
- Resource Leak
- Thread Shutdown
- Network Disconnect
- Partial Packet
- Edge Case

## 5. 구현 후 직접 설명한다

각 기능을 완료한 뒤 다음 질문에 답할 수 있어야 합니다.

```text
왜 만들었는가?
기존 구조의 문제는 무엇이었는가?
왜 이 설계를 선택했는가?
다른 선택지는 무엇이 있었는가?
구현 결과는 어떻게 검증했는가?
현재 구조의 한계는 무엇인가?
```

## 6. 변경 내용을 사용자가 확인할 수 있게 공유한다

코드, 문서, 프로젝트 설정 등 저장소 파일을 수정할 때마다 작업 결과에 다음 내용을 포함합니다.

- 수정한 파일의 경로
- 변경한 코드나 설정의 핵심 내용
- 변경 위치와 호출 흐름에서 맡는 역할
- 새로 사용한 C++ / DirectX / Windows 개념의 쉬운 설명과 도입 이유
- 새로 작성하거나 수정한 코드에는 목적과 동작을 이해할 수 있는 주석을 포함
- 사용자가 Visual Studio에서 확인할 파일과 눈여겨볼 부분

사용자가 변경 내용을 직접 검토하고 이해하는 과정이 개발 목표에 포함되므로, 수정 사실만 알리고 끝내지 않습니다. 여러 파일을 바꿨다면 파일별로 변경 내용을 구분해 설명하고, 새 파일은 솔루션 및 필터에 어떻게 등록했는지도 함께 안내합니다.
주석은 코드의 목적, 데이터 흐름, 수명·소유권, 제약처럼 읽는 데 필요한 맥락을 설명하고, 문법을 그대로 되풀이하는 데 그치지 않도록 작성합니다. 수정된 동작과 주석 내용이 어긋나지 않게 함께 갱신합니다.

## 7. Profiler 소유권과 갱신 구조

Runtime Profiler는 `Application`이 소유합니다. 각 측정 지점은 해당 Profiler에 프레임별 시간과 카운터를 기록합니다. 매 프레임 갱신되는 측정 데이터의 기본 전달 방식으로 Action / event 콜백 구조를 도입하지 않습니다.

Profiler 구현 시에는 측정 대상 구간, 집계 주기, 최근 프레임 평균, 측정 오버헤드를 먼저 설명하고 설계합니다. 새로운 측정 기법이나 C++ 개념을 사용하면 사용자가 코드에서 확인할 위치와 동작 원리를 쉽게 설명합니다.

---

# AI 협업 Workflow

모든 주요 기능은 아래 순서를 기준으로 개발합니다.

```text
1. 기존 코드 분석
        ↓
2. 문제 정의
        ↓
3. AI에게 설계안 2개 이상 요청
        ↓
4. 설계 비교 및 직접 선택
        ↓
5. 최소 범위 구현 요청
        ↓
6. 코드 리뷰
        ↓
7. Edge Case 탐색
        ↓
8. 직접 테스트
        ↓
9. 성능 / 정확성 검증
        ↓
10. 직접 코드 설명 및 개발 로그 작성
```

Codex 사용 시 기본적으로 다음과 같은 요청 순서를 사용합니다.

### Analyze

```text
현재 코드를 분석하고 문제와 관련된 호출 흐름만 설명하라.
아직 코드를 수정하지 마라.
```

### Design

```text
현재 구조를 최대한 유지하는 구현 방법을 2개 이상 제시하고
각 방식의 장단점을 비교하라.
```

### Implement

```text
선택한 설계로 최소 범위만 구현하라.
관련 없는 리팩터링은 하지 마라.
```

### Review

```text
작성한 코드를 Lifetime, Race Condition, Resource Leak,
Edge Case 관점에서 다시 리뷰하라.
```

### Test

```text
정상 입력뿐 아니라 실패 가능성이 있는 테스트 케이스를 제안하고,
가능하면 검증 코드도 작성하라.
```

---

# Sprint 기간

**2026.09.24 ~ 2026.10.02**

하루 약 **4~6시간** 집중 개발을 기준으로 합니다.

이번 기간에는 DX11ScrollRPG 고도화와 AI 활용 역량 학습을 우선합니다.

---

# Sprint Goal

스프린트 종료 시 다음 구조를 목표로 합니다.

```text
[Performance]
Runtime Profiler
Stress Test
Baseline 측정

[Rendering]
Sprite Batch
Draw Call Before / After 비교

[Shader]
Offscreen Render Target
Post Processing
HP 기반 Vignette

[Concurrency]
Thread Pool
Job Queue
mutex / condition_variable

[Network]
IOCP Server
Session
Async Receive / Send
TCP Packet Framing

[Multiplayer]
2 Client 접속
Spawn / Despawn
Remote Player
Movement Synchronization

[Game Network]
Snapshot Interpolation
Server Authoritative Movement

[Collision]
Spatial Hash Broad Phase
Collision Before / After 비교
```

---

# 09.24 - Profiler / Stress Test

## 목표

최적화를 적용하기 전에 현재 성능을 Baseline으로 확보합니다.

현재 게임은 비교적 가벼운 2D 게임이므로 FPS 상승 자체를 목표로 하지 않습니다.

대신 객체 수를 의도적으로 늘리는 Stress Test 환경을 구성하여 **구조가 객체 수 증가에 따라 어떻게 Scaling되는지** 측정합니다.

## 측정 항목

```text
FPS
Frame Time

Update Time
Physics Time
Collision Time
Render Time

Entity Count
Sprite Count
Draw Call Count
Collision Check Count
```

최근 60~120 Frame 평균값도 함께 확인할 수 있도록 구성합니다.

## Stress Test

### Rendering

```text
Sprite Count

100
500
1000
5000
```

### Collision

```text
Collider / Entity Count

100
500
1000
5000
```

## 완료 기준

- Runtime 성능 측정 UI 구현
- Render Stress Test 구현
- Collision Stress Test 구현
- 현재 Renderer의 Draw Call 증가 패턴 기록
- 현재 Collision Check 증가 패턴 기록
- Baseline 데이터 저장

## AI 활용 포인트

Codex에게 먼저 다음 내용을 분석시킵니다.

```text
현재 Game Loop와 Renderer 호출 흐름을 분석하고
Profiler를 삽입하기 적절한 위치를 제안한다.

측정 코드 자체의 Overhead 가능성도 분석한다.
```

---

# 09.25 - Sprite Batch

## 현재 구조

현재 Renderer는 Sprite마다 Vertex Buffer를 갱신하고 개별 Draw Call을 수행합니다.

```text
Sprite
  ↓
Vertex Buffer Update
  ↓
Texture / Pipeline State
  ↓
DrawIndexed
```

Sprite 수가 증가하면 Draw Call도 함께 증가합니다.

## 개선 구조

```text
RenderInfo
   ↓
Render Command
   ↓
Render Queue
   ↓
Sprite Batch
   ↓
Dynamic Vertex Buffer
   ↓
DrawIndexed
```

여러 Sprite의 Vertex를 하나의 Buffer에 기록하고 가능한 Sprite들을 하나의 Draw Call로 처리합니다.

이번 스프린트에서는 Texture Atlas는 적용하지 않습니다.

현재 프로젝트의 캐릭터와 Texture 종류가 많지 않아 Atlas의 실질적인 이점보다 구현 복잡도가 더 크다고 판단합니다.

## Batch 기준

우선 다음 조건을 기준으로 Batch를 구성합니다.

```text
Texture
Shader
Render State
```

## 검증

```text
Sprite Count
100
500
1000
5000
```

각 조건에서 다음을 기록합니다.

```text
Draw Call Count
Render CPU Time
Frame Time
```

## 완료 기준

- Sprite Batch 구현
- 기존 개별 Draw 방식과 비교
- Draw Call 감소 확인
- 동일 Stress Test 조건에서 Before / After 측정

## 면접 방어 질문

- Draw Call은 무엇인가?
- Draw Call이 많으면 왜 CPU 비용이 증가하는가?
- Dynamic Vertex Buffer를 사용하는 이유는 무엇인가?
- Batch가 끊기는 조건은 무엇인가?
- Texture Atlas를 적용하지 않은 이유는 무엇인가?

---

# 09.26 - Sprite Batch 검증 / Post Processing 기반

## 오전

Sprite Batch 결과를 다시 검증합니다.

- Batch가 잘못 합쳐지는 경우 확인
- Texture 변경 시 Flush 확인
- 비활성 Entity 처리
- Sprite 0개 / 1개 처리
- Buffer Capacity 초과 상황 검토

측정 결과를 개발 로그에 기록합니다.

## 오후

Post Processing을 위한 Offscreen Rendering Pipeline을 구성합니다.

```text
Game Scene
    ↓
Offscreen Render Target
    ↓
Shader Resource View
    ↓
Fullscreen Pass
    ↓
BackBuffer
```

## 완료 기준

- Scene을 BackBuffer가 아닌 별도 Render Target에 출력
- 해당 결과를 Shader Resource로 사용
- Fullscreen Pass를 통해 BackBuffer에 출력

## 면접 방어 질문

- RTV와 SRV의 차이는 무엇인가?
- 왜 바로 BackBuffer에 렌더링하지 않는가?
- Fullscreen Pass는 왜 필요한가?
- Render Target을 Texture처럼 다시 사용할 수 있는 이유는 무엇인가?

---

# 09.27 - HP Vignette Post Processing

## 목표

플레이어 체력에 따라 화면 가장자리가 어두워지거나 붉어지는 Vignette 효과를 적용합니다.

```text
Player HP
   ↓
Health Ratio
   ↓
PostProcess Settings
   ↓
Constant Buffer
   ↓
Pixel Shader
```

## 동작 예시

```text
HP 100%
→ 효과 거의 없음

HP 50%
→ 약한 Vignette

HP 20%
→ 강한 Vignette

HP 10% 이하
→ 붉은 Vignette
```

시간이 허용되면 낮은 체력에서 Pulse 효과도 실험합니다.

## Debug

가능하면 Debug UI에서 다음 값을 조절합니다.

```text
Health Ratio
Vignette Strength
Vignette Radius
Pulse Strength
```

## 완료 기준

- Post Processing Shader 구현
- HP와 Vignette 연결
- Constant Buffer를 통한 Gameplay → Shader 데이터 전달

---

# 09.28 - Thread Pool / Job Queue

## 목표

IOCP 학습에 앞서 C++ 멀티스레드와 Producer / Consumer 구조를 직접 구현합니다.

```text
Main Thread
    ↓ Submit
Thread-safe Job Queue
    ↓
Worker Thread Pool
```

## 기본 구조

```cpp
class JobSystem
{
public:
    void Start(size_t workerCount);
    void Stop();

    void Submit(std::function<void()> job);

private:
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;

    std::mutex mutex_;
    std::condition_variable cv_;

    bool running_ = false;
};
```

## 학습 항목

- `std::thread`
- `std::mutex`
- `std::lock_guard`
- `std::unique_lock`
- `std::condition_variable`
- `notify_one`
- `wait`
- Producer / Consumer
- Race Condition
- Critical Section
- Busy Waiting
- Deadlock
- Spurious Wakeup
- `join()`
- Shutdown 처리

## 테스트

```text
10 Jobs
100 Jobs
1000 Jobs

Worker 1
Worker 2
Worker 4
```

모든 Job이 정확히 한 번 실행되는지 확인합니다.

## 완료 기준

- Thread Pool 구현
- Thread-safe Job Queue
- Worker 생성 / 종료
- Job 처리 검증

## 면접 방어 질문

- Thread Pool을 사용하는 이유는 무엇인가?
- 매 Job마다 Thread를 생성하는 방식과 어떤 차이가 있는가?
- mutex는 왜 필요한가?
- condition_variable이 없으면 어떤 문제가 생기는가?
- spurious wakeup은 무엇인가?
- Race Condition과 Deadlock의 차이는 무엇인가?

---

# 09.29 - IOCP Server

## 목표

Windows IOCP 기반 비동기 TCP 서버를 구현합니다.

이번 단계의 목표는 대규모 MMORPG 서버가 아니라 **IOCP 구조와 Session 기반 비동기 Receive / Send를 이해하는 최소 Game Server**입니다.

## 기본 구조

```text
Listen Socket
      ↓
Accept
      ↓
IOCP
      ↓
Worker Threads
      ↓
Session
      ↓
Async Recv / Send
```

## 구현 대상

- Winsock 초기화
- Listen Socket
- Client Accept
- Completion Port 생성
- Client Socket IOCP 등록
- Worker Thread
- Overlapped Receive
- Overlapped Send
- Session
- Disconnect 처리

## 첫 번째 검증

```text
Client
  ↓
"Hello"
  ↓
IOCP Server
  ↓
"Hello"
```

Echo Server 형태로 I/O 흐름을 먼저 검증합니다.

## Packet Header

Echo가 안정적으로 동작하면 Packet 구조를 도입합니다.

```cpp
struct PacketHeader
{
    uint16_t size;
    uint16_t id;
};
```

## Thread Pool과 IOCP 비교

```text
Thread Pool

Application이 Job을 Queue에 넣음
       ↓
Worker가 처리


IOCP

OS가 완료된 I/O를 Completion Queue에 넣음
       ↓
Worker가 처리
```

두 구조의 공통점과 차이를 직접 설명할 수 있어야 합니다.

## 면접 방어 질문

- Blocking Socket과 Overlapped I/O의 차이는 무엇인가?
- IOCP는 어떤 문제를 해결하는가?
- Completion Port는 무엇인가?
- `GetQueuedCompletionStatus`는 어떤 역할을 하는가?
- Session Lifetime은 어떻게 관리할 것인가?
- Disconnect 도중 Pending I/O는 어떻게 처리할 것인가?

---

# 09.30 - 2 Client Multiplayer

## 목표

IOCP 서버를 실제 DX11 게임과 연결합니다.

최소 목표는 두 개의 Client가 접속하여 서로를 Spawn하고 이동 상태를 공유하는 것입니다.

## Packet

```text
S2C_Enter
S2C_Spawn
C2S_Move
S2C_Move
S2C_Despawn
```

## Server World

초기 서버 상태는 단순하게 유지합니다.

```cpp
struct PlayerState
{
    uint32_t id;
    Vector2 position;
};
```

```text
unordered_map<PlayerId, PlayerState>
```

## Client

```text
Local Player
Remote Player A
Remote Player B
...
```

현재 GameWorld의 소유권 구조를 활용합니다.

```text
gameObjects_
→ unique_ptr<GameObject>로 실제 수명 소유

remotePlayers_
→ Remote Player 비소유 참조
```

## 완료 기준

```text
Client A 실행
Client B 실행

A 이동
→ Server
→ B에게 Broadcast
→ B 화면의 Remote A 이동

B 이동
→ Server
→ A에게 Broadcast
→ A 화면의 Remote B 이동

Client Disconnect
→ S2C_Despawn
→ Remote Player 제거
```

## 반드시 검증할 Network Edge Case

- 한 `recv`에 Packet 일부만 도착
- 여러 Packet이 한 `recv`에 함께 도착
- Client 강제 종료
- Send 중 Disconnect
- 잘못된 Packet Size
- 동일 Session에 여러 Worker 접근

---

# 10.01 - Snapshot Interpolation

## 문제

Network Packet은 Render Frame과 동일한 주기로 도착하지 않습니다.

Packet 수신 즉시 Remote Player 위치를 적용하면 움직임이 끊겨 보일 수 있습니다.

## 개선

Remote Player의 Server Snapshot을 저장한 뒤 일정 시간 과거 상태를 보간합니다.

```text
Snapshot A
     ↓
Interpolation
     ↓
Snapshot B
```

## 데이터 구조 예시

```cpp
struct Snapshot
{
    double serverTime;
    Vector2 position;
};
```

```text
deque<Snapshot>
```

## Rendering

```text
Server Time

t0
t1
t2
t3

Render Time
   ↑

t1과 t2 사이 Position을 Lerp
```

## Debug

가능하면 다음 값을 확인합니다.

```text
Snapshot Count
Interpolation Delay
Remote Position
Target Position
```

시간이 허용되면 테스트용 인위적 Delay도 추가합니다.

```text
50 ms
100 ms
150 ms
```

## 면접 방어 질문

- 왜 받은 최신 Position을 즉시 표시하지 않는가?
- 왜 과거 시점을 Render하는가?
- Interpolation과 Extrapolation의 차이는 무엇인가?
- Snapshot Buffer가 부족하면 어떻게 처리할 것인가?

---

# 10.02 오전 - Server Authoritative Movement

## 문제

초기 Multiplayer 연결 검증 단계에서는 Client가 자신의 Position을 Server에 전달하는 구조를 사용할 수 있습니다.

하지만 Client Position을 그대로 신뢰하면 Server가 실제 게임 상태를 통제하기 어렵습니다.

## 개선

Client가 Position 대신 Input을 전달하고 Server가 PlayerState를 계산합니다.

```text
Client
"오른쪽 입력"
      ↓
Server
Input 처리
Velocity / Position 계산
      ↓
PlayerState Broadcast
```

## Packet 예시

```cpp
struct C2S_MoveInput
{
    uint32_t sequence;
    float direction;
    bool jump;
};
```

## 완료 기준

- Client Position 전송 방식의 한계 정리
- Input Packet 기반 이동 요청
- Server가 Position 계산
- Server State를 Client에 Broadcast
- Remote Player는 Snapshot Interpolation으로 표시

Local Player Prediction / Reconciliation은 이번 스프린트 범위에서 제외합니다.

## 면접 방어 질문

- Client Position을 그대로 신뢰하면 어떤 문제가 있는가?
- Server Authoritative 구조를 사용하는 이유는 무엇인가?
- Input Sequence는 왜 필요한가?
- Client Prediction이 없는 경우 어떤 조작 문제가 발생할 수 있는가?

---

# 10.02 오후 - Spatial Hash Broad Phase

## 목표

현재 AABB Collision 구조 앞에 Broad Phase를 추가하여 불필요한 충돌 후보 검사를 줄입니다.

## 현재

```text
Entity
  ×
Collider 전체 후보
  ↓
AABB
```

## 개선

```text
Entity
  ↓
Spatial Hash
  ↓
Nearby Candidate
  ↓
AABB
```

## 기본 개념

World Position을 Grid Cell로 변환합니다.

```text
cellX = floor(position.x / cellSize)
cellY = floor(position.y / cellSize)
```

각 Cell에는 해당 영역과 겹치는 Collider를 등록합니다.

```text
Cell (3, 4)

Entity 12
Entity 31
Ground 7
```

## 검증

Stress Test에서 다음 항목을 비교합니다.

```text
Collision Candidate Count
AABB Check Count
Collision CPU Time
```

## 반드시 확인할 Edge Case

- Collider가 여러 Cell에 걸치는 경우
- Cell 경계에 위치한 객체
- 음수 World Coordinate
- 동적 Entity가 Cell을 이동하는 경우
- 동일 Collider가 여러 Cell에서 중복 후보로 등장
- 비활성 Entity
- Cell Size가 지나치게 크거나 작은 경우

## AI 활용

Spatial Hash 구현 전에 Codex에게 다음 방식을 비교시킵니다.

```text
Spatial Hash
Quadtree
Sweep and Prune
```

현재 2D 횡스크롤 게임 구조와 구현 복잡도를 고려해 최종 방식을 직접 선택합니다.

## 면접 방어 질문

- Broad Phase와 Narrow Phase의 차이는 무엇인가?
- 왜 Spatial Hash를 선택했는가?
- Quadtree와 비교하면 어떤 장단점이 있는가?
- Cell Size는 성능에 어떤 영향을 미치는가?
- 중복 Candidate는 어떻게 제거하는가?

---

# 개발 로그 작성 규칙

각 기능마다 다음 형식으로 짧게 기록합니다.

```text
## 기능 이름

### 문제
현재 구조에서 확인한 문제

### AI 활용
Codex에게 어떤 분석 / 설계 / 테스트를 요청했는가

### 내 판단
AI 제안 중 어떤 방식을 선택하거나 제외했는가
그 이유는 무엇인가

### 구현
실제로 변경한 구조

### 검증
어떤 테스트를 수행했는가

### 결과
실측 데이터 또는 동작 결과

### 한계
현재 구현에서 아직 남아 있는 문제
```

예시:

```text
## Sprite Batch

### 문제
Sprite 수에 따라 Draw Call이 선형적으로 증가

### AI 활용
현재 Renderer 호출 흐름 분석
Batch 방식 비교
Edge Case 탐색

### 내 판단
현재 Texture 종류가 적어 Texture Atlas는 제외
동일 Texture / Shader 기준 Batch 적용

### 검증
100 / 500 / 1000 / 5000 Sprite Stress Test

### 결과
실측 후 기록
```

---

# AI 활용 역량평가 대비 기록

매 작업 후 다음 질문에 답합니다.

```text
AI가 처음 제안한 방법은 무엇이었는가?

그중 내가 선택한 방식은 무엇인가?

선택하지 않은 방식은 무엇이며 왜 제외했는가?

AI 코드에서 발견한 문제는 무엇인가?

AI가 놓친 Edge Case는 무엇인가?

어떤 테스트로 검증했는가?

AI 없이 동일 기능을 다시 설명하거나 수정할 수 있는가?
```

AI를 잘 사용하는 것뿐 아니라 **AI의 결과를 검증하고 책임질 수 있는 개발자**가 되는 것을 목표로 합니다.

---

# Sprint 완료 기준

## Performance

- [ ] Runtime Profiler
- [ ] Rendering Stress Test
- [ ] Collision Stress Test
- [ ] Baseline 기록

## Rendering

- [ ] Sprite Batch
- [ ] Draw Call Before / After
- [ ] Render CPU Time 비교

## Shader

- [ ] Offscreen Render Target
- [ ] Fullscreen Post Process
- [ ] HP 기반 Vignette

## Concurrency

- [ ] Thread Pool
- [ ] Thread-safe Job Queue
- [ ] condition_variable 기반 Worker 대기
- [ ] 안전한 Shutdown

## Network

- [ ] IOCP Server
- [ ] Session
- [ ] Async Recv / Send
- [ ] Packet Header
- [ ] TCP Packet Framing
- [ ] Disconnect 처리

## Multiplayer

- [ ] 2 Client 접속
- [ ] Spawn
- [ ] Move
- [ ] Broadcast
- [ ] Despawn

## Network Movement

- [ ] Snapshot Buffer
- [ ] Remote Player Interpolation
- [ ] Server Authoritative Movement

## Collision

- [ ] Spatial Hash
- [ ] Broad Phase / Narrow Phase 분리
- [ ] Collision Check Before / After

---

# 이번 Sprint에서 제외하는 범위

이번 기간에는 다음 기능을 구현하지 않습니다.

```text
Texture Atlas
Bloom
Dissolve Shader
Entity Handle / Generation
Lock-free Queue
Client Prediction
Server Reconciliation
Dummy Client 대규모 부하 테스트
Monster Network Synchronization
Item Network Synchronization
DB
Login / Authentication
Room / Matchmaking
```

필요한 경우 이후 Sprint에서 확장합니다.

---

# 최종적으로 설명할 수 있어야 하는 전체 구조

```text
                   DX11 Client
                       │
          ┌────────────┴────────────┐
          │                         │
      Rendering                 Networking
          │                         │
    Render Queue                  TCP
          │                         │
    Sprite Batch                  IOCP
          │                         │
   Offscreen Target             Session
          │                         │
    Post Process             Packet Framing
          │                         │
       Vignette              Server World
                                    │
                              Authoritative
                                Movement
                                    │
                                Snapshot
                                    │
                              Interpolation


                  Game / Runtime
                       │
               Performance Profiler
                       │
              Collision / Spatial Hash
                       │
                    AABB


                  Concurrency
                       │
                  Thread Pool
                       │
                   Job Queue
                       │
               Worker Threads
```

---

# 최종 목표

이 Sprint의 목적은 기능 개수를 늘리는 것이 아닙니다.

다음 과정을 반복하는 경험을 만드는 것이 핵심입니다.

```text
문제 발견
   ↓
AI를 이용한 코드 / 구조 분석
   ↓
설계안 비교
   ↓
직접 의사결정
   ↓
AI와 함께 구현
   ↓
코드 리뷰
   ↓
Edge Case 검증
   ↓
성능 / 동작 측정
   ↓
직접 설명
```

기존 프로젝트에서 직접 구현한 C++ / DirectX 11 기반을 바탕으로 AI Agent를 **개발자를 대체하는 도구가 아니라 분석, 구현 보조, 검증을 위한 협업 도구**로 활용합니다.

스프린트가 끝났을 때는 추가된 모든 코드에 대해 기술면접에서 구조와 선택 이유를 설명할 수 있어야 합니다.
