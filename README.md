# Exception

Unreal Engine 5.8 기반의 보스 레이드 액션 RPG 데모입니다.

플레이어 **Hendel**은 예외를 처리하는 `handler`에서 이름을 얻은 처리자이며, 조력자 **Nel(null)** 의 안내를 받으며 CMD가 깨운 보스들을 제압합니다. 현재 메인 흐름은 **필드 → Python 듀얼 보스(Vethara + Aurathos) → 필드 → Perl 보스(Vritra) → 필드 → CMD 최종 보스** 구조입니다.

## 핵심 방향

- C++로 전투, 보스 상태, 저장, UI 갱신 같은 코어 시스템 구현
- 블루프린트는 UI 배치, 애니메이션, VFX, 사운드, 연출 레이어에 집중
- 필드 → 체크포인트 → 보스방 → 보스전 → 보상/복귀 흐름을 갖춘 짧은 데모 완성
- 포트폴리오에서 코드 구조와 구현 과정을 설명할 수 있는 결과물 제작

## 게임 루프

```text
타이틀
→ 필드 진입
→ 체크포인트 저장
→ 보스방 접근
→ Python 듀얼 보스전
→ 그로기 누적
→ 처형
→ 보스 처치
→ 필드 복귀 / 다음 구역 개방
→ Perl 보스 Vritra
→ 필드 복귀 / CMD 구역 진입
→ 최종 보스 CMD
→ 엔딩 로그
```

엔딩은 두 갈래입니다. 기본 엔딩은 CMD 처치 후 Runtime 안정화로 끝납니다. 히든 엔딩은 별도 퀘스트 UI 없이 Nel이 대사로 흘리듯 부탁한 일들을 플레이어가 알아차려 전부 해줬을 때만 열리며, 하지 않아도 메인 진행에는 지장이 없습니다. 이 루트에서 Hendel은 히든 무기 **Mimikatz, Authority Seized**로 CMD가 거의 얻어낸 root authority를 선점하고, 소멸을 거부한 채 새로운 root로 남습니다. CMD는 죽기 직전 handler로 위장한 바이러스를 남겨 2편 떡밥을 만든다.

패배 시에는 체크포인트에서 리스폰하며, 보스전 상태는 다시 도전 가능한 상태로 초기화됩니다.

## MVP 범위

현재 MVP 기준은 아래와 같습니다.

- 필드 3구간
- 체크포인트 1개 이상
- Python 보스방 1개
- Python 듀얼 보스 2체
  - `Vethara, Unhandled Exception`
  - `Aurathos, Fatal Process`
- Perl 보스방 1개
- Perl 보스 1체
  - `Vritra, Perl Nomad`
- CMD 최종 보스방 1개
- CMD 최종 보스 1체
  - `CMD, The First Command`
- 보류 / 확장 보스
  - `Selvara, Abyssal Database`
- 플레이어 액션
  - 약공격
  - 강공격
  - 회피
  - 패링
  - 락온
  - 처형
- 보스 시스템
  - HP
  - Groggy
  - Phase 1 / Phase 2
  - 팀 단위 공격 조율
  - 보스별 패턴 구성
- UI
  - 플레이어 HUD
  - 보스 HP/Groggy HUD
  - 1체/2체 보스 HUD 대응
  - Pause Menu
  - Title Menu
- 저장
  - 체크포인트 자동 저장
  - 메뉴 수동 저장
  - Continue 로드

## 현재 구현 상황

### 플레이어

- 이동 / 카메라 조작
- HP / Stamina 관리
- 스태미나 소모 및 자동 회복
- 약공격 / 강공격
- 회피 및 무적 시간
- 패링 및 패링 성공 시 보스 Groggy 증가
- 공격 판정용 Sphere Sweep
- 피격 / 사망 / 체크포인트 리스폰
- 락온 카메라
- 보스 Groggy 상태에서 처형
- 주요 수치 변수 주석 정리

