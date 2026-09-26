# 09.27~09.28 통합 공격 시스템 구현 계획·요구사항

작성 기준: 2026.09.26, 코드 기준: `4d3167a`의 게임 구현. 현재 구현 결과와 명세 구체화 사항은 [구현 기록](CombatImplementation.md) 참고.

상태: **설계 기준. 구현·자동 검증 완료, 사용자 플레이 검토 별도.** 27일과 28일 작업을 하나의 범위로 통합하고, IOCP는 날짜 미정의 후속으로 이월. 사용자의 명세 기반 구현 지시에 따라 권장안을 적용. 인터페이스·에셋 구체화 차이는 구현 기록에 명시.

## 1. 범위와 확정 사항

- 기존 Ctrl 일반 공격·이동·점프·Monster/Item Pool 유지
- Q/W/E/R 추가, 쿨타임 각각 3/5/7/30초
- Q/W/E/R 해금 레벨 2/4/6/10. PlayerStat/EnemyStat·성장 JSON·레벨별 처치 EXP 도입
- Level/EXP 수치·진행 바와 공격별 단축키·쿨타임·해금 조건 HUD 필수 구현. 상세 내용은 [성장 시스템 설계](ProgressionDesign.md) 참고
- Projectile·HitEffect는 사전 생성 후 Pool 대여·반환
- Handle / Lookup / Generation으로 사용 회차 식별·재사용 검증
- 정적 스킬·애니메이션 설정은 기존 JSON 로딩 경로에 추가
- 공격·Pool·충돌·렌더링 상태 변경은 메인 스레드에서 수행
- 네트워크, Offscreen, Post Processing, Spatial Hash, 기존 전체 객체의 Handle 전환은 이번 범위 밖

## 2. 실제 코드에서 확인한 출발점

| 현재 위치 | 동작 | 필요한 변경 |
| --- | --- | --- |
| `Game/Player.cpp`: HandleInput / StartAttack | Ctrl 입력에서 Attack 상태로 전환, 공격 프레임은 JSON 참조 | 일반 공격과 스킬 입력 요청을 분리하고 공통 공격 슬롯에 연결 |
| `Game/World/GameWorld.cpp`: Update | Entity → Physics → Ground Collision → Combat → Monster 회수·재생성 → Item 처리 | 공격 요청·스킬 갱신·스킬 충돌·Pool 회수 단계 삽입 |
| `GameWorld::UpdateCombat` | TakeDamage 호출 뒤 성공 여부와 무관하게 RegisterAttackHit 호출 | 실제 피해 결과에 따라 일반 공격 명중 기록 |
| `Game/Monster.cpp`: TakeDamage | Dead / Hurt 상태에서 추가 피해 거부 | 일반 타격 정책 보존, R 주기 피해만 별도 정책 |
| `GameWorld::Update` | 사망 중 R을 누르면 Reset | 궁극기 R과 재시작 R 분리, 동일 입력 재사용 방지 |
| `GameObject::SetActive` | 같은 상태로 설정하면 OnEnable/OnDisable 미호출 | Pool 대여 시 명시적 초기화 후 활성화, 반환 API에서 상태 정리 |
| `Game/Animation/Animator.cpp` | Update 호출당 최대 한 프레임 진행 | 긴 deltaTime의 누적 프레임 처리 및 스킬 판정 시간 분리 |
| `Game/Data/GameData.cpp`: ReadTextures | 알려진 SpriteId 17개를 정확히 요구 | 스킬·효과 ID와 필수 경로 검증 동시 확장 |
| `Application::Render`, `GameWorld::AddGameObject` | 레이어 열거형·깊이 배열·Depth OFF 순서를 별도 관리 | 새 투명 레이어를 모든 경로에 반영 |

구현 전 기준 코드에는 PlayerAttack·Q·Projectile·Handle 구현이 없었음. 28일 확장을 먼저 붙이는 대신 공통 기반부터 순서대로 구현.

## 3. 설계 비교와 권장안

