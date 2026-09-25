# Profiler Baseline

## 측정 조건

| 항목 | 값 |
| --- | --- |
| 빌드 구성 | Release x64 |
| 게임 해상도 | 1280 × 720 |
| VSync | 켬 (`Present(1, 0)`) |
| 평균 구간 | 최근 최대 120 프레임 |
| 실행 PC / CPU / GPU | 미기록 |
| 측정 날짜 | 2026.09.25 |

각 테스트 버튼을 누른 뒤 생성 프레임이 평균에 포함되지 않도록 기록이 초기화됩니다. 표시값이 안정되도록 충분히 실행한 뒤 값을 옮겨 적습니다. 모든 개체 수는 같은 PC와 그래픽 설정에서 측정해야 비교할 수 있습니다.

## 측정 결과

시간 값은 ms, FPS 및 개수 항목은 평균 프레임당 값입니다. `AABB 검사`에는 Ground, 전투, 아이템 및 스트레스 Player 대 Monster 검사가 포함됩니다. `Stress Overlap`은 스트레스 Monster와 Player 몸체가 겹친 횟수의 프레임 평균입니다.

| 조건 (Stress Monster) | FPS | Frame ms | Update ms | Physics ms | Collision ms | Scene Render ms | UI Render ms | Present ms | Entity 수 | Sprite 수 | Draw Call 수 | AABB 검사 수 | Stress Overlap |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 일반 플레이 (0) | 60.3 | 16.57 | 0.03 | 0.00 | 0.01 | 0.04 | 0.01 | 16.37 | 2 | 23 | 23 | 31 | 0 |
| 100 | 60.3 | 16.59 | 0.24 | 0.00 | 0.21 | 0.12 | 0.01 | 16.14 | 101 | 122 | 122 | 1,615 | 6 |
| 500 | 60.3 | 16.59 | 1.05 | 0.01 | 0.97 | 0.46 | 0.01 | 14.83 | 501 | 522 | 522 | 8,015 | 30 |
| 1000 | 60.3 | 16.58 | 2.16 | 0.03 | 2.04 | 1.00 | 0.01 | 13.25 | 1,001 | 1,022 | 1,022 | 16,015 | 63 |
| 5000 | 58.8 | 17.00 | 9.82 | 0.28 | 9.13 | 4.98 | 0.01 | 1.40 | 5,001 | 5,022 | 5,022 | 80,015 | 304 |
| 10000 | 36.1 | 27.73 | 19.11 | 0.47 | 17.97 | 8.44 | 0.01 | 0.08 | 10,001 | 10,022 | 10,022 | 160,015 | 572 |
| 50000 | 7.1 | 140.21 | 98.77 | 2.50 | 93.06 | 41.04 | 0.01 | 0.10 | 50,001 | 50,022 | 50,022 | 800,015 | 3,009 |

## Sprite Batch 비교 측정 목표

기존 스크린샷의 100 / 500 / 1000 / 5000개 결과는 위 기준선으로 보존합니다. Profiler 버튼을 1000 / 5000 / 10000 / 50000개로 변경했으므로 이후 Sprite Batch 전후 비교는 다음 조건을 사용합니다.

| Stress Monster | Before Draw Calls | After Draw Calls | 감소율 | Before Scene Render ms | After Scene Render ms | Render 변화 | Before FPS | After FPS | Before Frame ms | After Frame ms |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1000 | 1,022 | 6 | 99.4% | 1.00 | 1.85 | +85.0% | 60.3 | 61.1 | 16.58 | 16.36 |
| 5000 | 5,022 | 8 | 99.8% | 4.98 | 6.60 | +32.5% | 58.8 | 60.2 | 17.00 | 16.61 |
| 10000 | 10,022 | 10 | 99.9% | 8.44 | 13.57 | +60.8% | 36.1 | 30.0 | 27.73 | 33.35 |
| 50000 | 50,022 | 30 | 99.9% | 41.04 | 66.97 | +63.2% | 7.1 | 6.0 | 140.21 | 166.11 |

기준선 캡처는 [Profiler/Baseline](Profiler/Baseline/)에, Batch 적용 후 캡처는 [Profiler/SpriteBatchAfter](Profiler/SpriteBatchAfter/)에 원본 PNG로 보관합니다: [After 1000](Profiler/SpriteBatchAfter/Stress_1000.png), [After 5000](Profiler/SpriteBatchAfter/Stress_5000.png), [After 10000](Profiler/SpriteBatchAfter/Stress_10000.png), [After 50000](Profiler/SpriteBatchAfter/Stress_50000.png).

## Depth Test ON/OFF 비교

동일한 Batch 및 화면 구성에서 Profiler 체크박스로 Depth Test만 전환해 캡처했습니다. 노란 테두리는 Render 시간, Draw Call 수, GPU 시간, PS 호출 수를 표시합니다. 편집된 비교판은 [DepthTestComparison.png](Profiler/DepthTest/DepthTestComparison.png), 원본은 [ON 캡처](Profiler/DepthTest/On/) 및 [OFF 캡처](Profiler/DepthTest/Off/)에 둡니다.

