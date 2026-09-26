# PlayerStat / EnemyStat 성장 시스템 설계·요구사항

작성 기준: 2026.09.26. [27·28일 통합 공격 계획](CombatImplementationPlan.md)에 추가하는 설계. 구현·자동 검증 결과는 [구현 기록](CombatImplementation.md) 참고. 아래는 설계 기준과 초기 수치 제안.

## 1. 적합성 판단과 범위

처치 → 경험치 획득 → 레벨 상승 → 스킬 해금의 플레이 흐름을 만들기에 적합. 기존 JSON 로더·공격 슬롯·피해 결과·Monster Pool을 확장하면 연결 가능. 스킬마다 별도 해금 Boolean을 저장하거나 클래스 생성자를 레벨별 수치로 채우는 구조는 피함.

확정 요구:

- Player의 Level / EXP 도입
- Q는 2레벨, W는 4레벨, E는 6레벨, R은 10레벨에 해금
- 기존 Q/W/E/R 쿨타임 3/5/7/30초 유지, Ctrl 기본 공격은 시작부터 사용
- Monster의 Level 도입, 레벨이 높을수록 처치 EXP 증가
- PlayerStat / EnemyStat 도입, 정적 정의를 JSON으로 저장·로딩
- Level/EXP UI와 공격별 단축키·쿨타임·해금 조건 UI 구현

권장 기본안과 미확정 사항:

- Player 시작 레벨 1·EXP 0, 초기 최대 레벨 10
- 레벨별 필요 EXP·적 보상 EXP는 명시적 테이블 사용. 수식보다 조정·검증이 쉬운 현재 규모에 적합
- Monster는 Spawn / Respawn 시 Player 레벨을 기준으로 설정. 생존 중 레벨·보상 변경 금지
- 현재 HP 밸런스를 보존하기 위해 첫 정의의 maxHp는 모든 레벨에서 3. 공격력 자동 성장·적 공격력 증가·자동 회복은 이번 요구에 포함하지 않음
- 현재 게임의 R Reset을 새 게임 시작으로 해석해 Level / EXP 초기화. 사망 자체에서는 수치 보존
- 진행 상황 디스크 저장은 별도 SaveData 범위. 이번 JSON은 초기값·성장 규칙이며 게임 중 덮어쓰지 않음

## 2. 설계 비교

| 대상 | 안 A | 안 B | 권장 |
| --- | --- | --- | --- |
| 성장 수치 | C++ 수식에서 계산 | JSON 레벨별 테이블 | B: 작은 레벨 범위의 직접 조정·검증 |
| Stat 구성 | HP·레벨·EXP를 기존 클래스와 Stat 양쪽에 보관 | PlayerStat/EnemyStat은 성장 상태, Character는 현재 HP 관리 | B: 현재 구조를 유지하며 중복 원본 방지 |
| Monster 레벨 | 매 프레임 Player 레벨과 동기화 | 생성·재활성화 때 확정 | B: 전투 중 난이도·보상 변경 방지 |
| 보상 시점 | 사망 애니메이션 종료 시 지급 | 피해 결과 Killed 발생 시 지급 | B: 즉시 성장 반영, 연출 시간과 분리 |

## 3. 소유권과 데이터 역할

| 구성 | 보관할 데이터 | 소유자 |
| --- | --- | --- |
| PlayerStatDefinition | 시작·최대 레벨, 레벨별 필요 EXP·maxHp | 불변 GameData |
| EnemyStatDefinition | 지원 레벨별 maxHp·killExp | 불변 GameData |
| AttackDefinition | 기존 스킬 수치 + requiredLevel | 불변 GameData |
| PlayerStat | 현재 level·expInLevel, PlayerStatDefinition 참조 | Player의 값 멤버 |
| EnemyStat | 이번 Spawn의 level·maxHp·killExp, EnemyStatDefinition 참조 | Monster의 값 멤버 |
| Character | 현재 hp_, 적용된 maxHp_ | 기존 Character |
| Monster 생명주기 | spawnSerial, experienceGranted | Monster |