| 대상 | 안 A | 안 B | 권장 |
| --- | --- | --- | --- |
| 공격 구조 | Player 내부 switch에 모든 스킬 추가. 파일은 적지만 상태·쿨타임·생성 책임 집중 | PlayerAttack 기반과 Player의 소유 배열. 행동이 같은 스킬은 같은 파생 클래스 사용 | B |
| Pool 소유권 | Pool이 unique_ptr를 소유하도록 월드 소유권 재편 | gameObjects_ 소유 유지, Pool은 고정 슬롯·비소유 참조·세대 관리 | B |
| 충돌 | 현재 위치 AABB만 사용. 고속 투사체가 적을 통과할 수 있음 | 투사체 경로와 적의 이동을 고려한 Swept AABB, 정지 영역은 AABB | B |
| 명중 식별 | Monster 포인터만 저장. 같은 주소의 Respawn을 구분하지 못함 | 일반 Monster 인덱스 + SpawnSerial로 사용 회차 식별 | B |
| 피격 정책 | Hurt이면 모든 피해 차단 | 일반 공격은 기존 정책 유지, 주기 피해는 경직과 분리 | B |

Q와 W는 피해 방식의 변형이므로 ProjectileAttack 하나에 서로 다른 설정 주입. E는 GroundAreaAttack, R은 PeriodicAttack으로 분리. 스킬 이름마다 파생 클래스를 만들지 않음.

## 4. 클래스·소유권·인터페이스

```text
Application
 ├─ const GameData                           정적 정의 소유
 └─ GameWorld
     ├─ gameObjects_: unique_ptr<GameObject> 실제 객체 수명 소유
     │   ├─ Player
     │   │   └─ array<unique_ptr<PlayerAttack>, 5>
     │   │       ├─ MeleeAttack              Ctrl
     │   │       ├─ ProjectileAttack         Q
     │   │       ├─ ProjectileAttack         W
     │   │       ├─ GroundAreaAttack         E
     │   │       └─ PeriodicAttack           R
     │   ├─ SkillObject[]                    이동 투사체·지면 영역·회오리
     │   └─ HitEffect[]                      시각 효과
     ├─ skillPool_                           슬롯·비소유 참조·세대
     └─ effectPool_                          슬롯·비소유 참조·세대
```

SkillObject는 GameObject 파생으로 구성. 별도 UpdateSkills에서 이동·수명·판정을 관리하고 entities_에 등록하지 않음. 기존 중력·지면 충돌 루프와 중복 갱신 방지. GameWorld가 SkillObject의 콜라이더와 HitEffect의 재생을 갱신. PlayerAttack은 사용 요청·쿨타임만 관리하고 발사 이후 객체를 소유하지 않음.

예정 인터페이스 개요:

```cpp
enum class AttackSlot { Normal, Q, W, E, R, Count };
enum class AttackUseResult { Started, Locked, Cooldown, InvalidState, PoolFull, NoGround };

class PlayerAttack
{
public:
    virtual ~PlayerAttack() = default;
    AttackUseResult TryUse(Player& player, GameWorld& world);
    void UpdateCooldown(float deltaTime);
    void Reset();
    float GetRemainingCooldown() const;
protected:
    // 실제 생성 성공 여부를 공통 TryUse에 전달. 실패하면 쿨타임 미소비.
    virtual AttackUseResult Execute(Player& player, GameWorld& world) = 0;
};

enum class DamageReaction { NormalHit, Periodic };
enum class DamageResult { Ignored, Applied, Killed };
struct DamageRequest { int amount; float attackerX; DamageReaction reaction; };
// Character / Player / Monster의 기존 void TakeDamage를 같은 반환 계약으로 변경.
// virtual DamageResult TakeDamage(const DamageRequest& request);
```

`TryUse`: 해금 레벨 확인 → 상태·쿨타임 확인 → Execute → 성공 시 쿨타임 시작과 시전 상태 전환. 생성 실패 시 Player 상태·쿨타임 유지. Normal은 기존 Attack 상태·유효 프레임 사용, 스킬은 SkillCast 상태를 추가해 기존 일반 공격 HitBox의 중복 활성화 방지. 새 캐릭터 시전 에셋 없이 기존 attack 클립 재사용.

Player와 GameWorld의 상호 참조는 동기 호출의 인자로만 전달. Worker 람다에 객체 주소를 캡처하지 않음. Player가 unique_ptr<PlayerAttack>를 전방 선언으로 보관하면 소멸자는 Player.cpp에서 정의.

