# Thread Pool / Job Queue

## 문제

현재 Application은 메인 스레드에서 입력, Update, Render를 순서대로 처리한다. IOCP와 향후 파일 로딩을 학습하기 전에 작업 제출, Worker 대기, 공유 Queue 보호, 종료 과정을 독립적으로 확인할 기반이 필요했다.

## AI 활용

고정 Worker와 단일 Queue 방식, future 기반 결과 전달 방식의 차이를 비교했다. std::function<void()>가 인자 없는 호출 인터페이스이며 람다 캡처로 작업 데이터를 보관할 수 있다는 점을 검토했다. 소유권, 제출과 종료의 경합, 예외, Worker의 자기 자신 join 문제를 검토하고 콘솔 테스트를 작성했다.

## 내 판단

사용자는 우선 인자 없는 공통 작업을 받는 가장 단순한 구현을 선택했다. 초기에는 템플릿 Submit과 JSON 로더 없이 독립 구현했다. 이후 사용자 요청으로 독립 테스트를 제거하고 실제 JSON 시작 로딩에 연결했다. JobSystem 계약은 유지하고, 로더가 promise/future로 결과를 전달한다. 게임 객체의 병렬 갱신은 도입하지 않았다. Worker마다 역할을 고정하지 않고 같은 Queue에서 작업을 가져간다.

## 구현

### 파일과 Visual Studio 확인 위치

- `Engine/Core/JobSystem.h`: public 계약과 소유 데이터. 게임 프로젝트의 헤더 파일 → Engine → Core에 등록.
- `Engine/Core/JobSystem.cpp`: Start → Submit → WorkerLoop → Stop 흐름. 소스 파일 → Engine → Core에 등록.
- `Engine/Core/Application.cpp`: BeginGameDataLoading → Submit → FinishGameDataLoading → InitializeGame에서 실제 사용.
- `Game/Data/GameData.cpp`: Worker가 호출하는 파일 읽기·파싱·검증 함수.
- 기존 `Tests/JobSystemTests.cpp` 및 테스트 프로젝트/필터/솔루션 등록은 제거했다.
- JSON 스키마·소유권·실행 검증은 [GameData.md](GameData.md)에 기록한다.
### 함수 계약

| 함수 | 동작 |
| --- | --- |
| `Start(workerCount)` | 0개, 중복 시작, Worker 생성 실패 시 false. 부분 생성된 Worker도 회수. 종료 후 재시작 가능 |
| `Submit(job)` | 빈 함수와 미시작·종료 중 제출은 false. true는 완료가 아닌 접수 성공 |
| `Stop()` | 접수 차단, 모든 Worker 깨우기, 접수된 작업 소진, join. 반복 호출 가능 |
| `GetFailedJobCount()` | 마지막 유효한 시작 시도부터 예외를 던진 작업 수. 자동 재시도 없음 |
| 소멸자 | Stop으로 Worker 종료를 기다림 |

Start / Stop / 파괴는 JobSystem을 생성한 스레드에서 수행한다. 다른 스레드의 Start / Stop은 std::logic_error로 검출한다. Worker 내부 Stop이 던진 예외는 작업 실패로 집계되며 Pool 전체를 멈추지 않는다. 잘못된 스레드에서 소멸하면 noexcept 소멸자를 벗어나는 예외로 프로그램이 종료되므로 소유 스레드에서 파괴해야 한다.

Submit과 실패 횟수 조회는 여러 스레드에서 가능하다. Stop과 Submit이 겹치면 같은 mutex 안에서 접수 여부가 결정된다. JobSystem 파괴 이전에는 외부 제출 스레드도 종료 또는 접근 중단해야 한다. Submit의 함수 객체 구성이나 Queue 메모리 할당 실패는 호출자에게 예외로 전달될 수 있다.

### 실제 호출

Application은 Worker 하나를 시작한 뒤 `Submit([result]() { ... })`로 LoadGameData 작업을 넣는다. `result`는 shared_ptr로 소유한 promise이며 this나 GameObject 포인터를 캡처하지 않는다. 메인 스레드는 future의 `wait_for(0)`으로 준비 여부만 확인하고, 준비된 뒤 `get()`으로 값 또는 예외를 받는다. 로딩 후 Worker는 condition_variable에서 쉬며 Application 종료 시 join한다.
### 새 C++ 개념과 수명

- `std::function<void()>`: 나중에 호출할 함수를 보관한다. Queue에 있는 동안은 JobSystem, 꺼낸 뒤에는 Worker의 지역 변수가 보관한다. 캡처한 포인터의 대상까지 소유하지는 않는다.
- `std::thread`: WorkerLoop를 실행하는 스레드. Job마다 만들지 않고 Start에서 정한 수를 재사용한다.
- `lock_guard`: 범위 안에서 Queue와 접수 상태를 보호하고 범위를 나가면 잠금을 해제한다.
- `unique_lock` / `condition_variable`: 대기 중 잠금을 놓고, 깨어나면 다시 잠가 조건을 확인한다. 조건 없는 잘못된 깨어남에도 빈 Queue에 접근하지 않는다.
- `join()`: Worker가 스스로 종료할 때까지 기다린다. 강제 종료 기능이 아니며 Queue 잠금 밖에서 호출한다.
- Queue에서 꺼내는 순서는 FIFO지만 여러 Worker의 실행 시작 및 완료 순서는 보장하지 않는다.
- Job과 캡처 데이터 파괴는 Queue 잠금 밖에서 수행한다. Job 안에서 Submit을 호출해도 같은 mutex를 잡은 채 작업을 실행하는 교착이 생기지 않는다.
- JobSystem 복사는 금지한다. Worker가 빌려 쓰는 this는 모든 join이 끝날 때까지 살아 있어야 한다.

## 검증과 결과

초기 독립 구현 단계에서 Debug / Release x64의 Worker 1/2/4 × Job 10/100/1000, 동시 제출, Stop 경합, 예외 처리, 재시작, 소멸자 종료 검증을 통과했다. 이 결과는 당시 기록이며, 해당 테스트 프로젝트는 현재 제거되어 실행 지침으로 제공하지 않는다.

현재 적용 검증은 [JSON 로딩 로그](GameData.md)를 기준으로 한다. 빌드 환경의 Path/PATH 중복과 MSBuild FileTracker 샌드박스 접근 문제는 검증 프로세스에서만 우회하고 제품 프로젝트에는 환경 우회 설정을 넣지 않았다.
## 한계

- 현재 실제 사용처는 시작 시 JSON 읽기·파싱이다. 성능 향상은 측정하지 않았으며 게임 핵심 로직은 메인 스레드에서 유지한다.
- Worker 생성의 중간 실패 회수 코드는 검토했지만 실패 주입 테스트는 수행하지 않았다.
- 유한한 테스트 통과는 가능한 모든 스케줄에서의 Race Condition 부재를 증명하지 않는다.
- JobSystem 자체에는 반환값·예외 상세 전달·완료 대기 API·취소·우선순위·Queue 용량 제한이 없다. 로더는 별도의 promise/future로 결과와 상세 예외를 전달한다.
- 끝나지 않는 Job 또는 외부 스레드와의 순환 대기는 Stop도 끝나지 않게 한다.
- Job 내부 데이터 접근은 별도로 동기화해야 한다. Queue mutex가 GameWorld나 DirectX를 보호하지 않는다.
- Worker 수가 많다고 항상 빠른 것은 아니다. 작은 Job은 Queue 잠금·함수 포장·깨우기 비용이 계산보다 클 수 있다.