### 보스

- 공통 보스 베이스 클래스
- HP / Groggy / Dead / Groggy 상태 관리
- Phase 1 / Phase 2 전환
- Groggy 지속 시간 후 회복
- 처형 시작 / 완료 처리
- 팀 코디네이터 기반 다중 보스 공격 조율
- Python 듀얼 보스 베이스
- `Vethara`, `Aurathos` 보스 클래스
- `Vritra` 보스 클래스
- `Selvara` 보스 클래스
- 패턴 공격 전 사거리/범위 텔레그래프 표시
- 보스 아레나 트리거
- 트리거별 보스 직접 지정 / 클래스 스폰 지원
- Static Mesh / Skeletal Mesh 보스 외형 선택 지원
- 보스 사망 시 게이트/보상 액터 처리

### UI

- C++ 플레이어 HUD
  - 좌상단 HP / Stamina
  - 좌하단 Q/E/R 숏컷
- 보스 상태 HUD
  - HP Bar
  - Groggy Bar
  - 처형 가능 표시
  - 1체/2체 이상 보스 슬롯 정리 및 초기화
- C++ Pause Menu
  - Resume
  - Level Up
  - Save Game
  - Return To Title
  - Quit Game
- Title Menu 자동 표시
- 타이틀 → Continue → 필드 복귀 시 입력 모드 복구

### 저장 / 진행

- `UBRSaveGameSubsystem` 기반 저장/로드
- 현재 레벨 이름 저장
- 플레이어 위치 / HP / Stamina 저장
- 체크포인트 위치 저장
- 인벤토리 슬롯 저장
- 메뉴 수동 저장 시 현재 위치를 재시작 위치로 저장
- 체크포인트 활성화 시 자동 저장

### 인벤토리

- 슬롯 기반 인벤토리 컴포넌트
- C++ 인벤토리 UI
- 아이템 추가 / 제거
- 슬롯 이동
- 사용 가능 아이템 처리
- 인벤토리 변경 Delegate
- 저장 파일에 슬롯 데이터 포함

### 맵 / 에셋

- `L_Title`
- `L_Runtime_Prototype`
- `L_Runtime_Field`
- `WBP_TitleMenu`
- `WBP_PlayerHUD`
- `WBP_BossStatusHUD`
- `WBP_PauseMenu`

## 입력

현재 테스트 기준 입력입니다.

| 행동 | 키 |
| --- | --- |
| 약공격 | 마우스 좌클릭 |
| 강공격 | 마우스 우클릭 |
| 회피 | Left Shift |
| 패링 | F |
| 상호작용 / 처형 | E |
| 락온 | Tab |
| Pause Menu | Escape |

에디터 PIE에서는 `Esc`가 실행 종료 단축키와 충돌할 수 있으므로, 에디터 단축키를 바꾸거나 개발 중 메뉴 키를 임시 변경해서 사용합니다.

## 기술 구조

### C++ 담당

- 플레이어 전투 상태 머신
- HP / Stamina / Groggy 수치 처리
- 공격 판정
- 데미지 처리
- 보스 상태 및 페이즈
- 보스 아레나 시작/종료
- 체크포인트 / 리스폰
- 저장 / 로드
- 인벤토리 데이터
- UI 갱신용 함수와 Delegate

### 블루프린트 담당

- UI 레이아웃
- 버튼 연결
- 애니메이션 몽타주 연결
- VFX / SFX
- 보스 외형 배치
- 레벨 블록아웃 및 연출

## 주요 코드 위치

