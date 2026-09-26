# DirectX 11 2D Action Game

C++20과 DirectX 11로 개발한 2D 액션 게임 프로토타입. Win32 게임 루프와 렌더링 파이프라인부터 게임 시스템까지 직접 구현.

![게임 플레이 데모](Docs/output.gif)

<p align="center">
  <a href="https://github.com/Yeop-Git/DX11ScrollRPG/releases/tag/v0.1.0">
    <img src="https://img.shields.io/badge/Download-v0.1.0-2ea44f?style=for-the-badge&amp;logo=github&amp;logoColor=white" alt="Download v0.1.0">
  </a>
</p>

`v0.1.0`: 아래 **기본: 직접 작성한 코드**의 렌더링·게임 시스템을 포함한 릴리스. AI Sprint는 해당 버전 이후의 확장 개발이며 `v0.1.0` 배포본에는 미포함.

## 목차

- [프로젝트 정보](#project-information)
- [조작법](#controls)
- [기본: 직접 작성한 코드](#handwritten-foundation)
  1. [DirectX 11 렌더링](#directx-rendering)
  2. [렌더링·리소스·애니메이션 구조](#system-architecture)
  3. [객체·물리·충돌](#entity-physics-collision)
  4. [FSM 기반 전투](#fsm-combat)
  5. [Active 상태 기반 재사용](#object-reuse)
- [AI Sprint: 성능 최적화·멀티스레드·데이터 분리](#ai-sprint)
  1. [Profiler와 스트레스 테스트](#profiler-stress-test)
  2. [Sprite Batch](#sprite-batch)
  3. [Depth Test와 픽셀 처리량](#depth-test)
  4. [Render Queue](#render-queue)
  5. [Thread Pool / Job Queue](#thread-pool)
  6. [JSON 설정과 비동기 로딩](#json-loading)
- [Sprint 진행 일정](#sprint-schedule)
- [빌드 및 실행](#build-and-run)
- [프로젝트 구조](#project-structure)
- [에셋 크레딧](#asset-credits)

<a id="project-information"></a>
## 프로젝트 정보

| 구분 | 내용 |
| --- | --- |
| 플랫폼 | Windows PC |
| 장르 | 2D 액션 게임 |
| 개발 인원 | 1명 |
| 개발 기간 | 2026.08 ~ 2026.10 |
| 기술 | C++20, Win32 API, DirectX 11, HLSL, stb_image, nlohmann/json |

<a id="controls"></a>
## 조작법

| 키 | 동작 |
| --- | --- |
| `←` / `→` | 좌우 이동 |
| `Alt` | 점프 |
| `Ctrl` | 공격 |
| `R` | 사망 후 재시작 |
| `F1` | Profiler 및 스트레스 테스트 패널 표시 전환 |

<a id="handwritten-foundation"></a>
## 기본: 직접 작성한 코드

아래 1~5번 작업은 [v0.1.0 릴리스](https://github.com/Yeop-Git/DX11ScrollRPG/releases/tag/v0.1.0)에 포함. Win32 / DirectX 11 기반 렌더링, 객체·물리·충돌, FSM 전투, Monster / Item 재사용 구현.

<a id="directx-rendering"></a>
### 1. DirectX 11 렌더링

#### 개발 배경

게임 엔진이 처리하는 그래픽스 파이프라인과 GPU 리소스의 역할을 익히기 위해 스프라이트 렌더링 과정을 직접 구성.

#### 구현 내용

- Win32 윈도우 및 Direct3D 11 Device, Device Context, Swap Chain 초기화
- Vertex / Index Buffer, Input Layout, HLSL Vertex / Pixel Shader 연결
- Shader Resource View와 Sampler를 이용한 텍스처 및 스프라이트 시트 출력
- Point Sampling 기반 픽셀 아트 출력, 프레임 UV 계산 및 좌우 반전
- `ComPtr` 기반 DirectX COM 리소스 수명 관리

#### 한계 및 개선점

- 창 크기 변경에 따른 Render Target 및 Viewport 재설정 필요

<a id="system-architecture"></a>
### 2. 렌더링·리소스·애니메이션 구조

#### 개발 배경

초기 `Application`에 모여 있던 초기화·리소스·갱신·렌더링 책임을 분리하고, 게임 로직에서 DirectX API와 텍스처 경로에 대한 의존성을 제거.

#### 구현 내용

- `Application`: 시스템 초기화와 게임 루프 관리
- `GameWorld`: 게임 객체와 월드 갱신 관리
- `RenderInfo`: 게임 객체에서 Renderer로 전달하는 스프라이트 출력 정보
- `Renderer`: 렌더 정보 기반 DirectX 렌더 명령 처리
- `ResourceManager`: 텍스처와 Shader Resource View 보관 및 수명 관리
- `AnimationClip`: 프레임 수, 재생 시간, 스프라이트 크기, Offset 보관
- `Animator`: 프레임 진행, Loop 및 재생 종료 처리

#### 한계 및 개선점

- 당시 클래스 내부 애니메이션 정의 → AI Sprint에서 JSON 분리 적용
- 당시 Application의 UI 책임 → AI Sprint에서 UIManager 분리 적용

<a id="entity-physics-collision"></a>
### 3. 객체·물리·충돌

#### 개발 배경

Player와 Monster 사이 중복된 이동·중력·충돌 처리를 정리하고, 객체 역할과 물리·충돌·수명 관리 책임을 분리.

#### 구현 내용

- `GameObject → Entity → Character` 계층으로 공통 객체·갱신 대상·전투 캐릭터 구분
- `Transform`: 위치와 스케일 보관
- `Physics`: 속도, 중력 및 지면 상태 보관
- `Collider`: 충돌 크기와 오프셋 기반 AABB 계산
- `GameWorld`: Physics 갱신, AABB 검사 및 지면 충돌 보정
- `gameObjects_`: `unique_ptr<GameObject>`를 이용한 객체 수명 소유
- 시스템별 목록: 소유 객체를 가리키는 비소유 포인터 보관

#### 한계 및 개선점

- 지면 착지 중심의 충돌 반응. 벽·천장 및 고속 이동 충돌 처리는 미지원
- 객체 삭제 시 시스템별 비소유 참조 목록 갱신 필요
- 객체 증가에 대비한 Broad Phase 도입 필요

<a id="fsm-combat"></a>
### 4. FSM 기반 전투

#### 개발 배경

여러 Boolean 조합에서 발생하는 상충 상태를 방지하고, 몸체 충돌과 공격 판정을 독립적으로 관리.

#### 구현 내용

- Player: Idle, Run, Jump, Attack, Dead 상태
- Monster: Idle, Chase, Hurt, Dead 상태
- Grounded 및 Velocity 기반 Player 이동 애니메이션 전환
- 공격 애니메이션의 유효 프레임에 한정한 HitBox 활성화
- 공격당 대상별 중복 피격 방지
- Player 피격 시 무적 시간, 점멸 및 Knockback 적용

#### 한계 및 개선점

- 상태 증가에 대비한 상태별 동작 또는 Controller 분리 검토
- 현재 공격당 한 대상만 피격 처리. 다중 대상 공격 지원 필요

<a id="object-reuse"></a>
### 5. Active 상태 기반 재사용

#### 개발 배경

Monster와 Item의 반복적인 생성·삭제로 발생하는 메모리 할당을 줄이고, 재사용 시 초기화와 정리 시점을 명확히 구분.

#### 구현 내용

- Monster와 Item 사전 생성 및 `Active` 상태 기반 사용 관리
- `OnEnable()`: HP, Velocity, State 초기화
- `OnDisable()`: 전투·애니메이션·물리 상태 정리
- 사망 Monster의 Respawn Queue 등록
- 획득 Item의 재사용 Queue 반환

#### 한계 및 개선점

- Monster와 Item 흐름에 맞춘 개별 Pool 관리
- 재사용 대상 증가 시 공통 `ObjectPool<T>` 구조 검토

<a id="ai-sprint"></a>
## AI Sprint: 성능 최적화·멀티스레드·데이터 분리

직접 구현한 C++ / DirectX 11 게임을 기반으로 Codex와 설계 비교·구현·검증을 진행한 확장 개발.

- 성능 분석·렌더링: Profiler, Stress Test, Sprite Batch, Depth Test, Render Queue
- 멀티스레드·데이터 분리: Thread Pool, Job Queue, AnimationClip JSON, 비동기 파일 로딩
- 후속 목표: QWER·Pool·Handle, IOCP·멀티플레이, Spatial Hash. Offscreen·Post Processing은 후순위

<a id="profiler-stress-test"></a>
### 1. Profiler와 스트레스 테스트

#### 개발 배경

객체 수가 늘어날 때 프레임 비용이 집중되는 구간을 확인하고, 렌더링 최적화 전후를 비교할 기준선 마련.

#### 구현 내용

- `Application` 소유 Profiler 및 최근 최대 120프레임 평균 기록
- Frame, Update, Physics, Collision, Scene Render, UI Render, Present 측정
- Entity, Sprite, Draw Call, AABB 검사 수 표시
- 1,000 / 5,000 / 10,000 / 50,000 Stress Monster 생성
- GPU 시간과 Pixel Shader 호출 수 수집
- F1 패널에서 Depth Test와 스트레스 테스트 조건 제어

#### 한계 및 개선점

- GPU 시간이 1ms 미만인 구간은 측정 편차의 영향이 큼
- 상세 조건과 기준선은 [Profiler 측정 기록](Docs/ProfilerBaseline.md) 참고

<a id="sprite-batch"></a>
### 2. Sprite Batch

#### 개발 배경

Sprite별 Draw Call에서 발생하는 CPU 제출 비용을 줄이면서 기존 렌더 순서를 보존.

#### 구현 내용

- `DrawSprite`에서 사각형 정점 4개를 CPU 목록에 추가
- 텍스처, Profiler 계측 상태 변경 또는 2,048 Sprite 용량 도달 시 Flush
- Dynamic Vertex Buffer를 `WRITE_DISCARD`로 Map해 정점 복사
- 재사용 인덱스 버퍼로 누적 Sprite를 DrawIndexed 처리
- 실제 Batch 제출 성공 시 Sprite 및 Draw Call 카운터 갱신

| Stress Sprite | Draw Calls 전 → 후 | Render CPU (ms, 전 → 후) |
| ---: | ---: | ---: |
| 1,000 | 1,022 → 6 | 1.00 → 1.85 |
| 5,000 | 5,022 → 8 | 4.98 → 6.60 |
| 10,000 | 10,022 → 10 | 8.44 → 13.57 |
| 50,000 | 50,022 → 30 | 41.04 → 66.97 |

![Sprite Batch 적용 전후 Profiler 비교. 테두리는 Draw Call 값을 표시한다.](Docs/Profiler/SpriteBatchComparison.png)

#### 한계 및 개선점

- Draw Call은 약 99% 이상 감소했지만 모든 측정 구간에서 Scene Render CPU 시간 증가
- 정점 계산, CPU 목록 작성·복사 및 Batch 분할 비용 추가 분석 필요
- 현재는 렌더 순서를 유지하기 위해 연속된 유사 요청만 Batch로 처리
- 서로 다른 텍스처 요청이 번갈아 나타나면 같은 텍스처를 사용하는 Sprite가 많더라도 Batch가 자주 끊겨 효율 감소

<a id="depth-test"></a>
### 3. Depth Test와 픽셀 처리량

#### 개발 배경

깊이 버퍼가 가려진 영역의 픽셀 셰이더 처리를 줄이는지 GPU 시간과 셰이더 호출 수로 확인.

#### 구현 내용

- 월드 Sprite에 Depth Test 적용 및 Profiler 패널에서 전환
- GPU 시간과 Pixel Shader 호출 수 수집
- 50,000 Stress Sprite에서 ON/OFF 측정

![Depth Test ON/OFF 비교. 테두리는 Render, Draw Calls, GPU 시간과 PS 호출 수를 표시한다. 두 번째 행은 5,000 버튼 선택 당시 실제 Stress Sprite가 500개로 측정되어 별도 표기했다.](Docs/Profiler/DepthTest/DepthTestComparison.png)

| Stress Sprite | GPU Time (ms, ON → OFF) | PS Invocations (ON → OFF) | Render CPU (ms, ON → OFF) |
| ---: | ---: | ---: | ---: |
| 1,000 | 0.36 → 0.28 | 761,804 → 8,902,340 | 1.39 → 2.16 |
| 5,000 버튼 선택 (실제 500) | 0.58 → 1.25 | 1,675,407 → 38,882,099 | 6.93 → 8.97 |
| 10,000 | 0.74 → 2.27 | 2,063,266 → 78,192,054 | 13.74 → 18.00 |
| 50,000 | 1.67 → 11.46 | 29,449,766 → 376,943,225 | 67.88 → 89.77 |

#### 한계 및 개선점

- 50,000 구간에서 Depth Test ON의 GPU 시간 약 85% 감소, Pixel Shader 호출 수 약 92% 감소
- Render CPU 병목은 남아 있어 추가 최적화 필요

<a id="render-queue"></a>
### 4. Render Queue

#### 개발 배경

Player와 Monster 스프라이트 요청이 번갈아 들어오는 조건에서 연속 Batch만 사용할 때와 Queue로 텍스처별 요청을 모을 때의 Draw Call 및 CPU Render 비용을 비교.

#### 구현 내용

- 렌더 요청을 레이어와 텍스처 기준으로 모은 뒤, 레이어 순서에 따라 Queue를 제출
- 같은 텍스처 묶음 안의 Sprite 순서는 유지하고, 깊이 테스트가 켜진 Cutout 요청만 같은 레이어 안에서 텍스처별로 모음
- Queue 제출 시 기존 Sprite Batch를 사용하고, 투명 Sprite는 Queue 대상에서 제외
- Player와 Monster Sprite를 교차 배치한 렌더 전용 스트레스 테스트 추가
- Batch, Queue, Depth Test 및 테스트 패턴을 Profiler에서 전환
- Profiler에 CPU와 GPU 측정값을 구분해 표시

![Render Queue 적용 전후 비교. 왼쪽은 Queue OFF, 오른쪽은 Queue ON이며 노란 테두리는 Render, Queue, Draw Calls, GPU Time, PS 값을 표시한다. Profiler 화면은 원본 캡처에서 가져왔다.](Docs/Profiler/RenderQueue/RenderQueueComparison.png)

| Sprite 수 | Draw Calls (OFF → ON) | Render CPU (ms, OFF → ON) | GPU Time (ms, OFF → ON) |
| ---: | ---: | ---: | ---: |
| 1,000 | 1,002 → 4 | 16.64 → 2.53 | 0.49 → 0.13 |
| 5,000 | 5,002 → 6 | 16.77 → 8.75 | 7.69 → 0.31 |
| 10,000 | 10,002 → 8 | 32.84 → 16.73 | 17.46 → 0.12 |
| 50,000 | 50,002 → 28 | 105.80 → 85.71 | 93.48 → 0.61 |

#### 한계 및 개선점

- 50,000 Sprite에서 Draw Calls 50,002 → 28 감소. Render CPU는 85.71 ms로 남아 정점 준비 비용 개선 필요
- GPU 시간 감소 확인. 픽셀 처리량 감소를 의미하지는 않음
- 투명 Sprite와 Depth Test OFF에서는 순서 보존을 위해 요청 재정렬 제외

<a id="thread-pool"></a>
### 5. Thread Pool / Job Queue

#### 개발 배경

Worker를 재사용해 작업 실행·대기·종료를 관리하고, 게임 시작 시 파일 로딩을 분리할 기반 마련.

#### 구현 내용

- `std::function<void()>` 작업을 보관하는 공통 Queue와 고정 Worker 구성
- `mutex` 기반 Queue 보호, `condition_variable` 기반 Worker 대기
- Queue 잠금 밖에서 작업 실행, 작업 예외 집계 후 후속 작업 처리
- Stop 시 새 접수 차단, 접수된 작업 처리 후 Worker join
- Application에서 Worker 하나를 시작해 JSON 읽기·파싱·검증에 사용

#### 한계 및 개선점

- 작업 강제 취소·우선순위·Queue 용량 제한 미지원
- 게임 객체 갱신과 렌더링은 메인 스레드에서 유지
- 함수 계약·수명·검증 기록은 [Thread Pool 개발 로그](Docs/ThreadPool.md) 참고

<a id="json-loading"></a>
### 6. JSON 설정과 비동기 로딩

#### 개발 배경

Player / Monster 생성자의 정적 정의를 [GameData.json](Assets/Data/GameData.json)으로 분리. JSON 수정 후 재실행으로 재컴파일 없이 설정 적용.

#### 구현 내용

| 분리한 데이터 | 내용 |
| --- | --- |
| AnimationClip 10개 | 프레임 수·재생 간격·반복 여부·프레임 크기·렌더 크기·offset |
| Player / Monster 설정 | 초기 HP·시작 위치·충돌 크기·이동 및 피격 수치·공격 유효 프레임과 범위 |
| 텍스처 경로 17개 | SpriteId별 이미지 파일 경로 |

```text
Application → JobSystem.Submit
    → Worker: 파일 읽기 → JSON 파싱·검증 → promise에 결과 또는 예외 저장
    → Main: future 완료 확인 → 리소스 생성 → GameWorld 초기화
```

- Application 소유 불변 GameData를 Player / Monster가 읽기 전용으로 공유
- 현재 HP·FSM·Animator 재생 상태는 객체별로 유지
- 로딩 중 창 메시지 처리, 필수 설정 오류 시 메시지 표시 후 시작 중단
- nlohmann/json 3.12.0 사용 및 [MIT 라이선스](Engine/ThirdParty/nlohmann/LICENSE.MIT) 포함

#### 검증 결과

- Debug / Release x64 빌드 성공
- 기존 10개 클립 값 보존 및 JSON 수정값의 캐릭터·Animator 반영 확인
- 캐릭터 초기화·공격 프레임·풀 재활성화·오류 18건 검증 통과
- Worker 결과·예외 전달·종료 처리 및 Debug 게임 기동·정상 종료 확인

#### 한계 및 개선점

- 시작 시 1회 로딩, 핫 리로드 미지원
- PNG 디코딩·D3D 생성은 메인 스레드에서 수행
- 화면 육안 회귀 검증·성능 개선 측정 미실행
- 스키마·소유권·검증 범위는 [JSON 로딩 개발 로그](Docs/GameData.md) 참고

<a id="sprint-schedule"></a>
## Sprint 진행 일정

2026.09.26 기준, 하루 4~6시간의 목표 일정. 의존 관계·이월 기준은 [작업 계획](Docs/SprintWorkPlan.md), 기능별 계약·검증 기준은 [요구사항 명세](Docs/Requirements.md) 참고.

| 날짜 / 상태 | 작업 | 완료 목표 |
| --- | --- | --- |
| 기존 구현·측정 자료 기록 | Profiler / Stress Test, Sprite Batch, Depth Test, Render Queue | 기존 결과와 한계 보존 |
| 09.26 · 구현·자동 검증 | Thread Pool / Job Queue | 시작 시 JSON 로딩에 사용. 사용자 육안 검토는 별도 |
| 09.26 · 구현·자동 검증 | AnimationClip JSON·비동기 로딩 | 클립·캐릭터 설정·텍스처 경로를 외부화, Worker 로딩 후 메인 스레드 초기화 |
| 09.27 목표 | 공격 구조·Projectile Pool·Q | Ctrl 공격 유지, Q 투사체와 3초 쿨타임 |
| 09.28 목표 | W/E/R·Effect Pool·Handle / Lookup | 5/7/30초 쿨타임, 명중 효과, Pool 반환·재사용 시 이전 Handle 무효화 |
| 09.29 목표 | IOCP Echo Server | Session·비동기 Receive/Send·Disconnect·안전한 종료 |
| 09.30 목표 | Packet Framing·2 Client | 부분/병합 패킷, Spawn/Despawn·기본 이동 동기화 |
| 10.01 목표 | Interpolation·서버 권위 이동 | 원격 이동 보간, 서버가 입력으로 위치 계산 |
| 10.02 목표 | 통합 검증·기록, 여유 시 Spatial Hash | 검증·기록 시간을 우선 확보하고 Broad Phase 전후 비교 |
| 10.02 이후 후순위 | Offscreen·Post Processing·HP Vignette | 네트워크·충돌 후속 작업 이후 진행 |

- QWER 쿨타임: 3 / 5 / 7 / 30초. 후보 에셋: Fire_Ball / Wind / Earth_Spike / Tornado, Q 명중 효과: Explosion
- Foozle ZIP: 효과 64×64, 아이콘 32×32, CC0. 게임 등록·공격 세부 설계는 후속
- Projectile / HitEffect는 Pool 사용. Handle / Lookup / Generation은 재사용 시 참조 유효성 검증 목적
- 네트워크 상태 반영은 메인 스레드에서 처리 예정. IOCP 완료 Queue와 일반 Job Queue 구분
- Sprite Batch / Render Queue 경계 조건 검증 기록 제외. Depth 5,000 버튼의 실제 500개 표기 유지, 재측정 제외
- 일정 지연 시 Spatial Hash 우선 이월, 10.02 통합 검증 시간 확보. 맵 ID·배치·Spawn JSON은 장기 확장

<a id="build-and-run"></a>
## 빌드 및 실행

### 요구 사항

- Windows 10 이상
- Visual Studio 및 MSVC v145 Platform Toolset
- Windows 10 SDK
- DirectX 11 지원 GPU

### 실행

1. 저장소 복제

   ```bash
   git clone https://github.com/Yeop-Git/DX11ScrollRPG.git
   cd DX11ScrollRPG
   ```

2. Visual Studio에서 `DX11ScrollRPG.slnx` 열기
3. `x64`와 `Debug` 또는 `Release` 구성 선택 후 빌드 및 실행

셰이더·텍스처·JSON이 상대 경로를 사용하므로 작업 디렉터리는 저장소 루트로 설정. `Assets/Data/GameData.json` 필수 포함.

<a id="project-structure"></a>
## 프로젝트 구조

```text
DX11ScrollRPG/
├─ Assets/                 # Data/GameData.json, 게임 텍스처
├─ Engine/
│  ├─ Core/                # Application, JobSystem, Game Loop, Profiler, UI
│  ├─ Graphics/            # Renderer, ResourceManager, RenderInfo
│  ├─ Math/                # Vector2
│  └─ ThirdParty/          # stb_image, nlohmann/json 및 라이선스
├─ Game/
│  ├─ Animation/           # AnimationClip, Animator
│  ├─ Collision/           # AABB
│  ├─ Data/                # GameData 정의와 JSON 로더
│  ├─ Entity/              # GameObject, Entity, Character, Physics
│  ├─ World/               # GameWorld, Ground, WorldItem
│  ├─ Player.cpp
│  └─ Monster.cpp
└─ Shaders/                # HLSL Vertex / Pixel Shader
```

<a id="asset-credits"></a>
## 에셋 크레딧

Anokolisa의 [Legacy-Fantasy - High Forest 2.0](https://anokolisa.itch.io/sidescroller-pixelart-sprites-asset-pack-forest-16x16) 그래픽 리소스 사용. 저작권 및 이용 조건은 원저작자 배포 페이지 기준.