## 5. 입력·사용 규칙 권장안

- Ctrl은 기존 누르고 있는 동안의 재공격 동작 유지. QWER는 눌린 순간 한 번 요청
- 게임 프레임 시작 시 Ctrl/QWER 상태를 한 번 수집. 비활성 창에서는 요청 생성 금지
- 사망·넉백·공중·일반 공격·SkillCast 중 신규 스킬 거부. 입력 예약 없음
- 동시에 여러 스킬을 누르면 Q → W → E → R 순으로 첫 요청 하나 선택. 실패해도 다른 키로 자동 대체하지 않음. 스킬 요청과 Ctrl이 겹치면 스킬 우선
- 시전 시작 즉시 스킬 생성. SkillCast 동안 수평 이동과 일반 공격 차단, 클립 종료 후 기존 Idle/Run 전환
- 정상 발동 뒤 쿨타임은 deltaTime으로 감소. 넉백 중에도 감소하고 사망 시 갱신 중단. Reset 시 0으로 초기화
- 사망 중 R은 Reset에만 사용. Reset 프레임 즉시 종료, 입력 이전 상태를 현재 키 상태로 맞춰 R을 떼고 다시 누르기 전 스킬 발동 금지
- Player 사망·Reset 시 활성 스킬/효과와 대기 명령 반환·폐기. Stress Mode 진입 시 스킬 생성 차단 및 활성 스킬/효과 정리

키 입력 중복 처리를 막기 위해 GameWorld의 R 재시작과 Player의 공격 입력이 같은 프레임 입력 스냅샷 사용. World 객체를 갱신하는 일반 Job Queue는 도입하지 않음.

## 6. 스킬 동작과 초기 조정값

단축키·쿨타임·해금 레벨은 확정. 아래 피해·이동 수치는 구현·플레이 확인을 위한 초깃값 제안이며 JSON으로 조정.

| 슬롯 | 행동 | 피해량 제안 | 속도 / 수명 제안 | 적 접촉 정책 |
| --- | --- | ---: | --- | --- |
| Ctrl | 기존 근접 공격 | 1 | 기존 공격 클립 | 실제 피해 성공 시 한 대상 명중 기록 |
| Q / Fire_Ball | 직선 투사체 | 1 | 0.9 월드 단위/s, 최대 1.5초 | 이동 경로상 첫 적 접촉 시 반환. 피해 성공 시 Explosion |
| W / Wind | 관통 투사체 | 1 | 1.4 월드 단위/s, 최대 1.2초 | 같은 Monster 사용 회차에 접촉 1회만 시도 |
| E / Earth_Spike | 전방 지면 영역 | 2 | 9프레임 × 0.08초 | 0부터 세는 3~5프레임 구간에서 대상별 1회 시도 |
| R / Tornado | 느린 이동 + 주기 피해 | 틱당 1 | 0.2 월드 단위/s, 최대 4초 | 활성 구간 동안 0.5초 간격으로 영역 내 대상별 1회 |

- Q/W는 발사 시 바라보는 방향 고정. 이후 Player 회전과 무관하게 이동
- E는 전방 위치의 Ground 상단에 배치. 적합한 Ground가 없으면 발동 실패·쿨타임 미소비
- R은 발동 위치의 지면 높이를 유지하며 전진. 경사·낭떠러지 추종은 후속
- 접촉과 피해 성공 구분. Q는 Hurt인 적에 막혀도 소멸하지만 성공 이펙트는 발생하지 않음. W/E는 피해 거부 시에도 그 사용 회차의 접촉 기록 유지
- R은 Hurt 중에도 HP 감소 가능, 기존 Hurt 애니메이션과 넉백을 매 틱 재시작하지 않음. 치명 피해는 Dead 전환 우선
- R의 첫 틱은 생성 시각 기준 0, 이후 0.5초 간격. 끝 시각 4초 제외, 최대 8틱. 긴 프레임은 지나간 틱을 시간순 처리하고 대상별 중복 틱 방지
- 효과 Pool 고갈은 시각 효과만 생략. 이미 적용한 피해를 취소하지 않음

## 7. Handle / Lookup / Pool 계약

