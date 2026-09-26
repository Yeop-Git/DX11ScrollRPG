# AI Sprint 작업 계획

작성 기준: 2026.09.26. Sprint 기간: 2026.09.24~10.02, 하루 4~6시간.

이 문서는 대화에서 결정한 범위와 날짜별 실행 목표를 정리한다. 일정은 목표이며 기능 완료를 보장하거나 이미 수행한 것으로 표시하지 않는다. 기능별 계약과 검증 기준은 [요구사항 명세](Requirements.md), 실제 Thread Pool 검증 결과는 [개발 로그](ThreadPool.md)를 기준으로 한다.

## 1. 현재 상태

| 구분 | 확인된 상태 | 남은 일 |
| --- | --- | --- |
| Profiler / Stress Test / Sprite Batch / Depth Test / Render Queue | 구현 및 비교 자료 존재 | 기존 측정 결과와 한계 보존 |
| Thread Pool / Job Queue | 실제 시작 시 JSON 로딩에 연결, 이전 독립 테스트 프로젝트 제거 | 사용자 코드 검토·직접 실행·커밋 |
| QWER / Projectile / Effect Pool / Handle | 계획 및 에셋 분석 단계 | 설계 선택·구현·검증 |
| AnimationClip JSON / 비동기 로딩 | 09.26 우선 적용, Worker 로딩·메인 스레드 초기화 구현 | [로딩 로그](GameData.md)의 검증과 한계 확인 |
| IOCP / Multiplayer / Interpolation / 서버 권위 이동 / Spatial Hash | 미구현 | 아래 일정에 따라 진행 |
| Offscreen / Post Processing / HP Vignette | 후순위 | 10.02 이후 일정으로 이월 |

Thread Pool의 테스트 통과는 게임 성능 향상이나 게임 내 비동기 로딩 완료를 뜻하지 않는다. 현재 변경 사항은 커밋 완료로 간주하지 않는다.

## 2. 확정된 범위와 제외 사항

- 기존 Ctrl 일반 공격을 유지하고 Q / W / E / R 스킬을 추가한다. 쿨타임은 각각 3 / 5 / 7 / 30초다.
- Projectile과 HitEffect는 사전 생성한 Pool을 사용한다. 동적 삭제 사례를 만들기 위해 Pool을 없애지 않는다.
- Handle / Lookup / Generation은 포트폴리오에서 참조 유효성·재사용을 설명하고 검증하기 위해 구현한다. 현재 구조에서 필수라는 주장과 구분한다.
- 인자 없는 `std::function<void()>` 기반 JobSystem을 우선 사용한다. 템플릿 Submit은 도입하지 않는다. 로더만 promise/future를 사용해 시작 요청 한 건의 결과를 전달한다.
- 입력·GameWorld 갱신·Pool 조작·DX11 Immediate Context 접근은 현재 클라이언트의 메인 스레드에 둔다.
- JSON 읽기·파싱·검증은 JobSystem으로 분리하고, 완료 결과 적용은 메인 스레드에서 수행한다.
- IOCP는 OS의 완료 Queue를 사용한다. 자체 Job Queue로 IOCP를 대체하지 않는다.
- Sprite Batch / Render Queue 경계 조건 검증 기록은 이번 일정에서 제외한다. 이 제외는 새 스킬·Handle·네트워크 기능의 정확성 검증을 제외한다는 의미가 아니다.
- Depth 비교의 5,000 버튼 캡처 재측정은 후속 과제로 요구하지 않는다. 기존 실제 500개 표기와 한계는 보존한다.
- 스킬 네트워크 동기화, 몬스터·아이템 네트워크 동기화, Client Prediction / Reconciliation, Lock-free Queue, Work Stealing, 일반적인 Task Graph는 이번 범위에서 제외한다.

## 3. 날짜별 계획

