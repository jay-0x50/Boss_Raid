# Exception 게임 기획서

<aside>
<img src="https://www.notion.so/icons/target_blue.svg" alt="https://www.notion.so/icons/target_blue.svg" width="40px" />

**목표:** 3~4개월 내 “짧은 필드 탐험 + 1:1 보스 레이드” 액션 데모 완성

**핵심:** 프로그래밍 언어/개발 도구를 보스 콘셉트로 재해석하고, C++ 기반 전투 시스템으로 게임플레이와 엔진 프로그래밍 역량을 함께 증명

</aside>

## 1. 프로젝트 개요

### 1) 개발 목표

- **프로젝트명:** Exception
- **기간:** 3~4개월 (단기 데모)
- **장르/형태:** 소울라이크 스타일 **필드 탐험 + 보스 레이드** 액션
- **핵심 콘셉트:** Python, Perl, CMD/Terminal처럼 언어나 개발 도구의 아이콘/마스코트가 강한 대상을 보스 캐릭터로 구현
- **이름 의미:** Exception은 프로그래밍의 “예외”이면서, 평범한 테스트맵이 아닌 “예외적으로 강한 적과 마주치는 상황”을 함께 뜻함
- **포트폴리오 포인트**
    - 블루프린트 단독 결과물과 차별화
    - **C++ 기반의 시스템 설계/최적화/확장성**을 증명

### 1-1) 세계관/컨셉 요약

플레이어 **Hendel**은 예외를 처리하는 `handler`에서 이름을 얻은 디버거/처리자이며, 각 보스는 특정 언어 또는 데이터 시스템에서 발생한 거대한 오류 개체로 등장한다.

- **Python 계열 보스:** 두 마리 드래곤 (파이썬 로고의 파랑/노랑을 모티브). **Vethara, Unhandled Exception (파란 드래곤, 원거리/번개-얼음 속성)** 와 **Aurathos, Fatal Process (골드 드래곤, 근거리/용암-불 속성)** 로 구성. 체력바 2개 연동 방식으로 운영.
- **Perl 계열 보스:** **Vritra, Perl Nomad**. Perl 낙타 마스코트를 사막 유목형 오류 개체로 재해석하며, 정규식/해시/원라이너 콘셉트의 돌진과 장판 패턴을 사용.
- **조력자:** **Nel(null)**. 프로그래밍의 `null`처럼 실체가 비어 있는 안내자이며, 플레이어에게 로그와 경고를 전달한다.
- **CMD 계열 최종 보스:** **CMD, The First Command**. 봉인된 보스들을 깨워 The Runtime을 파괴하려는 최종 예외.

메인 진행은 **필드 → Python 듀얼 보스 → 필드 → Perl 보스 Vritra → 필드 → CMD 최종 보스** 순서로 구성한다. 기존 SQL 보스 **Selvara**는 삭제하지 않고 확장 후보/보류 보스로 남긴다.

### 2) 핵심 게임 루프 (Core Loop)

1. 필드 시작 지점/체크포인트에서 출발
2. 짧은 필드 구간을 이동하며 조작/전투 감각 적응
3. 보스방 입구 도착 → 전투 진입
4. 보스 패턴 학습 → **회피/패링**
5. **그로기(체력 외 게이지)** 유도
6. 극딜(딜 윈도우) → 페이즈 전환
7. 클리어 시 보상/길 개방 → 필드로 복귀
8. 패배 시 체크포인트에서 재도전

### 3) 디자인 목표 (플레이 경험)

- “처음엔 맞지만, 반복하면 **읽히고 대응할 수 있는**” 패턴 설계
- **패링은 고위험 고보상**, 회피는 안정적이되 리소스(스태미나) 관리가 필요
- 그로기/처형은 “보상을 확실히 주는” 전투 리듬 장치로 사용

### 4) 전체 진행 구조: 필드 → 보스 → 필드 → 보스

본 데모는 오픈월드 전체를 구현하는 것이 아니라, **짧은 필드 구간과 보스전을 반복하며 The Runtime의 깊은 레이어로 내려가는 액션 RPG 데모**를 목표로 함.