Stat을 GameObject나 Entity에서 상속하지 않고 composition으로 추가. Character::maxHp_는 현재 적용값이며 JSON의 별도 원본 아님. 기존 PlayerDefinition/MonsterDefinition의 maxHp 항목은 Stat 정의로 이동해 두 JSON에 중복 기록하지 않음. 애니메이션·이동·충돌 설정은 기존 정의 유지.

예정 인터페이스 개요:

```cpp
class PlayerStat
{
public:
    explicit PlayerStat(const PlayerStatDefinition& definition);
    LevelUpResult AddExperience(uint64_t reward);
    void Reset();
    uint32_t GetLevel() const;
    uint64_t GetExperience() const;       // 현재 레벨 안의 경험치
    uint64_t GetRequiredExperience() const;
};

class EnemyStat
{
public:
    explicit EnemyStat(const EnemyStatDefinition& definition);
    void InitializeForSpawn(uint32_t level);
    uint32_t GetLevel() const;
    uint64_t GetKillExperience() const;
    int GetMaxHp() const;
};
```

LevelUpResult는 변경 전·후 레벨과 상승 횟수 전달. Player가 결과를 받아 새 maxHp를 Character에 적용하되 현재 HP는 증가시키지 않고 새 최대치 이하로 제한. 이번 기본 테이블은 maxHp 3을 유지하므로 기존 HP 동작 보존.

## 4. JSON 로딩

예정 파일:

- `Assets/Data/PlayerStat.json`: schemaVersion, initialLevel, initialExp, maxLevel, levels
- `Assets/Data/EnemyStat.json`: schemaVersion, levels
- `Assets/Data/GameData.json`: 기존 클립·텍스처·공격 정의. Normal/Q/W/E/R의 requiredLevel을 각각 1/2/4/6/10으로 저장

Player 레벨 행은 `{ level, expToNext, maxHp }`, Enemy 레벨 행은 `{ level, maxHp, killExp }` 형식. 최대 레벨의 expToNext는 0, 그 이전 레벨은 양수. requiredLevel의 저장 위치는 공격 정의 한 곳으로 한정.

GameData.json을 기준으로 같은 Data 디렉터리의 두 파일을 로딩. 현재 시작 Job 하나가 세 파일을 읽고 파싱·상호 검증한 뒤 완성된 GameData 하나를 future로 전달. 파일별로 게임 상태를 먼저 적용하지 않으며, 하나라도 실패하면 기존 정책대로 시작 중단. Worker 수 증가나 범용 설정 시스템 추가 불필요.

GameData에 성장 정의와 공격별 icon 경로 포함. 실제 코드·데이터에 schemaVersion 3 적용 완료. PlayerStat/EnemyStat 파일은 각각 자체 schemaVersion 1부터 시작. 파일별 버전은 릴리스 버전과 별개.

필수 검증:

- initialLevel이 1 이상 maxLevel 이하, 초기 EXP가 해당 레벨의 필요량 미만. 최대 레벨 초기 EXP는 0
- 최대 레벨이 R 해금 레벨 이상, 레벨 테이블에 1~maxLevel의 중복·누락 없음
- expToNext는 최대 레벨 이전에 양수이며 비감소. maxHp는 양의 정수
- Enemy 테이블은 Player의 모든 Spawn 가능 레벨을 포함. killExp는 양의 정수이며 레벨 상승에 따라 엄격히 증가
- requiredLevel이 정의된 Player 레벨 범위 안에 존재. 실제 배포 데이터의 Q/W/E/R 값은 2/4/6/10과 일치
- EXP는 uint64_t 범위 내 정수만 허용. 음수·소수·과대값을 형변환 전에 거부
- 파일·레벨 행·필드 경로를 포함한 오류 전달

## 5. 레벨업·해금 처리

`expInLevel`은 총 누적 EXP가 아닌 현재 레벨 안의 진행량. 보상에 남은 필요량을 차례로 적용해 한 번의 보상으로 여러 레벨 상승 가능. 새 레벨마다 필요 EXP를 다시 조회하며 uint64_t 합산 오버플로를 피하도록 남은 보상을 나누어 처리.

예: 레벨 1 EXP 5/10에서 보상 40 획득 → 5로 레벨 2 → 30으로 레벨 3 → 레벨 3 EXP 5 유지.