각 날짜의 시간에는 분석·설계 비교, 구현, 검증, 설명과 로그를 포함한다. 뒤의 기능을 위해 검증 시간을 없애지 않는다.

| 날짜 | 중심 작업 | 시간 배분 예시 | 완료 판단 |
| --- | --- | --- | --- |
| 09.26 · 3일차 | Thread Pool 실제 적용·JSON 외부화 | 로더·시작 경로 연결, 정상/실패 검증, 설명·로그 | Worker 파일 읽기 → 결과 전달 → 메인 스레드 초기화와 설정 수명 설명. 사용자 육안 확인은 별도 |
| 09.27 · 4일차 | 공격 구조, Projectile Pool, Q | 분석·설계·에셋 정리 1~1.5h, 구현 2~3h, 검증·로그 1~1.5h | Ctrl 유지, Q 3초 쿨타임, 좌우 발사, 명중·수명 종료 시 반환 |
| 09.28 · 5일차 | W/E/R, Effect Pool, Handle | 설계·정책 0.5~1h, 구현 2.5~3.5h, 검증·로그 1~1.5h | 5/7/30초 쿨타임과 스킬별 판정, Q 명중 Explosion, Pool 재사용 후 이전 Handle 무효화 |
| 09.29 · 6일차 | IOCP Echo Server | 수명·종료 설계 1~1.5h, 구현 2~3h, 검증·로그 1~1.5h | 연결·비동기 수신/송신·Echo·Disconnect·Pending I/O 회수 |
| 09.30 · 7일차 | Packet Framing, 2 Client | 프로토콜·실행 주체 설계 1h, 구현 2~3h, 검증·로그 1~2h | 부분·병합 패킷, 두 Client Spawn/Despawn·기본 이동 전달 |
| 10.01 · 8일차 | Interpolation, 서버 권위 이동 | 설계 1h, 보간 1~1.5h, 입력 기반 이동 1~2h, 검증·로그 1~1.5h | 원격 보간과 서버 위치 계산. Prediction 없는 로컬 조작 지연은 한계로 기록 |
| 10.02 · 9일차 | 통합 검증·포트폴리오 정리, 여유 시 Spatial Hash | 통합 검증·로그 2~3h 우선 확보, 잔여 시간 2~3h에 Broad Phase 분석·최소 구현 | 기존 기능의 회귀 검증을 우선. Spatial Hash는 비교 검증까지 끝난 경우만 완료 표시 |

09.28과 10.01은 작업 밀도가 높다. 첫 스킬과 기본 네트워크의 실제 작업량에 따라 후속 목표를 조정한다. R 끌어당기기·띄우기, 화려한 추가 HUD 같은 미합의 기능은 넣지 않는다. 간단한 쿨타임 표시 여부는 스킬 설계 시 선택한다.

## 4. 의존 관계와 진행 순서

```text
공격 구조 선택 → Q + Projectile Pool → W/E/R + Effect Pool
                             └────→ Handle 적용·재사용 검증

IOCP Echo·Session 수명 → Packet Framing → 2 Client 기본 동기화
                                           ↓
                              Snapshot Interpolation
                                           ↓
                              Server Authoritative Movement

기존 AABB 기준 → Spatial Hash 후보 생성 → 결과·검사 수 비교

JobSystem 최소 구현 [완료]
       ↓
AnimationClip 동기 JSON 로더
       ↓
Worker 로딩 + promise/future + 메인 스레드 초기화
       ↓
추후 맵 ID / 맵 데이터 / Spawn 설정으로 확장
```

Handle의 타입과 Pool 대여·반환 계약은 Q 구현 때 먼저 설계해 후속 재작업을 줄인다. Handle은 Queue의 스레드 안전성이나 비동기 작업 중 객체 수명을 자동으로 보장하지 않는다.

## 5. 날짜별 시작 전 결정할 사항

