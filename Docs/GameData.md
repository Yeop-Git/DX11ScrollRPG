# JSON 설정과 JobSystem 파일 로딩

작성: 2026.09.26. AI Sprint 3-1에서 실제 시작 경로에 적용했다.

## 문제

Player와 Monster 생성자가 AnimationClip과 이동·피격 설정을 직접 채우고, ResourceManager가 텍스처 경로를 나열했다. 값을 바꾸려면 코드를 수정해야 했고 Monster마다 동일한 클립 정의를 보관했다. 기존 JobSystem은 독립 테스트에서만 사용됐다.

## AI 활용과 판단

사용자는 독립 테스트를 제거하고 Job Queue를 파일 I/O의 실제 사용처에 연결하도록 요청했다. 객체별 JSON 읽기/복사와 시작 시 한 번 읽는 공유 정의를 비교해 후자를 적용했다. Pool과 Stress Monster도 동일한 정의를 참조하므로 중복 읽기와 클립 정의 복사를 피한다.

반복 완료 메시지를 위한 mutex Queue와 요청 한 건의 promise/future를 비교했다. 현재는 시작 시 로딩 한 건이므로 promise/future를 사용한다. JobSystem의 공통 인터페이스는 여전히 `Submit(std::function<void()>)`다. 템플릿 Submit이나 std::async로 실행 경로를 대체하지 않았다.

## 구현과 호출 흐름

```text
Main: Window / DirectX 초기화 → JobSystem.Start(1) → Submit(void() 람다)
Worker: LoadGameData → 파일 읽기 → JSON 파싱 → 검증 → promise에 값 또는 예외
Main: 메시지 처리 / 로딩 화면 → future.wait_for(0) → 준비되면 get()
    → 불변 GameData 소유 → ResourceManager → Renderer / UI → GameWorld
    → 기존 입력 / Update / Render
종료: JobSystem 파괴·join → 나머지 멤버 파괴 → World 이후 GameData 파괴
```

`promise`는 결과를 전달하는 쪽, `future`는 그 결과를 받는 쪽이다. `wait_for(0)`은 완료 여부만 확인한다. `get()`은 준비된 값 또는 Worker가 저장한 예외를 받으며 한 번만 호출한다. Job이 캡처하는 shared_ptr는 promise의 수명을 보장한다. Worker는 Application의 this, Player, Monster, D3D 객체를 캡처하지 않는다.

Worker는 C++의 일반 동기 파일 읽기를 실행한다. 메인 스레드 관점에서 비동기이며, OS 비동기 파일 I/O나 IOCP 구현은 아니다. 로딩이 끝나면 Worker는 condition_variable에서 대기하고 Application 종료 시 join한다. 파일마다 Pool을 종료하지 않는다.

## 변경 파일과 Visual Studio 확인 위치

| 파일 | 역할과 확인할 부분 |
| --- | --- |
| `Assets/Data/GameData.json` | Player 6개 / Monster 4개 클립, 캐릭터 수치, 17개 SpriteId의 텍스처 경로. 리소스 파일 → Data |
| `Game/Data/GameData.h` | 정적 데이터 구조. 현재 HP·재생 프레임·FSM은 포함하지 않음. 헤더 파일 → Game → Data |
| `Game/Data/GameData.cpp` | LoadGameData, 필수 키·타입·수치·FSM loop 계약 검증. 소스 파일 → Game → Data |
| `Engine/Core/Application.h/.cpp` | BeginGameDataLoading / FinishGameDataLoading / InitializeGame, 설정과 Worker의 파괴 순서 |
| `Game/Player.h/.cpp` | PlayerDefinition 참조, 생성·상태 전환·공격 프레임·이동·피격 설정 적용 |
| `Game/Monster.h/.cpp` | MonsterDefinition 참조, 생성·상태 전환·추적·피격·풀 재활성화 설정 적용 |
| `Game/World/GameWorld.h/.cpp` | 일반·풀·Stress 객체 생성 시 같은 설정 주입 |
| `Engine/Graphics/ResourceManager.h/.cpp` | JSON 경로 목록을 받아 메인 스레드에서 PNG 디코딩·D3D 생성 |
| `Engine/ThirdParty/nlohmann/json.hpp`, `LICENSE.MIT` | 검증된 JSON 파서와 배포 라이선스. 헤더 파일 → Engine → ThirdParty |
| `DX11ScrollRPG.vcxproj/.filters` | 새 소스·헤더·JSON·라이선스 등록 |
| `DX11ScrollRPG.slnx`, `Tests/JobSystemTests.*` | 이전 독립 테스트 등록 및 파일 제거. 솔루션에는 게임 프로젝트만 남음 |
| `README.md`, `AGENTS.md`, `Docs/SprintWorkPlan.md`, `Docs/Requirements.md`, `Docs/ThreadPool.md` | JSON 적용을 09.26 현재 작업으로 변경, 테스트 전용 설명 갱신 |

Animator가 보관하는 현재 클립 복사본과 시간·현재 프레임은 재생 상태로 유지한다. 모든 Monster의 전체 정의를 복사하지 않는다. 설정은 Application이 `unique_ptr<const GameData>`로 소유하며 GameWorld보다 먼저 선언해 더 늦게 파괴한다. 실행 중 교체하지 않으므로 Player/Monster의 const 참조가 무효화되지 않는다.

## JSON 편집 계약