```text
필드 1: Runtime 외곽
→ Python 듀얼 보스전: Vethara + Aurathos
→ 필드 2: Perl 사막/스크립트 폐허
→ Perl 보스전: Vritra
→ 필드 3: CMD 진입 구역
→ 최종 보스전: CMD
→ 엔딩 로그
```

엔딩은 기본 엔딩과 히든 엔딩으로 나뉜다. 기본 엔딩은 CMD 처치 후 The Runtime이 안정화되는 단순 클리어 루트다. 히든 엔딩은 별도 퀘스트 UI 없이 Nel이 대사 속에서 흘리듯 부탁한 일들을 플레이어가 알아차려 모두 해줬을 때만 열린다. 이 부탁들은 수행하지 않아도 메인 진행에는 지장이 없으며, 모르면 그대로 기본 루트로 흘러간다. CMD는 아직 완전한 root authority를 얻지 못했고, 보스들을 깨워 Runtime을 불안정하게 만든 뒤 root를 탈취하려 한다. 히든 루트에서 Hendel은 자신이 CMD를 처리하기 위해 실행된 `handler` 도구이고 task 완료 후 무로 돌아가야 한다는 진실을 알게 된다. Hendel은 히든 무기 **Mimikatz, Authority Seized**로 CMD가 거의 얻어낸 root authority를 선점해 소멸을 거부하고, 결과적으로 The Runtime의 새로운 root가 된다. CMD는 죽기 직전 handler로 위장한 바이러스를 생성해 다음 이야기의 갈등을 남긴다.

#### (1) 시작 지점 / 체크포인트

- 플레이어는 휴식 포인트에서 시작
- 역할
    - 리스폰 위치 저장
    - HP/스태미나 회복
    - 보스전 패배 후 즉시 재도전
    - 필요 시 조작 연습용 더미 배치

#### (2) 짧은 필드 구간

- 목표 길이: 보스방까지 **1~3분 내**
- 역할
    - 이동/카메라/회피 조작 적응
    - 플레이어가 “필드를 지나 보스를 찾으러 간다”는 흐름 제공
    - 보스전만 따로 놓인 테스트맵 느낌을 줄임
- 구성
    - 간단한 길/지형
    - 약한 적 1~2마리 또는 함정 1개
    - 보스 실루엣, 소리, 문, 안개벽 등으로 목표 지점 암시

#### (3) 보스방 입장

- 보스문/안개벽 트리거를 통해 보스전 시작
- 입장 시 처리
    - 보스 UI 표시
    - 보스 AI 활성화
    - 전투 구역 이탈 제한
    - 보스전 시작 연출 또는 사운드 재생

#### (4) 보스전

- 본 문서의 전투 시스템을 적용
    - 패턴 학습
    - 회피/패링
    - 그로기
    - 처형
    - 페이즈 전환
- 패배 시 체크포인트로 리스폰하고 보스전 상태 초기화

#### (5) 보스 처치 후 복귀/보상

- 보스 처치 시 전투 종료
- 보상 예시
    - MaxHP +5
    - 스태미나 회복속도 소폭 증가
    - 다음 길/문 개방
- 목표: “보스 클리어 → 필드 복귀 → 다음 목표 탐색” 흐름을 데모 안에서 짧게 보여줌

### 5) 데모 스코프 기준

- 필드 3구간 + 체크포인트 + 보스방 3개를 하나의 짧은 캠페인 흐름으로 구성
- 1차 완성 우선순위는 **Python 듀얼 보스 → Perl 보스 Vritra → CMD 최종 보스** 순서
- 각 필드는 넓은 탐험보다 “다음 보스로 향하는 준비 구간”에 집중
- 전체 플레이 타임 목표: **20~30분 내**
- 실패/재도전을 포함해도 **40분 내**에 핵심 경험을 확인할 수 있게 설계

---

## 2. 기술적 가이드라인 & 시스템 구조 (C++ Core)

### 1) 객체 생명 주기 / GC 안정성

- 런타임에서 살아있는 UObject 참조는 **TObjectPtr + UPROPERTY()**로 GC 추적에 등록
- 메모리 누수 가능성을 구조적으로 차단