| 작업 | 비교·결정할 내용 |
| --- | --- |
| 공격 구조 | PlayerAttack 상속 + Player composition의 최소 인터페이스, 설정 데이터로 표현할 차이와 별도 행동 클래스의 경계 |
| QWER | 제안 에셋·공격 방식 선택, 피해량·속도·거리·수명·유효 프레임, 기본 공격/점프/피격 중 사용 규칙 |
| R | 현재 Monster Hurt 상태의 추가 피해 거부와 다단 타격 정책, 사망 후 R 재시작과의 입력 구분 |
| Pool / Handle | 고갈 시 정책, Generation 갱신 시점, 월드 Reset과 세대 번호 재사용·범위 초과 처리 |
| IOCP | Session / OVERLAPPED / 송수신 버퍼 소유권, 부분 송신, 종료 시 Pending I/O 정리 순서 |
| 게임 네트워크 | I/O 완료 처리와 서버 월드 상태 변경의 실행 주체, Client 메인 스레드 반영 시점 |
| JSON | 현재: nlohmann/json 3.12.0, schemaVersion 1, 필수 설정 실패 시 종료. 후속: 핫 리로드 요청 식별·취소·오래된 결과 정책 |

최종 선택은 기능 구현 전에 사용자와 비교·확정한다. 문서의 제안을 사용자 선택 완료로 기록하지 않는다.

## 6. 일정이 밀릴 때

1. 객체 수명, 중복 반환, Thread Shutdown, Session Disconnect, Packet Framing의 정확성을 먼저 확보한다.
2. 두 Client의 기본 연결·Spawn/Despawn·이동을 안정화한다.
3. Interpolation과 서버 권위 이동은 각각 검증한 뒤 완료 처리한다. 앞 단계가 미완료이면 다음 단계로 넘어가지 않는다.
4. Spatial Hash를 첫 이월 후보로 둔다. 10.02 통합 검증과 기록 시간은 유지한다.
5. 그래도 시간이 부족하면 남은 네트워크 고도화를 다음 작업일로 이월하고, 완료된 것과 미완료 항목을 명시한다. QWER나 Handle 범위를 조용히 삭제하지 않는다.

## 7. 10.02 이후 후속 계획

- 미완료된 안전성·핵심 기능 검증을 먼저 마친다.
- AnimationClip JSON과 시작 시 비동기 로딩은 09.26으로 앞당겼다. 후속에는 필요한 데이터 종류부터 확장한다.
- 재로딩·월드 전환을 도입할 때 요청 식별·취소·기존 설정 수명 정책을 먼저 설계한다.
- Spatial Hash가 이월되면 별도 측정 시간을 확보한다.
- Offscreen Render Target → Fullscreen Post Processing → HP Vignette 순으로 진행한다.
- 맵 ID·맵 배치·Spawn 정보 JSON은 장기 확장 항목이다. 지금 맵 포맷 전체나 범용 에디터를 만들지 않는다.

사용자의 추가 구현 요청으로 JSON 로딩을 현재 작업에 반영했다. 이후 일정은 실제 소요 시간에 따라 조정하며 통합 검증 시간을 유지한다.

## 8. 작업 완료와 기록

- 기존 호출 흐름 분석 → 설계 2안 이상 비교·선택 → 최소 구현 → 리뷰 → 실패 입력·경계 조건 검증 → 결과 기록 순서를 따른다.
- 완료 증거는 빌드 구성, 실행한 테스트, 직접 확인한 동작/측정, 미실행 항목으로 구분한다.
- 성능 수치는 같은 조건에서 비교하며, Draw Call 감소나 Worker 수 증가를 성능 향상으로 단정하지 않는다.
- 각 기능 로그에는 문제 / AI 활용 / 내 판단 / 구현 / 검증 / 결과 / 한계를 남긴다.
- 커밋은 별도 요청 시 진행하며 실제 작업 날짜·당일 순번을 사용한다. 미래 커밋 번호를 완료 이력으로 만들지 않는다.