- 최대 레벨 도달 시 level을 상한에 고정하고 expInLevel 0으로 정리. 이후 보상은 성장에 미반영, HUD는 MAX 표시
- 잠금 상태는 `player.level < attack.requiredLevel`로 계산. 별도 unlocked 플래그 중복 저장 금지
- PlayerAttack::TryUse에 Locked 반환값 추가. 해금 검사 → 상태·쿨타임 검사 → 생성 → 성공 시 쿨타임 순서
- 잠긴 키 입력은 생성·애니메이션·쿨타임에 영향 없음. 해금 전 입력을 대기시켰다가 자동 발동하지 않음
- 다중 레벨 상승 시 해당 레벨 이하의 모든 슬롯 사용 가능. 새로 해금된 스킬의 최초 쿨타임은 0
- 레벨업을 일으킨 타격은 기존 발사 시점의 피해 정의로 처리. 이미 진행 중인 시전이나 쿨타임을 초기화하지 않음
- 입력 요청은 해당 프레임 피해 처리보다 먼저 소비하므로, 처치로 해금된 스킬은 다음 유효 입력부터 사용

## 6. Monster 레벨·보상·재사용

현재 CollectDeadMonsters는 사망 애니메이션 종료 후 killCount 증가·아이템 드롭·비활성화·Respawn Queue 등록을 수행. 이 경로에 EXP를 추가로 지급하지 않음.

```text
GameWorld::ApplyDamageToMonster(source, target, request)
  → Monster::TakeDamage → Ignored / Applied / Killed
  → Killed + Player 기여 + 일반 Monster + 미지급 사용 회차
  → experienceGranted 표시
  → 해당 Spawn의 EnemyStat.killExp를 Player::GainExperience에 전달
  → PlayerStat 레벨업 → 적용 maxHp·해금 상태·HUD 갱신

이후 사망 애니메이션 종료
  → 기존 killCount / DropItem / Respawn 처리만 수행
```

일반 공격과 QWER는 모두 같은 ApplyDamageToMonster 경로 사용. Player에 경험치를 지급하는 위치는 한 곳으로 제한. source는 현재 싱글플레이 Player 공격인지 구분하는 값이며, 사망 시점에 임의의 Player 포인터를 추측하지 않음.

- 이번 Spawn의 level·killExp는 처음 설정한 값 유지. 처치 후 Player 레벨이 올라가도 방금 처치한 적 보상 재계산 금지
- experienceGranted는 SpawnSerial과 함께 관리. Killed 처리 시 보상 호출 전에 표시해 중복 이벤트·다단 공격에도 1회 지급
- 반납·사망 애니메이션·Reset·강제 비활성화·Stress Monster에는 EXP 없음
- GameWorld의 SpawnMonster 도우미에서 레벨 선정 → EnemyStat 초기화 → SpawnSerial 증가·보상 플래그 초기화 → HP/FSM/Animator 초기화 → 활성화 순서
- CreateMonsters / UpdateMonsterLock / UpdateMonsterRespawn / Reset의 직접 SetActive(true)를 해당 경로로 통합. 같은 Idle 상태여도 Animator 재생은 명시적으로 초기화
- 기존 Monster Pool의 개수·killCount 해금 기준·아이템 드롭 규칙 유지. 스킬 해금과 Monster 개체 수 해금은 서로 다른 규칙
- Stress Monster는 고정 레벨 1·보상 지급 금지. 일반 성장과 성능 측정 분리

## 7. 성장 테이블

레벨·스킬 해금 및 처치 수 기준은 확정 요구. 동일 레벨 적 기준으로 Lv.L → L+1에 L+1마리 필요. 필요 EXP는 `(L+1) × 해당 레벨 적의 처치 EXP`로 산정해 JSON 테이블에 저장. HP는 기존 초깃값 유지.