### 2) C++ 인터페이스 기반 디커플링

- 전투 상호작용(피격/타격/상태변환)은 **UInterface (예: IExCombatInterface)** 통해서만 전달
- 플레이어/보스가 서로의 헤더를 직접 Include 하는 **하드 레퍼런싱 제거**
- Cast To 남발을 줄여 **컴파일/런타임 비용 및 결합도 감소**

### 3) 이벤트 중심 UI (Multicast Delegate)

- 스탯(HP/스태미나/그로기) 변화는 **DECLARE_DYNAMIC_MULTICAST_DELEGATE** 기반으로 브로드캐스팅
- UMG는 **필요할 때만** 업데이트 (Event-Driven)
- Tick 기반 UI 동기화 최소화

### 4) C++ / BP 하이브리드 전략

- **C++로 구현 (코어):**
    - 스탯/상태 머신/피격 판정/데미지 처리/최적화/메모리 안정성
- **BP로 구현 (연출 레이어):**
    - 애니메이션 몽타주, SFX, Niagara VFX, 카메라 연출, UI 애니메이션
- 목표: “현업 스타일(코어는 C++, 연출은 BP)” 구조로 설득력 확보

---

## 3. 프로젝트 디렉토리 구조 (예시)

```
Source/Exception/
├── Public/
│   ├── Characters/
│   │   ├── ExBaseCharacter.h
│   │   ├── ExPlayerCharacter.h
│   │   └── ExBossCharacter.h
│   ├── Components/
│   │   └── ExStatComponent.h
│   ├── Interfaces/
│   │   └── ExCombatInterface.h
│   └── AI/
│       ├── BTTask_BossAttack.h
│       └── BTDecorator_CheckPhase.h
└── Private/
    ├── Characters/
    │   ├── ExBaseCharacter.cpp
    │   ├── ExPlayerCharacter.cpp
    │   └── ExBossCharacter.cpp
    ├── Components/
    │   └── ExStatComponent.cpp
    ├── Interfaces/
    │   └── ExCombatInterface.cpp
    └── AI/
        ├── BTTask_BossAttack.cpp
        └── BTDecorator_CheckPhase.cpp
```

---

## 4. 핵심 클래스 아키텍처 (요약)

| 클래스 | 부모 | 핵심 멤버/스펙 | 역할 |
| --- | --- | --- | --- |
| AExBaseCharacter | ACharacter | TakeDamage() override
PlayHitEffects() (BlueprintImplementableEvent) | 플레이어/보스 공통 베이스. 언리얼 기본 데미지 프레임워크 연동, 피격 연출 트리거. |
| AExPlayerCharacter | AExBaseCharacter | SetupPlayerInputComponent()
Enhanced Input 바인딩 | 입력/조작, 구르기 무적 프레임, 공격 콤보 선입력 큐 처리. |
| AExBossCharacter | AExBaseCharacter | UPROPERTY(BlueprintReadOnly) bool IsPhase2
UBehaviorTree* BossBT | 보스 페이즈/전용 로직. HP 비율 감시 → 페이즈 전환, 블랙보드 값 갱신. |
| UExStatComponent | UActorComponent | HP/Stamina/Groggy
FOnHPChangedDelegate 등 델리게이트 | 스탯 독립화(컴포넌트). 증감산 처리 + 이벤트 브로드캐스트. |
| UExCombatInterface | UInterface | UFUNCTION(BlueprintNativeEvent)
OnWeaponHit(FGameplayTag AtkType) | 타격/상호작용 통로. Cast 제거로 결합도/비용 감소. |

---

## 5. 스펙 데이터 테이블 (Character Stats)