| Stress Monster | GPU ms (Depth ON → OFF) | PS Invocations (ON → OFF) | Scene Render ms (ON → OFF) | Frame ms (ON → OFF) | Draw Calls (ON → OFF) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1,000 | 0.36 → 0.28 | 761,804 → 8,902,340 | 1.39 → 2.16 | 16.60 → 16.59 | 6 → 6 |
| 5,000 버튼 선택 (실제 Stress 500) | 0.58 → 1.25 | 1,675,407 → 38,882,099 | 6.93 → 8.97 | 16.83 → 18.62 | 8 → 8 |
| 10,000 | 0.74 → 2.27 | 2,063,266 → 78,192,054 | 13.74 → 18.00 | 33.68 → 37.52 | 10 → 10 |
| 50,000 | 1.67 → 11.46 | 29,449,766 → 376,943,225 | 67.88 → 89.77 | 167.68 → 189.64 | 30 → 30 |

50,000 구간에서는 Depth Test ON 시 GPU 측정값이 약 85% 낮고 PS invocation 수는 약 92% 감소했습니다. Scene Render도 OFF의 89.77ms에서 ON의 67.88ms로 낮게 측정됐습니다. Draw Call 수는 두 상태에서 같아, 이 비교에서 관찰된 변화는 제출 횟수가 아닌 깊이 검사에 따른 픽셀 셰이더 실행량 차이에 부합합니다. PS invocation은 overdraw 픽셀을 직접 센 값이 아니며, 1,000개 GPU 수치는 sub-ms 영역이라 프레임 간 변동에 민감합니다. 동일한 빌드와 기기에서 안정된 구간을 다시 측정해 교차 확인할 필요가 있습니다.

**측정 조건 주의:** 두 번째 ON/OFF 캡처는 버튼에 `5000`이 표시되어 있지만 Profiler에는 `Entities 501`, `Sprites 522`로 나타납니다. 따라서 화면상 실제 Stress Sprite 수를 500개로 기록했으며, 이 자료는 의도한 5,000개 측정을 대표하지 않습니다. 원본 이미지 이름도 선택 버튼과 실제 개체 수가 혼동되지 않도록 구분했습니다.

## 측정 메모

- 버튼을 누르면 게임 월드를 초기 상태로 되돌린 뒤 지정한 개수만큼 Monster를 만듭니다.
- 스트레스 Monster는 애니메이션, Physics 및 Collider 갱신 대상입니다. 중력과 이동 AI는 끄고, Player 피해와 Monster 피해 처리는 생략합니다.
- Profiler 패널은 UI Render 시간에 포함되며 자체 사각형/텍스트 Draw Call은 게임 카운터에서 제외됩니다.
- 글자 비트맵 생성과 DX11 텍스처 복사는 UI가 열린 동안 0.5초마다 프레임 기록 종료 후 수행되므로 Frame / UI Render 시간에는 포함되지 않습니다.
- Heart와 Game Over Sprite는 기존과 같이 Sprite 및 Draw Call 카운터에 포함됩니다.
- 결과를 기록할 때 창 크기, 백그라운드 프로그램 등 비교에 영향을 준 조건도 여기에 덧붙입니다.
- 위 값은 사용자가 제공한 순서(일반 플레이, 100, 500, 1000, 5000)의 스크린샷에서 옮겼습니다. Release x64 / 1280 × 720 / VSync 설정은 기존 측정 조건을 따랐다고 기록하되, 기기 사양은 확인되지 않았습니다.
- VSync가 켜져 있어 100~1000 구간의 FPS와 Frame Time은 약 60 FPS / 16.6 ms로 제한됩니다. 따라서 Sprite Batch 효과는 FPS만으로 판단하지 않고 Scene Render 시간과 Draw Call 수를 주요 지표로 비교합니다. 5000 구간은 프레임 예산을 넘기기 시작해 전체 FPS 영향도 관찰할 수 있습니다.
- 이 스크린샷의 Draw Call 수는 Sprite 수와 동일하게 증가합니다(일반 23, 100개 122, 500개 522, 1000개 1,022, 5000개 5,022). 이는 현재 개별 Sprite 제출 경로의 기준선입니다.
- 위 수치는 사용자가 제공한 Profiler 스크린샷에서 옮겼으며, CPU/GPU 모델과 캡처 당시 빌드 설정은 화면에서 확인되지 않아 미기록 상태입니다. Sprite Batch 전후 비교 시 동일한 빌드·해상도·VSync 조건을 유지하고 기기 정보를 추가 기록합니다.
- 10000개에서 FPS가 36.1, 50000개에서 7.1로 내려가고 Render가 각각 8.44ms, 41.04ms로 증가했습니다. Collision도 함께 증가하므로 전후 효과는 Draw Calls와 Scene Render를 중심으로 비교하고, 전체 프레임 개선은 보조 지표로 봅니다.
- Batch 적용 후 Draw Call은 네 조건 모두 99% 이상 감소했지만, Scene Render 시간은 32.5~85.0% 증가했습니다. 10000 / 50000 조건의 FPS도 각각 36.1→30.0, 7.1→6.0으로 낮아져 렌더 CPU 경로의 추가 비용을 조사해야 합니다. 1000 / 5000은 VSync 상한 부근이라 FPS 차이만으로 개선을 판단하지 않습니다.