| Level | 다음 레벨 필요 EXP | 해당 레벨 Enemy 처치 EXP | Player / Enemy maxHp | 스킬 해금 |
| ---: | ---: | ---: | ---: | --- |
| 1 | 10 | 5 | 3 / 3 | Ctrl |
| 2 | 30 | 10 | 3 / 3 | Q |
| 3 | 60 | 15 | 3 / 3 | — |
| 4 | 100 | 20 | 3 / 3 | W |
| 5 | 150 | 25 | 3 / 3 | — |
| 6 | 210 | 30 | 3 / 3 | E |
| 7 | 280 | 35 | 3 / 3 | — |
| 8 | 360 | 40 | 3 / 3 | — |
| 9 | 450 | 45 | 3 / 3 | — |
| 10 | 0 (MAX) | 50 | 3 / 3 | R |

생존 중인 Monster는 기존 레벨을 유지하므로 같은 화면에 서로 다른 레벨이 존재할 수 있음. 개체별 레벨·보상을 Spawn 시점의 표시 또는 로그로 확인.

## 8. 성장 HUD·공격 슬롯 UI·Reset

사용자 추가 요구에 따라 공격별 단축키·쿨타임·해금 조건 UI를 통합 구현 범위에 포함. 생성 정사각형 아이콘과 회전 쿨타임 표시 포함.

- Player Level, 현재 EXP / 다음 레벨 필요량과 EXP 진행 바를 기본 HUD에 표시. 예: `Lv.3 · EXP 15 / 60`, 진행률 25%
- 진행 바는 현재 레벨의 EXP / 필요 EXP로 계산해 0~1로 제한. 다중 레벨업 후 최종 레벨의 잔여 EXP 반영. 최대 레벨은 MAX와 채워진 바로 표시하며 0으로 나누지 않음
- EXP 획득·레벨업·사망·재시작 결과를 같은 프레임의 HUD 값에 반영. Profiler 표시 여부와 독립적으로 출력
- 화면 우하단에 Ctrl/Q/W/E/R 정사각형 슬롯 5개, 최하단 EXP 패널 위에 16px 기준 간격으로 배치. Profiler의 F1 표시 여부와 무관하게 기본 HUD에서 표시
- 상단 이름 바와 하단 기본 쿨타임 바 제거. 단축키는 항상 표시하며 해금 레벨은 잠긴 스킬의 자물쇠 바로 아래에만 표시
- 잠금 중에만 자물쇠 오버레이와 Lv. 조건 표시. 단축키는 아이콘 우상단. 쿨타임은 12시부터 시계방향으로 걷히는 오버레이와 중앙 남은 시간(s), 완료 시 두 표시 제거
- 공중·사망·넉백·다른 공격 중 상태는 아이콘 어둡게 표시. 별도 상태 문구 바는 제거
- 잠금 → 사용 상태 제한 → 쿨타임 → 준비 순서로 대표 상태 선택. 상태 제한 중에도 남은 쿨타임 수치는 계속 표시
- 내장 imagegen 생성 아이콘 사용. 기존 GDI 텍스트·Renderer 도형으로 구성, 신규 UI 라이브러리 불필요
- 슬롯은 정보 표시 전용. 마우스 클릭 발동·단축키 재지정은 이번 범위 밖

표시 예시:

| 실제 상태 | 표시 예시 |
| --- | --- |
| Lv.1의 Q | `Q / 자물쇠 / Lv.2` |
| Lv.2의 Q, 사용 가능 | `Q / 아이콘` |
| Q 사용 후 1.2초 경과 | `Q / 중앙 1.8s / 회전 마스크` |
| 사망 중 R | `R / 어두운 아이콘` 및 기존 재시작 안내 |

사망 중 아직 잠긴 R은 대표 상태 `잠김` 유지, 재시작 안내는 별도 표시. 게임 재시작 기능과 공격 슬롯의 해금 조건을 혼합하지 않음.

데이터 흐름:

```text
GameData.AttackDefinition: inputKey / cooldown / requiredLevel / displayName
PlayerAttack: 남은 쿨타임 / 실제 사용 가능 여부와 실패 사유
PlayerStat: Level / EXP
    → Player::GetAttackHudData()의 값 스냅샷
    → Application::UpdateUI → UIFrameData
    → UIManager: 표시 상태 저장·문자 생성
    → Renderer: 기존 HUD 패스에서 출력
```