```cpp
template<class Tag>
struct PoolHandle
{
    uint64_t storageId = 0;      // Pool 인스턴스 식별, 다른 World/Pool과 혼동 방지
    uint32_t slot = UINT32_MAX; // 객체 주소 대신 고정 슬롯 번호
    uint64_t generation = 0;   // 슬롯의 사용 회차
};
// SkillHandle과 EffectHandle은 서로 다른 Tag 사용.
// TryAcquire(spawnInfo) -> optional<Handle>
// Lookup(handle) -> T* 또는 nullptr
// Release(handle) -> bool
```

- GameWorld가 초기 객체를 gameObjects_에 등록하고 Pool에 비소유 T* 연결. free 슬롯 목록에서 O(1) 대여·반환·조회
- 기본 용량 제안: SkillObject 64, HitEffect 128. JSON에서 양의 제한값으로 지정하고 플레이 중 자동 확장 없음
- Lookup은 storageId·슬롯 범위·대여 상태·generation 순으로 검사. 외부 장기 저장은 Handle만 허용, 조회 포인터는 현재 처리 구간에서만 사용
- Release 성공 시 비활성화·상태 정리·generation 증가. 중복 Release와 오래된 Handle은 false, free 목록에 중복 추가 금지
- Reset은 활성 슬롯 반환, 기존 generation 초기화 금지. Pool 재생성 시 새 storageId 사용
- generation 최대값 도달 슬롯은 폐기 상태로 전환해 재대여 금지. storageId 고갈은 새 Pool 생성 실패로 처리
- 위치·방향·수명·Animator·히트 기록·틱 상태·반환 예약을 대여 때 모두 초기화. 반환 대기 슬롯은 같은 프레임에 재대여하지 않음
- SetActive 직접 호출은 Pool 내부로 제한하는 사용 규칙 적용. 현재 GameObject 공개 API가 이를 컴파일 단계에서 강제하지는 못하므로 전체 접근 제어 리팩터링과 구분
- 기존 Monster / Item Pool은 유지. 스킬의 Monster 명중 이력은 `(monsters_ 인덱스, spawnSerial)` 사용. 정상 활성화마다 serial 증가, Reset에서도 이전 serial로 되돌리지 않음. 고갈 시 재활성화 중단
- 새 SkillObject/HitEffect는 런타임 삭제하지 않음. 이 설계의 검증 대상은 물리적 메모리 삭제가 아닌 Pool의 논리적 사용 회차

## 8. GameWorld 갱신과 충돌 순서

```text
입력 스냅샷 / 재시작 처리
  → 기존 활성 스킬 Handle·Monster 위치/SpawnSerial 기록
  → Player / Monster Update 및 쿨타임 갱신
  → 기존 Physics / Ground Collision
  → 공격 요청 하나 소비, Pool 대여·시전 상태 전환
  → 프레임 시작에 활성 상태였던 스킬 이동·수명 갱신
  → 기존 근접/몸체 충돌, 이어서 스킬 충돌·피해·효과 요청
  → Player 사망이면 모든 스킬·효과 반환 예약
  → 효과 재생 갱신 / 반환 목록 적용
  → 기존 Monster 회수·Respawn / Item 처리
```

새로 생성한 스킬·효과는 해당 프레임에 표시하고 다음 프레임부터 시간 갱신·충돌 처리. 동일 프레임의 전체 deltaTime을 생성 직후 적용하는 문제 방지. 스킬 충돌은 player collider의 활성 여부와 별도로 관리하되 사망·Stress 차단 규칙 적용.

Q/W/R 이동은 남은 수명만큼의 시간으로 제한. 경로 밖 화면 경계 반환도 충돌 확인 뒤 적용해 마지막 이동 구간의 명중 보존. Q는 monsters_ 등록 순서가 아닌 가장 이른 충돌 시각을 선택하고 동률은 인덱스로 결정.

Swept AABB는 스킬의 이전/현재 위치와 적의 이전/현재 위치의 상대 이동으로 검사. 순간 이동·Respawn 시 새 SpawnSerial로 이전 이동 경로를 폐기. E는 유효 시간 구간과 이번 프레임 구간의 교집합에서 판정해 긴 프레임의 유효 구간 건너뜀 방지. R은 도달한 틱 시각의 보간 위치로 판정하며 지나간 모든 유효 틱을 한 번씩 처리. JSON에서 lifetime/hitInterval 비율 상한을 두어 단일 프레임의 처리량 제한.