| ID (Row) | CharacterName | MaxHP | MaxStamina | MaxGroggy | MoveSpeed | 설명 |
| --- | --- | --- | --- | --- | --- | --- |
| ST_Player | 플레이어 | 100 | 100 | 0 | 600 | 조작 캐릭터 기본 스펙 (그로기 없음) |
| ST_Vethara_P1 | Vethara (1페이즈) | 500 | 0 | 100 | 450 | 파란 드래곤 / 원거리 패턴 중심 |
| ST_Vethara_P2 | Vethara (2페이즈) | 500 | 0 | 150 | 600 | HP 50% 이하 진입 시 광폭화. 번개 장판 강화. |
| ST_Aurathos_P1 | Aurathos (1페이즈) | 500 | 0 | 100 | 400 | 골드 드래곤 / 근거리 패턴 중심 |
| ST_Aurathos_P2 | Aurathos (2페이즈) | 500 | 0 | 150 | 550 | HP 50% 이하 진입 시 광폭화. 용암 돌진 강화. |
| ST_Vritra_P1 | Vritra (1페이즈) | 560 | 0 | 140 | 165 | Perl 낙타 / 정규식 라인 견제와 돌진 중심 |
| ST_Vritra_P2 | Vritra (2페이즈) | 560 | 0 | 140 | 201 | HP 50% 이하 진입 시 광역 장판과 긴 돌진 강화 |
| ST_CMD_P1 | CMD (1페이즈) | 650 | 0 | 160 | 420 | CMD 명령어 기반 근접/좌표 공격 |
| ST_CMD_P2 | CMD (2페이즈) | 650 | 0 | 180 | 500 | Root Access 진입. 분열/입력 차단/환경 변조 |

### 체력바 연동 규칙 (하이브리드 방식)

두 보스의 체력바는 **각각 독립적으로 표시**되지만 아래 연동 규칙이 적용된다.

| 조건 | 효과 |
| --- | --- |
| 두 체력의 차이가 **30% 이상** 벌어질 때 | 체력이 많은 쪽이 **격노 상태** 진입 → 이동속도 +20%, 공격력 +15% |
| 한 마리 처치 시 | 나머지 보스 **즉시 페이즈 2 진입 + 격노 연출** (포효 + 무적 구간) |
| 처형 | 각각 독립적으로 발동 가능 (그로기 게이지 별도 관리) |

> **설계 의도:** 한 마리만 집중 공략하면 나머지가 격노해 위험이 증가. 골고루 공략하면 안전하지만 전투가 길어짐. 플레이어가 전략적 선택을 하게 유도.

---

## 6. 플레이어 액션 명세 (Player Actions)

| Action | Input | Stamina | Montage | 히트박스/노티파이 구간 | 로직/예외처리 |
| --- | --- | --- | --- | --- | --- |
| 약공격 1타 | LMB | 10 | AM_LightAtk_01 | 0.15s ~ 0.25s | 히트 시 보스 그로기 +5. AnimNotifyState로 무기 콜리전 토글. |
| 약공격 2타 | LMB (연계) | 12 | AM_LightAtk_02 | 0.12s ~ 0.22s | 1타 종료 전 입력 윈도우에 재입력 시 Input Queue로 연계. |
| 강공격 | RMB | 25 | AM_HeavyAtk | 0.40s ~ 0.55s | 가드 파괴 플래그 포함. 그로기 +20. 공격 중 턴 레이트 감속. |
| 회피 (구르기) | Space | 20 | AM_Dodge_Roll | 0.0s ~ 0.35s (무적) | 회피 시 ECharacterState::Invincible. 보스 히트 검사에서 무시. |
| 패링 (쳐내기) | F | 15 | AM_Parry | 0.0s ~ 0.15s | Normal 타입 공격만 상쇄. 보스 경직 + 그로기 +40. |

---

## 7. 보스 AI 패턴 명세 (Behavior Tree)

| Phase | Pattern | Blackboard 조건 | Trigger Range | Cooldown | 연출/로직 |
| --- | --- | --- | --- | --- | --- |
| Common | 타깃 추적 | HasTarget = True | 전체 | 0s | MoveTo로 플레이어 위치 추적. 회전 속도 보간. |
| Phase 1 | 스킬 1 | IsPhase2 = False | 근거리 (&lt; 250) | 3s | AM_Boss_Combo01. 1타 후 거리 재검사 → 2타 연계 태스크. |
| Phase 1 | 스킬 2 | IsPhase2 = False | 중거리 (250~600) | 8s | AM_Boss_Earthquake. Notify 프레임에 AOE 스폰 + C++ Radial Damage. |
| Phase 2 | 변신? | BossHP &lt;= 50% (최초 1회) | 전체 | 1회 | 노드 Abort(Self). 무적 활성화 → 포효 몽타주 → Niagara 바인딩 → P2 트리 전환. |
| Phase 2 | 스킬 3 | IsPhase2 = True | 원거리 (&gt; 500) | 6s | AM_Boss_ShadowDash. 플레이어 최종 벡터 확보 후 속도 3배 + 보간 이동, 잔상 이펙트. |