- 단축키를 문자열 레이블만 별도로 저장하지 않음. JSON의 inputKey를 검증된 키 enum으로 변환하고 입력 바인딩·표시가 함께 참조. 기본값은 Ctrl/Q/W/E/R, 중복·미지원 키는 로딩 오류
- `PlayerAttack::GetAvailability`를 입력 검사와 HUD에서 공유. HUD가 자체 레벨·상태 규칙을 다시 구현하지 않음. GetAvailability는 조회만 수행, 쿨타임이나 Pool 상태 변경 금지
- 실제 Pool 대여 성공 여부는 TryUse에서 확정. 일반적인 `준비` 표시는 잠금·캐릭터 상태·쿨타임 통과를 의미하며, Pool 고갈이나 E 지면 부재는 실패 직후 짧은 사유 표시로 보완
- UIFrameData에는 표시용 값만 복사. UIManager는 Player/Monster 포인터·Stat 원본을 소유하지 않고 성장·해금·저장 계산도 수행하지 않음
- HUD 텍스트용 캐시를 Profiler 텍스트와 분리. 레벨·잠금·준비·실패 상태 변경은 즉시 갱신, 남은 시간 문자열은 0.1초 단위로 변경될 때만 재생성. 텍스처를 매 프레임 새로 만들지 않음
- 남은 시간이 양수일 때 표시는 올림한 소수 첫째 자리 사용. 실제로 사용 불가한데 `0.0초 / 준비`로 표시하지 않음
- 기존 HP HUD·GameOver·Profiler 버튼과 겹치지 않게 현재 1280×720 기준 영역 확보. UI는 Depth 비활성 HUD 패스에서 그리며 월드 텍스처 정렬과 분리
- Monster 레벨은 화면 표시 또는 검증 로그에서 확인 가능하도록 구성. 개체별 상시 HP 바는 필수 아님
- 사망 자체는 EXP를 자동 증가·차감하지 않음. R Reset은 권장안에서 새 게임으로 처리해 초기 Level/EXP·HP·쿨타임·Pool 상태 복원
- Reset 시 활성 공격·효과와 오래된 보상 요청을 폐기한 뒤 Player 초기화 → Monster Spawn 순서. 초기 레벨로 돌아간 뒤 이전 처치 EXP가 늦게 지급되는 문제 방지

## 9. 추가 파일·변경 위치

| 파일 | 예정 변경 |
| --- | --- |
| `Game/Stats/PlayerStat.h/.cpp` 신규 | Player Level/EXP, 다중 레벨업·최대 레벨·Reset |
| `Game/Stats/EnemyStat.h/.cpp` 신규 | Spawn 시 레벨·maxHp·killExp 확정 |
| `Game/Stats/StatDefinitions.h` 신규 | JSON 레벨 행·정의 타입 |
| `Assets/Data/PlayerStat.json`, `EnemyStat.json` 신규 | 초기 상태·레벨별 정의 |
| `Game/Data/GameData.h/.cpp` | Stat 정의 소유, 3개 파일 읽기·상호 검증 |
| `Game/Player.h/.cpp` | PlayerStat 소유, GainExperience, maxHp 적용·Reset, 공격 HUD 스냅샷 |
| `Game/Monster.h/.cpp` | EnemyStat 소유, SpawnSerial·보상 플래그·Spawn 초기화 |
| `Game/Combat/PlayerAttack.h/.cpp` 예정 | requiredLevel 검사·Locked 결과·공통 GetAvailability |
| `Game/World/GameWorld.h/.cpp` | Spawn 경로 통합·단일 피해/EXP 지급 경로 |
| `Engine/Core/Application.cpp`, `UIManager.h/.cpp` | Level/EXP·공격 슬롯 표시값·텍스트 캐시·HUD 출력 |
| `DX11ScrollRPG.vcxproj/.filters` | Stats와 신규 JSON 등록 |

## 10. 요구사항·수용 기준

