# BlackFriday (프로젝트) - 협업/에셋 관리 규칙

이 문서는 **Unreal Engine 프로젝트에서 콘텐츠(에셋) 폴더를 어떻게 나누고**, 팀원이 **어떻게 작업→승격(Promote)→공유**하는지에 대한 최소 규칙입니다.  
목표는 딱 하나: **참조 오류 / 중복 제작 / 충돌**을 줄이고, 찾기 쉬운 프로젝트 만들기.

---

## 1) 폴더 철학 (핵심 3줄)

- `Content/Colab/<개인>` : **작업대(WIP/실험/테스트)**. 깨져도 OK  
- `Content/BlackFriday/...` : **게임 본체(확정본)**. 다른 사람이 써도 되는 상태만  
- `Content/PublicAsset/...` : **프로젝트 공용 라이브러리(재사용/공유)**

> 원칙: **최종 게임 참조는 `BlackFriday` 또는 `PublicAsset`에서만** 걸기  
> `Colab` 참조는 프로토타입 단계에서만 허용(최종 전에는 승격)

### Content 배치 규칙 (Colab / PublicAsset / BlackFriday)
1) `Content/Colab/<이니셜>/...` : 개인 작업대(WIP/실험/테스트). 여기 있는 건 언제든 바뀌거나 삭제될 수 있음.  
2) `Content/PublicAsset/...` : 팀 공용 “재료”만(공용 머티리얼/텍스처/메쉬/VFX/폰트 등). 작업중(WIP) 투척 금지.  
3) `Content/BlackFriday/...` : 게임 본체(실제 플레이에 쓰는 최종 BP/맵/UI/시스템). 최종 참조는 여기서만.  
4) 드라이브에서 받은 파일은 **WIP면 Colab**, **재료면 PublicAsset**, **게임에 바로 쓰는 최종이면 BlackFriday**에 둔다.  
5) 이동/리네임은 **언리얼 Content Browser에서만** 하고, 끝나면 **Fix Up Redirectors + Save All** 필수.  
6) 폴더 정리/대규모 이동(리팩터링)은 **정해진 담당자/정해진 시간**에만 진행한다(그 시간엔 해당 폴더 수정 금지).

---

### 템플릿 자산 사용 규칙
- 템플릿 맵/기본 캐릭터는 **원본을 직접 수정하지 않습니다.**
- 필요한 경우:
  1) 복사본을 만들어 작업하거나(예: `BP_ThirdPersonCharacter_DEV`)
  2) 새 맵/새 캐릭터를 `BlackFriday` 경로에 생성해서 사용합니다.
- 원본 수정이 필요하면 사전 합의 후 담당자가 처리합니다.

---

## 2) “승격(Promote)” 규칙 (참조 오류 줄이는 핵심)

### ✅ 작업 흐름
1. 개인 폴더(`Colab/<개인>`)에서 작업
2. “게임에 들어갈 수준”이면 `BlackFriday/...` 또는 `PublicAsset/...` 로 **이동**
3. 이동 후 필수:
   - 대상 루트에서 **Fix Up Redirectors**
   - **Save All**
4. 커밋/PR 전:
   - 의존 에셋(머티리얼/텍스처/VFX/사운드) 누락 없는지 확인
   - 최소 1회 PIE 또는 해당 맵 로드 확인

> 금지: **윈도우 탐색기에서 .uasset/.umap 이동/이름변경**

---

## 3) 네이밍 규칙 (최소 프리픽스)

- `BP_` Blueprint Actor
- `WBP_` Widget Blueprint
- `BPI_` Blueprint Interface
- `BFL_` Blueprint Function Library
- `SM_` Static Mesh / `SK_` Skeletal Mesh
- `M_` Material / `MI_` Material Instance
- `T_` Texture
- `NS_` Niagara System / `NE_` Niagara Emitter
- `S_` Sound / `SC_` SoundCue
- `DA_` DataAsset / `DT_` DataTable

권장 예: `BP_CartPawn`, `BP_Item_Banana`, `WBP_ResultScreen`, `DA_Item_Tomato`

---

## 4) 맵 협업 규칙 (충돌 방지)

- `Maps/Main/*` 은 **담당자(Owner) 지정**
- 같은 메인 맵을 동시에 수정하지 않기
- 개인 테스트는 `Colab/<개인>/TestMaps` 또는 `Maps/Test`

---

## 5) Git / PR 기본 규칙

- `main`(또는 `develop`)은 **항상 실행 가능 상태**
- 개발은 `feature/<기능명>` 브랜치
- PR에는:
  - 변경 요약(3줄)
  - 게임플레이 영향/리스크
  - 스크린샷/짧은 영상(레벨/에셋이면 강추)

대형 바이너리 관리는 **Git LFS 필수**(별도 설정 문서로 관리)

---

## 6) PR 체크리스트

- [ ] Content Browser에서만 이동/리네임했다
- [ ] Fix Up Redirectors 했다
- [ ] Save All 했다
- [ ] 의존 에셋 누락 없이 커밋했다
- [ ] PIE/맵 로드로 정상 동작 확인했다
- [ ] (맵 변경 시) 담당자와 충돌 없는지 확인했다

---

## 7) 승격 예시

- 작업: `Content/Colab/SERINN/WIP/BP_Item_Banana`
- 승격: `Content/BlackFriday/BP/Items/BP_Item_Banana`
- 공용 재질: `Content/PublicAsset/Art/Materials/MI_Toon_Base`

---

## 8) 한 줄 요약

**Colab는 작업대, BlackFriday는 게임 본체, PublicAsset은 공용 라이브러리.**  
옮길 땐 에디터에서만, 옮긴 뒤 Redirector 정리 + Save All.