저장소 루트를 작업 디렉터리로 실행한다. 기존 Assets/Textures와 Shaders도 동일한 상대 경로 규칙을 사용한다. JSON을 저장하고 게임을 다시 실행하면 재컴파일 없이 설정을 다시 읽는다.

- `schemaVersion`: 현재 1만 지원한다. 필수 키를 생략해 C++ 기본값으로 조용히 대체하지 않는다.
- `animations`: 프레임 수, 프레임 간격(초), loop, 픽셀 단위 프레임 크기, 월드 렌더 반크기, offset이다. `renderHalfSize.x = 0`은 기존 Renderer의 비율 계산 규칙을 유지한다.
- Player: HP, 충돌 반크기, 시작 위치, 렌더 Y 보정, 이동/점프 속도, 무적·넉백·점멸 간격, 공격 프레임·범위다.
- Monster: HP, 충돌 반크기, 시작 위치, 렌더 Y 보정, 추적 속도·범위, 넉백 속도다.
- `attackFirstFrame` / `attackLastFrame`: 0부터 시작하는 양끝 포함 프레임 번호. 공격 클립 범위 안에 있어야 한다.
- `textures`: 알려진 SpriteId 17개를 모두 정의한다. 알 수 없는 이름이나 누락을 거부한다. 상태→SpriteId 대응은 코드에 유지한다.
- 현재 FSM의 종료 처리를 보존하기 위해 Idle/Run/Chase는 반복, JumpStart/JumpEnd/Attack/Hurt/Dead는 비반복으로 검증한다.
- 파일은 1 MiB 이하이며, 수치의 타입·유한성·범위를 검사한다. 구체적인 편집 가능 범위는 GameData.cpp에 명시했다.

필수 JSON 실패는 파일 경로·오류를 표시하고 시작을 중단한다. 텍스처 실패도 해당 경로를 표시한다. 완료되기 전에는 게임 입력·월드 갱신을 시작하지 않는다. 부분 데이터를 기존 월드에 덮어쓰는 경로는 없다.

## 검증과 결과

- Debug / Release x64 게임 빌드 성공. 초기 빌드에서 발견한 GameWorld::Initialize 선언/정의 불일치를 수정한 뒤 통과했다.
- 실제 로더를 연결한 임시 검증에서 기존 10개 클립의 프레임 수·간격·loop·크기·offset이 이전 코드와 일치했다.
- 17개 텍스처 경로의 파일 존재, Player/Monster 초기화, 공격 유효 프레임, Monster 피격·풀 재활성화를 확인했다.
- 임시 JSON의 HP·클립 간격을 바꿔 실제 캐릭터와 Animator에 반영되는 것을 확인했다.
- 누락 키, 타입 오류, 잘못된 프레임·시간·loop·공격 범위, 과도한 수치, 벡터 길이, 텍스처 이름·경로, 버전, 문법, 대형/없는 파일과 비동기 오류 전달을 포함한 18건을 확인했다.
- 이 과정에서 JSON 생성 스크립트가 음수 offset을 문자열로 기록한 문제를 실제 로더가 검출했다. 숫자로 수정한 후 전체 검증을 통과했다.
- 실제 JobSystem을 통한 성공/예외 결과와 Stop 이후 결과 회수를 확인했다. 임시 검증 코드는 Git 제외 대상 Build/Verification에만 두고 솔루션 테스트 프로젝트를 재추가하지 않았다.
- Debug 게임을 숨김 실행해 로딩 완료 후 창 제목 전환을 확인했고, WM_CLOSE로 정상 종료(코드 0)했다. 이는 기동 확인이며 화면 육안 검증을 대신하지 않는다.

## 한계와 직접 확인할 부분

- 게임 화면의 애니메이션·입력·전투 육안 회귀 확인은 미실행이다. 기존 10개 클립 수치는 보존했지만 최종 화면 확인과 구분한다.
- JSON 변경은 재시작 시 적용된다. 핫 리로드·취소·재요청·월드 전환 및 오래된 결과 무효화는 아직 없다. 이를 추가할 때 설정 참조 수명과 요청 ID 정책을 먼저 정한다.
- PNG 파일 읽기·디코딩·GPU 생성은 메인 스레드다. 이번 Worker 적용 범위는 JSON 읽기·파싱·검증이다.
- 맵 배치·스폰·아이템·UI 레이아웃·물리 알고리즘 상수는 기존 코드에 유지한다. 맵 ID/배치/Spawn 스키마는 후속이다.
- 스프라이트 시트의 실제 픽셀 배치와 JSON 메타데이터의 일치까지 자동 검증하지 않는다. 값을 바꾸면 에셋과 화면을 함께 확인해야 한다.
- Worker 강제 취소는 없다. 로딩 중 종료하면 진행 중인 파일 읽기가 끝날 때까지 join한다. 느린 디스크의 종료 대기 시간은 측정하지 않았다.
- 작은 설정 파일이므로 속도 향상을 주장하지 않는다. 데이터 분리와 스레드 간 결과 전달의 실제 적용이 이번 목적이다.

## 파서 출처

[nlohmann/json v3.12.0 공식 릴리스](https://github.com/nlohmann/json/releases/tag/v3.12.0)의 단일 헤더를 수정 없이 사용한다. MIT 라이선스를 함께 포함했다. JSON의 문자열·숫자·이스케이프·오류 처리를 직접 구현하지 않기 위해 도입했다.

공식 json.hpp SHA-256과 다운로드 파일을 대조했다: `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`.