```text
Source/Exception/Player/Character/ExceptionCharacter.*
플레이어 이동, 전투, 락온, 처형, 피격, 리스폰

Source/Exception/Player/Controller/ExceptionPlayerController.*
HUD, Pause Menu, Title Menu, 입력 모드, 메뉴 저장/종료

Source/Exception/Boss/Base/BRBossBase.*
보스 공통 HP, Groggy, Phase, 처형 상태

Source/Exception/Boss/Python/BRPythonBoss.*
Python 듀얼 보스 공통 패턴 구성

Source/Exception/Boss/Selvara/BRSelvaraBoss.*
Selvara 보스 패턴 구성

Source/Exception/Boss/Perl/BRVritraBoss.*
Vritra 보스 패턴 구성

Source/Exception/Boss/Team/BRBossTeamCoordinator.*
다중 보스 공격 조율

Source/Exception/World/BRBossArenaTrigger.*
보스전 시작, 보스 클래스 스폰, HUD 표시, 보스 사망 처리

Source/Exception/World/BRCheckpoint.*
체크포인트 활성화, 회복, 자동 저장

Source/Exception/Inventory/BRInventoryComponent.*
슬롯 기반 인벤토리

Source/Exception/Save/BRSaveGameSubsystem.*
저장/로드 처리

Source/Exception/UI/BRBossStatusWidget.*
보스 HUD 갱신

Source/Exception/UI/BRPlayerHUDWidget.*
플레이어 HUD 갱신

Source/Exception/UI/BRPauseMenuWidget.*
ESC 메뉴 / 레벨업 UI

Source/Exception/UI/BRInventoryWidget.*
C++ 인벤토리 UI
```

## 사용 기술

- Unreal Engine 5.8
- C++
- Blueprint
- Enhanced Input
- UMG
- Git
- Git LFS

## 프로젝트 구조

```text
Exception/
├── Config/              # 언리얼 프로젝트 설정
├── Content/             # 에셋, 위젯, 레벨 파일
├── Source/              # C++ 소스 코드
├── 기획서/               # 게임 기획 및 개발 문서
├── Exception.uproject
├── .gitignore
└── .gitattributes
```

## Git 관리

Unreal 프로젝트 특성상 자동 생성 파일과 캐시 파일은 Git에서 제외합니다.

제외 대상 예:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`
- `.vs/`
- `.vscode/`

포함 대상 예:

- `Source/`
- `Config/`
- `Content/`
- `기획서/`
- `Exception.uproject`

`.uasset`, `.umap`, 이미지, 사운드 같은 바이너리 에셋은 Git LFS로 관리합니다.

## 다음 개발 목표

1. `L_Runtime_Field` 필드 동선 블록아웃 마감
2. Python 듀얼 보스 패턴 밸런싱
3. Vritra 블루프린트 메시/애니메이션/스케일 조정
4. Vritra 보스방 트리거와 체력바 최종 확인
5. CMD 최종 보스 기획/패턴 클래스 추가
6. 아이템 효과 및 보스 보상 연결
7. BGM / SFX 적용
8. 보스 처치 후 보상 및 다음 구역 개방 처리
9. 패키징 테스트

## 개발 기록

- Git 저장소 초기화
- Unreal용 `.gitignore` 작성
- Git LFS 설정
- 플레이어 전투 시스템 1차 구현
- 보스 HP / Groggy / Phase / 처형 시스템 구현
- 체크포인트 / 리스폰 / 자동 저장 구현
- 보스 아레나 트리거 구현
- Python 듀얼 보스 구조 추가
- 보스 HUD 1체/2체 표시 대응
- Pause Menu / Title Menu / Continue 흐름 구현
- 인벤토리 저장/로드 연동
- `L_Runtime_Field` 신규 맵 추가
- `Selvara, Abyssal Database` 보스 클래스 추가
- `Vritra, Perl Nomad` 보스 클래스 추가
- C++ HUD / C++ Pause Menu / C++ Inventory UI 추가
- 보스 클래스 기능별 폴더 분리
- 보스 트리거 클래스 스폰 및 단일 보스 체력바 관리 개선
- 보스 기본 더미 큐브 제거 및 Skeletal Mesh 외형 지원