---

## 7-1. 패턴 선택 방식: “쿨타임=가능(Ready)” + “AI=실행 타이밍 판단”

일반적인 **고정 로테이션(스택/큐)** 또는 “쿨타임 종료 즉시 사용” 방식은 플레이어가 **패턴을 쉽게 외워버리는 문제**가 있음.

따라서 본 프로젝트는 아래 철학으로 보스 패턴을 운영함:

- **쿨타임은 밸런스 장치**로 유지한다 (연속 사용 상한선)
- 쿨타임이 끝났다고 **바로 사용하지 않는다**
- **AI가 상황을 보고 “지금 쓰면 맞겠다”라고 판단될 때 발동**한다

### 1) 운영 규칙(Ready 개념)

- 각 패턴은 쿨타임이 끝나면 **Ready 상태**가 됨 (사용 “가능”)
- Ready 상태의 패턴 중에서만 선택 후보를 구성
- 실제 발동은 다음 조건을 만족할 때만 수행:
    - 사거리/각도/라인오브사이트가 유리
    - 플레이어가 공격 지속/스태미나 고갈 등으로 회피 여지가 낮아 보임(추정)
    - 직전 패턴 반복을 피함(반복 페널티)

### 2) AI 선택 구조(권장: BT + Blackboard)

- Blackboard 예시 키
    - `DistToTarget` (float)
    - `Ready_Skill1`, `Ready_Skill2`, `Ready_Skill3` (bool 또는 남은 쿨타임 float)
    - `LastUsedPattern` (Name/Tag)
    - `AggroLevel` (float, 템포 조절)
    - `bTargetIsGreedy` (bool, 공격 지속/근접 유지 등을 근거로 추정)
- Selector 우선순위(예시)
    1. 페이즈 전환/특수 상태
    2. **확정 히트 확률이 높은 패턴**(예: 돌진/카운터)
    3. 거리/포지셔닝 교정 패턴
    4. 기본 압박/추적 (fallback)

### 3) 예측 가능성 낮추기(추천 장치)

- **가중 랜덤(Weighted Random):** 조건을 만족한 후보들 중 가중치로 선택
- **반복 페널티:** `LastUsedPattern` 또는 최근 N초 사용 패턴에 가중치 감소
- **AggroLevel 변조:** 플레이어가 잘 피하면 Aggro↑, 맞으면 Aggro↓ 같은 방식으로 템포 변주

### 4) 밸런스 관점에서의 쿨타임

- 쿨타임은 “AI가 사기치지 않도록” **연속 사용 상한을 두는 안전장치**
- 발동 타이밍은 AI가 결정하므로, 플레이어 입장에서는 “쿨타임이 끝나도 당장 안 나갈 수 있음” → 더 생동감 있는 전투 리듬을 만들 수 있음

## 8. 판정/데미지 매트릭스 (Combat Matrix)

| DamageID | AttackCharacter | Damage | Groggy | HitStop | CameraShake | ReactionType |
| --- | --- | --- | --- | --- | --- | --- |
| DMG_Player_Light | 플레이어 | 15 | 5 | 0.05 | CS_LightHit | Light_Hit |
| DMG_Player_Heavy | 플레이어 | 35 | 20 | 0.12 | CS_HeavyHit | Heavy_Hit |
| DMG_Boss_Normal | 보스 | 20 | 0 | 0.08 | CS_BossHit | Light_Hit |
| DMG_Boss_Smash | 보스 | 45 | 0 | 0.20 | CS_Explosion | Knockback |

---

## 9. 상태 머신 핵심 로직 (State Machine)

