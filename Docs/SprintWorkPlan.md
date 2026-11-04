# 기존 AI Sprint 구현·검증 기록

갱신 기준: 2026.11.03. 기존 Sprint 기간: 2026.09.24~10.02.

이 문서는 기존 구현과 검증 기록의 위치를 보존한다. 현재 일정과 기본 실습의 실행 기준은 [11월 3~6일 작업 계획](InterviewPreparationPlan.md)이다. 미구현 서버 확장 일정과 요구는 제거했으며 기존 측정값·자동 검증·사용자 확인 여부는 보존한다.

## 1. 기존 구현 상태

| 구분 | 확인된 상태 | 확인할 기록·한계 |
| --- | --- | --- |
| Profiler / Stress Test / Sprite Batch / Depth Test / Render Queue | 구현·비교 자료 존재 | 기존 수치·조건·한계 보존. Draw Call 감소를 CPU 시간 개선으로 단정하지 않음 |
| Thread Pool / Job Queue | 시작 JSON 로딩에서 Worker 1개·Job 1개 사용 | [Thread Pool 로그](ThreadPool.md). 과거 독립 테스트 프로젝트는 제거 |
| AnimationClip·Stat·공격 JSON | Worker에서 읽기·파싱·검증, 메인에서 초기화 | [로딩 로그](GameData.md). PNG 디코딩·GPU 생성은 현재 메인 스레드 |
| 성장 Stat / QWER / Pool / Handle / HUD | 기존 구현·자동 검증 기록 존재 | [구현 기록](CombatImplementation.md). 실제 플레이·밸런스 확인과 구분 |
| Spatial Hash / Offscreen / Post Processing / HP Vignette | 미구현·후순위 | 현재 3일 준비 범위에 추가하지 않음 |

자동 검증 204개와 WARP 출력은 과거 실행 기록이다. 문서 갱신이나 새 계획 수립을 재실행·직접 플레이 완료로 표시하지 않는다.

## 2. 유지할 구조와 계약

- 기존 Ctrl 공격·QWER·성장 HUD와 Monster / Item / Projectile / HitEffect 재사용 구조 유지.
- 실제 객체 수명은 GameWorld가 unique_ptr로 소유. Handle은 Pool 사용 회차 식별이며 스레드 안전성 보장 수단이 아님.
- JobSystem의 인자 없는 `std::function<void()>` 계약 유지. 결과·예외 전달은 로더의 promise/future 사용.
- Start / Stop / 파괴는 생성 스레드, Submit은 여러 스레드에서 가능. Stop은 접수 중단 후 받은 작업 소진·join.
- 입력·GameWorld·Pool·UI·Immediate Context 접근은 현재 메인 스레드에서 수행.
- Application이 불변 GameData를 World보다 오래 소유. 마지막 멤버 JobSystem은 먼저 파괴되어 Worker 종료 대기.
- Depth 비교의 5,000 버튼 캡처는 실제 500개라는 기존 표기 유지. 기존 측정값을 새 조건의 결과로 재해석하지 않음.

## 3. 현재 작업과 협업

**최종 설계와 소스·테스트 코드 작성은 사용자 담당. Codex는 직접 코드를 수정하지 않고 분석·설계 후보 비교·API 설명·리뷰·검증 방법 제안을 지원한다. 문서는 함께 수정한다.**

현재 목표는 기존 코드·Thread Pool·JSON 로딩 이해와 직접 검증, 별도 TCP 문자열·UDP 위치 값 Echo, 크래시 덤프 기본 분석이다. PNG 로딩의 Thread Pool 활용은 기본 준비 후 시간 여유가 있을 때만 검토한다. 상세 단계·제외 범위·완료 체크는 현재 작업 계획에서만 관리한다.

## 4. 기록 규칙

- 기존 흐름 분석 → 설계 후보 비교 → 사용자 결정·직접 구현 → 리뷰 → 직접 검증 → 결과 기록.
- 새 구현과 과거 구현, 자동 검증과 사용자 육안 확인, 계획과 완료를 구분.
- 문제 / AI 지원 / 사용자 판단 / 구현 / 검증 / 결과 / 한계 기록.
- 성능은 같은 조건에서 비교하며 미측정 개선을 주장하지 않음.
- 커밋·태그·원격 반영은 별도 사용자 요청 범위에서 수행. 기존 이력·측정 자료는 변경하지 않음.

## 관련 문서

- [현재 작업 계획](InterviewPreparationPlan.md)
- [기존 요구사항](Requirements.md)
- [통합 공격 설계 기록](CombatImplementationPlan.md)
- [성장·HUD 설계 기록](ProgressionDesign.md)
- [Thread Pool](ThreadPool.md)
- [JSON 로딩](GameData.md)
- [성장·공격 구현 기록](CombatImplementation.md)