충돌 중 컨테이너 erase나 Pool 재대여 금지. 반환은 Handle 목록에 예약하고 충돌 순회 후 일괄 처리. 새 스킬 검사는 기존 CollisionChecks와 CombatCollision 계측에 포함, 추가 Profiler UI는 필요 시 후속.

## 9. JSON·에셋·렌더링

- GameData에 공격 슬롯별 정의와 Pool 용량 추가. 공통 필드는 cooldown/damage, 스킬 필드는 speed/lifetime/hitInterval/hitBox/spawnOffset/시각 클립 참조
- 기존 player.attack 유효 프레임·공격 범위는 일반 공격 정의로 이동. 같은 값을 두 곳에 유지하지 않음
- GameData schemaVersion을 2로 변경하고 배포 JSON과 로더를 함께 수정. v0.1.0 릴리스 태그 변경 없음. 이전 schema 1을 조용히 기본값으로 처리하지 않고 버전 오류로 거부
- 현재 엄격한 17개 텍스처 목록을 스킬·효과 SpriteId까지 확장. 클립 ID·텍스처 ID 누락과 중복 슬롯 검사
- 쿨타임·수명·간격의 유한성·범위, 양의 Pool 용량, damage 정수, 유효 프레임 범위, R 틱 수 상한 검사
- 캐릭터 상태 클립의 loop 제약과 스킬 시각 클립의 loop 제약을 구분. 스킬 JSON을 기존 Player FSM 검증 함수에 그대로 끼우지 않음
- Foozle 원본의 프레임 방향·수명 단계 재확인 후 가로 시트 준비. 비행 중 소멸 프레임까지 반복하지 않도록 비행/종료 클립 분리. 정확한 프레임 구간은 에셋 확인 후 결정
- HitEffect는 피해 없는 GameObject. Collider 비활성, 비반복 클립 종료 후 Pool 반환
- Animator는 누적 시간만큼 진행하도록 보완하되 큰 deltaTime에서 유한 계산 유지. E/R 피해 시각은 렌더 프레임이 아닌 공격 경과 시간으로 판정
- RenderLayer에 투명 스킬/효과 레이어 추가. AddGameObject의 깊이·용량 배열, Application의 Depth ON/OFF 출력 경로를 함께 수정
- AlphaBlend 스킬은 Cutout Render Queue 재정렬에서 제외. 고정 슬롯 출력 순서를 유지하며 생성 시각 기준 정렬은 이번 범위 밖
- 신규 파일을 게임 vcxproj와 filters의 Combat / Effects / Data에 등록. 외부 UI 라이브러리 추가 없음

필수 HUD: Level·현재/필요 EXP·진행 바와 Ctrl/QWER별 단축키·쿨타임 중 잔여 초·잠금 중 해금 조건 표시. 기존 UIFrameData → UIManager → Renderer 경로 확장. 입력과 HUD는 공격 정의 및 공통 GetAvailability 참조. 생성 정사각형 아이콘·자물쇠·회전 쿨타임은 추가 요구로 포함. 상세 데이터 흐름은 [성장 시스템 설계](ProgressionDesign.md) 참고.

## 10. 파일별 작업 목록