### 9-1) 보스 그로기 & 처형(Execution) 프로세스

1. 보스가 타격을 입을 때마다 `TakeDamage` 내부에서 전달된 `GroggyDamage` 누적
2. `CurrentGroggy >= MaxGroggy`가 되면 **실행 중 BT 태스크 Abort**
3. 보스 상태를 `ECharacterState::Groggy`로 변경하고 전용 몽타주를 루프 재생 (예: 5초)
4. 그로기 중 플레이어가 근거리에서 **상호작용 키(E)** 입력 시
    - 특수 카메라 시퀀스 + Execution 몽타주 재생
    - 처형 종료 후 그로기 0 초기화 → Idle 복귀

### 9-2) 실시간 HP 기반 페이즈 전환

1. `AExBossCharacter::TakeDamage`에서 데미지 적용 후 잔여 HP 비율 계산
2. `CurrentHP <= MaxHP * 0.5` 최초 성립 시 블랙보드 `IsPhase2 = true`
3. 데코레이터가 값을 옵저빙하여 하위 노드를 **Interrupt**하고 상위로 탈출
4. 광폭화 연출(포효/무적/이펙트) 이후 **P2 전용 서브트리**로 전환

---

## 10. (추가 제안) 데모 범위 정의 (MVP)

### 필수 (Must)

- 필드: 시작 지점/체크포인트 + 짧은 보스 접근 구간
- 플레이어: 약/강/회피/패링 + 스태미나
- 보스: Python 듀얼 보스, Perl 보스 Vritra, CMD 최종 보스의 P1/P2 패턴과 페이즈 전환
- 그로기/처형 1종
- UI: HP/스태미나/그로기 + **1체/2체 보스 체력바 표시 대응**
- 리스폰: 사망 시 체크포인트 복귀 + 보스 상태 초기화
- 보스 처치 후: 보상/문 개방/필드 복귀 중 최소 1개
- 카메라/히트스톱/피격 리액션 최소 1단계

### 선택 (Nice)

- 약한 필드 적 1~2종
- 필드 함정 1종
- 튜토리얼 메시지(패링 타이밍)
- 사운드/이펙트 폴리싱
- 리플레이성: 클리어 시간/피격 횟수 간단 기록

<aside>
<img src="https://www.notion.so/icons/checkmark_green.svg" alt="https://www.notion.so/icons/checkmark_green.svg" width="40px" />

**완성 기준(권장):** 필드에서 보스방까지 이동한 뒤, 5분 내에 ‘학습 → 적응 → 처형 성공’까지 경험 가능하고, 패배 시 체크포인트에서 빠르게 재도전할 수 있는 데모.

</aside>

## 11. 관련 문서

- [보스 콘셉트 시트](../03_보스/보스_콘셉트_시트.md)
- [보스 패턴 시트 Phase 1/2](../03_보스/보스_패턴_시트_Phase_1_2.md)
- [Vritra 보스 패턴 시트](../03_보스/보스_패턴_시트_Vritra.md)
- [Selvara 보스 패턴 시트](../03_보스/보스_패턴_시트_Selvara.md) - 보류/확장 후보
- [보스 Intro 연출 플로우](../03_보스/보스_Intro_연출_플로우.md)
- [전투 시스템 상세](../01_전투_시스템/전투_시스템_상세.md)
- [처형 Execution 시스템 상세](../01_전투_시스템/처형_Execution_시스템_상세.md)
- [카메라 락온 시스템](../02_플레이어/카메라_락온_시스템.md)
- [플레이어 피격 리액션 상세](../02_플레이어/플레이어_피격_리액션_상세.md)
- [리스폰 재진입 동선 설계](../04_월드_흐름/리스폰_재진입_동선_설계.md)
- [보스 AI 적용 설계 API 명세](../05_기술_구현/보스_AI_적용_설계_API_명세_UE_CPP.md)
- [Unreal C++ 구현 체크리스트](../05_기술_구현/Unreal_CPP_구현_체크리스트.md)
- [개발 일정 마일스톤](../06_일정_체크리스트/개발_일정_마일스톤_3_4개월.md)