| ID | 요구사항 | 수용 기준 |
| --- | --- | --- |
| G-01 | Player Level/EXP | JSON 초기값으로 생성, PlayerStat에서 일원화 |
| G-02 | 스킬 해금 | Lv1은 Ctrl만, Lv2/4/6/10에서 Q/W/E/R 사용 가능 |
| G-03 | 잠금 상태 | 직전 레벨에서 거부, 쿨타임·Pool·애니메이션 변경 없음 |
| G-04 | 다중 레벨업 | 임계 직전/일치/초과·대량 보상에서 올바른 상승과 잔여 EXP 유지 |
| G-05 | 최대 레벨·정수 | 상한 고정·MAX 표시, 음수/소수/과대값 거부, 합산 오버플로 없음 |
| G-06 | Enemy Level/EXP | Spawn 시 확정, 레벨별 보상 증가, 생존 중 Player 성장과 독립 |
| G-07 | 보상 1회 지급 | Ctrl/Q/W/E/R 치명타 모두 1회. 동시 명중·주기 피해·사망 연출 종료에서 중복 없음 |
| G-08 | Pool 재사용 | Respawn마다 레벨·HP·보상 플래그 초기화, 이전 사용 회차와 구분 |
| G-09 | 처치 외 지급 금지 | Reset·강제 비활성화·Stress Monster에서 EXP 없음 |
| G-10 | 설정 일괄 적용 | 3개 파일 중 하나라도 오류면 시작 중단, Worker의 게임 객체 변경 없음 |
| G-11 | 정의 중복 방지 | maxHp·requiredLevel의 JSON 원본 단일화, 현재 HP 소유자는 Character |
| G-12 | 성장 HUD | Level·현재/필요 EXP·진행 바·MAX가 실제 성장 상태와 일치. 다중 레벨업·사망·Reset·F1 전환 후에도 올바른 표시 |
| G-13 | 재시작 | R Reset 성장 초기화, 오래된 보상 없음, 잠긴 R 오발 없음 |
| G-14 | 기존 공격 검증 | 검증용 고레벨 초기화로 QWER 확인, 제품 기본값은 Lv1/EXP0 |
| H-01 | 슬롯 정보 | Ctrl/QWER 단축키, 잠금 중에만 자물쇠·해금 레벨, 쿨타임 중에만 잔여 초 표시 |
| H-02 | 상태 일치 | 잠금·준비·쿨타임·상태 제한이 실제 GetAvailability 결과와 일치 |
| H-03 | 시간 표시 | 실제 양수 쿨타임을 준비로 표시하지 않으며, 쿨타임 종료 후 상태 갱신 |
| H-04 | 입력·데이터 일치 | JSON 입력 키·기본 쿨타임·requiredLevel 변경이 입력 판정과 UI에 함께 반영 |
| H-05 | 게임 흐름 | 레벨업·사망·R Reset·Pool 고갈·지면 부재에서 표시가 이전 상태로 남지 않음 |
| H-06 | 출력·비용 | F1 숨김 중에도 공격 HUD 표시, HP/GameOver/Profiler와 구분, 불필요한 매 프레임 텍스처 생성 없음 |

## 11. 구현 순서와 추가 검증

1. Stat 정의·JSON 로더·PlayerStat/EnemyStat, EXP 임계값 검증
2. Spawn 경로 통합·DamageResult·EXP 지급 일원화
3. PlayerAttack 해금 검사·GetAvailability·Q 구현 후 W/E/R 확장
4. 성장·공격 슬롯 HUD, Reset·통합 회귀

기존 C-01~22, 추가 G-01~14와 H-01~06을 함께 검증. 해금은 1→2, 3→4, 5→6, 9→10의 경계 확인. QWER 검증을 위해 제품 코드의 해금 조건을 무력화하지 않고 검증용 정의에서 정상 범위의 고레벨로 시작.

JSON 수정 후 재실행, 레벨 행 누락·중복, 잘못된 EXP, Enemy 레벨 부족을 검증. 한 번의 범위 공격으로 여러 적 처치, R 연속 명중, 사망 연출 중 추가 충돌, Respawn 후 재처치, 최대 레벨, Reset 확인. UI는 실제 발동 결과·남은 시간·해금 상태와 대조해 육안 확인.

증분 추정: 성장 데이터·보상·해금·HUD·검증에 약 4~6시간. 통합 공격 계획은 합계 약 14~20시간으로 갱신. 이틀 내 무조건 완료로 취급하지 않고 미완료분은 후속으로 이월. IOCP는 계속 연기.