| 파일 | 예정 변경 |
| --- | --- |
| `Game/Combat/PlayerAttack.h/.cpp` 신규 | 공통 TryUse·쿨타임·Reset 및 일반 공격 어댑터 |
| `Game/Combat/SkillAttacks.h/.cpp` 신규 | ProjectileAttack(Q/W), GroundAreaAttack(E), PeriodicAttack(R) |
| `Game/Combat/SkillObject.h/.cpp` 신규 | 공격 인스턴스의 이동·수명·판정·렌더 정보 |
| `Game/Combat/PoolHandle.h`, `WorldSlotPool.h` 신규 | 타입별 Handle과 비소유 슬롯 Pool |
| `Game/Combat/Damage.h` 신규 | DamageRequest / DamageResult / DamageReaction |
| `Game/Combat/AttackInput.h` 신규 | 프레임 입력과 눌림 판정, Reset 시 입력 동기화 |
| `Game/Effects/HitEffect.h/.cpp` 신규 | 효과 재생·반환과 렌더 정보 |
| `Game/Player.h/.cpp` | 공격 배열 소유, SkillCast, 명령 소비·쿨타임·Reset |
| `Game/Monster.h/.cpp`, `Game/Entity/Character.h/.cpp` | 피해 반환 계약·주기 피해·SpawnSerial |
| `Game/World/GameWorld.h/.cpp` | Pool 생성·스킬 갱신·충돌·반환·재시작·명중 효과 연결 |
| `Game/Collision/AABB.h` 또는 신규 SweptAABB 파일 | 상대 이동 충돌 시각 계산 |
| `Game/Animation/Animator.cpp` | 긴 프레임의 재생 시간 처리 |
| `Game/Data/GameData.h/.cpp`, `Assets/Data/GameData.json` | schema 2 공격 정의·Pool 설정·검증 |
| `Engine/Graphics/SpriteId.h`, `Engine/Core/Application.cpp` | 신규 에셋 ID, 투명 레이어 렌더 경로 |
| `Assets/Textures/Skills/`, `Assets/Textures/Effects/` 신규 | 사용 시트·라이선스·출처 기록 |
| `DX11ScrollRPG.vcxproj/.filters` | 신규 파일 등록. 별도 서버 프로젝트 추가 없음 |

경로와 클래스명은 구현 시 기존 스타일에 맞춰 조정 가능. 일반 Entity 계층·기존 Monster/Item 소유권을 일괄 재편하지 않음.

## 11. 요구사항·수용 기준

기존 Requirements.md의 ATK / LIFE / ASSET / DATA를 아래 구현 단위로 구체화. 제안 정책은 사용자 검토 전까지 미확정.

| ID | 요구사항 | 통과 조건 |
| --- | --- | --- |
| C-01 | 일반 공격 유지 | Ctrl 공격·유효 프레임·단일 성공 명중·좌우 판정 유지 |
| C-02 | 공통 공격 구성 | Player 소유 슬롯 5개, Q/W는 같은 행동 클래스에 다른 정의 주입 |
| C-03 | 입력 중복 방지 | 누름 유지·동시 입력·비활성 창·Reset 시 의도하지 않은 재발동 없음 |
| C-04 | 쿨타임 | Q/W/E/R 3/5/7/30초 이전 재발동 거부, 성공한 생성에만 소비 |
| C-05 | 사용 상태 | 사망·넉백·공중·공격 중 거부, 실패 시 상태와 쿨타임 유지 |
| C-06 | Q | 좌우 발사·가장 가까운 충돌·첫 접촉 반환·수명 종료 |
| C-07 | W | 관통·Monster 사용 회차별 1회 시도·재대여 후 이력 초기화 |
| C-08 | E | 전방 Ground 배치·유효 구간만 판정·Ground 부재 시 실패 |
| C-09 | R | 지정 틱별 피해, Hurt 중 피해 가능, 경직 반복 없음, 틱 중복 없음 |
| C-10 | 피해 결과 | Ignored/Applied/Killed 구분, 성공 피해만 명중 효과와 성공 기록 |
| C-11 | Pool 고갈 | 공격 Pool 고갈 시 미발동·미소비, 효과 Pool 고갈 시 피해 유지 |
| C-12 | Handle 조회 | 잘못된 저장소·슬롯·세대·반환된 Handle 모두 조회 실패 |
| C-13 | 재사용·Reset | 동일 슬롯 재대여와 World Reset 후 이전 Handle 부활 없음 |
| C-14 | 세대 고갈 | generation/storageId/SpawnSerial 최대값에서 재사용 충돌 없이 실패·폐기 |
| C-15 | 소유권 | 객체는 gameObjects_만 소유, Pool은 참조만 관리, 순회 중 삭제·재대여 없음 |
| C-16 | 명중 이력 | 같은 Monster 주소의 Respawn을 이전 생명과 구분 |
| C-17 | 프레임 변화 | 30/60/144 FPS와 큰 deltaTime에서 투사체 통과·E 구간 누락·R 틱 중복 없음 |
| C-18 | 효과 수명 | 위치·Animator 초기화, 1회 재생 후 반환, 피해 기능 없음 |
| C-19 | JSON | 재컴파일 없이 조정, 잘못된 설정은 파일/필드 오류, Worker가 게임 객체에 접근하지 않음 |
| C-20 | 렌더 | 좌우·투명 배경·크기 정상, Depth ON/OFF에서 새 레이어 누락 없음 |
| C-21 | Reset·사망·Stress | 활성 공격·효과·요청 정리, R 재시작 직후 궁극기 오발 없음 |
| C-22 | 기존 동작 | 이동·점프·일반 공격·피격·사망·Monster Respawn·Item 수집 회귀 없음 |

## 12. 구현 순서와 검증

| 순서 | 구현 | 검증 후 다음 단계 진입 |
| --- | --- | --- |
| 1 | 피해 결과 반환 + 입력 요청 + PlayerAttack/일반 공격 분리 | 기존 Ctrl 공격·피격·Reset 재현 |
| 2 | 고정 슬롯 Pool·Handle·SkillObject + Q | 획득·반환·고갈·재사용·직선 충돌·쿨타임 |
| 3 | W 관통 + E 지면 영역 + HitEffect | 대상별 이력·유효 시간·효과 초기화·반환 |
| 4 | R 주기 피해 + Hurt 정책 | 틱 수·타격 간격·경직·죽음·R 입력 충돌 |
| 5 | JSON/에셋/렌더 경로 통합 마무리 | 로더 실패·투명 출력·수치 조정 확인 |
| 6 | 통합 회귀·코드 설명·기록 | C-01~22의 실행 증거와 미실행 항목 분리 |

기존 단계에 앞서 Stat·성장 JSON·Spawn/EXP 지급 경로 구현. JSON과 에셋 연결은 2단계 Q부터 함께 진행하며 5단계는 전체 일관성 검증. 마지막 단계에 성장·공격 HUD와 G-01~14·H-01~06 검증 포함. 성장·HUD를 포함한 추정 작업량은 약 14~20시간이며 자동 완료 약속이 아님. 시간이 부족하면 미완료 항목을 이월하고 IOCP를 병행하지 않음.

검증 방법:

- Debug / Release x64 빌드. 실제 로더·Pool·공격 함수를 호출하는 재현 검증 사용, 이전 JobSystemTests 프로젝트 복원 없음
- 시간 검증은 Sleep 대신 명시적 deltaTime 입력. 쿨타임 직전·도달 시점, R 틱 수 비교
- Pool 용량 1로 강제 고갈, 슬롯 A 반환 후 B 재대여, 이전 Handle 조회 실패·중복 반환·Reset·최대 세대 주입
- Q 경로의 먼 적을 목록 앞에 배치해도 가까운 적에 먼저 닿는지 확인. 이동하는 적·고속 투사체·수명 마지막 구간 확인
- W/E 접촉 이력과 Monster Respawn, R Hurt/Dead 전환, 효과 Pool 고갈 시 피해 유지 확인
- 실제 게임에서 QWER·Ctrl·좌우·점프·피격·죽음·R Reset·아이템 수집을 육안 확인. Profiler 성능 재측정은 별도 요구하지 않음
- 게임 판정의 경계 검증은 이번 기능 정확성 확인이며 기존 렌더링 비교의 경계 조건 기록 제외와 구분

## 13. 구현 전 선택할 사항

1. 위 Q/W/E/R 동작과 초기 수치, 지상 전용·시전 중 이동 차단 정책
2. Q가 피해 거부 대상에 닿아도 소멸하는 접촉 정책, W/E의 1회 시도 정책
3. R의 Hurt 우회 피해와 무경직 정책, 사망 시 모든 활성 공격 회수
4. 64/128 Pool 용량과 고갈 정책
5. 최대 레벨·필요 EXP·Enemy Spawn 레벨·Reset 성장 초기화 정책. 성장·공격 HUD 포함은 확정

확정된 키·쿨타임·Pool·Handle·JSON 요구는 유지. 위 선택은 추천 기본안이며 실제 구현 요청 시 사용자 지시를 우선해 반영.
