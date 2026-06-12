# DataMonitor

실시간 데이터 상태를 콘솔에서 조회하는 관리자용 모니터링 도구 (PoC).

---

## 화면 구성

```
+---------------------------------------------------------------------------------+
|  DataMonitor v1.0  |  2026-06-12 10:30:45  |  Refresh: 2s  (next: 1s)          |
+---------------------------------------------------------------------------------+
|  Filter: [ALL]  Sort: [KEY]  Page: 1 / 2  Entries: 24                          |
+-----+----------------------------+-----------------+---------+-------+----------+
| ID  | KEY                        | VALUE           | TYPE    | STS   | UPDATED  |
+-----+----------------------------+-----------------+---------+-------+----------+
|   1 | api.error_rate             | 0.3 %           | METRIC  | OK    | 10:30:43 |
|   2 | api.req_per_sec            | 320             | COUNTER | OK    | 10:30:43 |
|   3 | cache.hit_rate             | 87.3 %          | METRIC  | OK    | 10:30:43 |
|   4 | queue.msg.depth            | 1523            | GAUGE   | WARN  | 10:30:44 |
|   5 | sys.cpu.usage              | 91.2 %          | METRIC  | ERR   | 10:30:44 |
+---------------------------------------------------------------------------------+
|  Status: OK:19   WARN:3   ERR:2                                                  |
+---------------------------------------------------------------------------------+
|  [Q]Quit  [R]Refresh  [P]Pause  [F]Filter  [S]Sort  [+/-]Interval  [Up/Dn]Scroll |
+---------------------------------------------------------------------------------+
```

상태별 색상:
- **초록 (OK)**: 정상 범위
- **노랑 (WARN)**: 경고 임계값 초과
- **빨강 (ERR)**: 에러 임계값 초과

---

## 빌드

### 요구사항
- Visual Studio 2022 (v145 toolset)
- Windows 10 / 11

### Visual Studio에서 빌드
1. `DataMonitor.slnx` 열기
2. 구성: `Debug | x64` 선택
3. **빌드 > 솔루션 빌드** (Ctrl+Shift+B)

### MSBuild 명령줄 빌드

```bat
:: Visual Studio 개발자 명령 프롬프트에서 실행
msbuild DataMonitor.vcxproj /p:Configuration=Debug /p:Platform=x64

:: Release 빌드
msbuild DataMonitor.vcxproj /p:Configuration=Release /p:Platform=x64
```

빌드 결과물: `x64\Debug\DataMonitor.exe` 또는 `x64\Release\DataMonitor.exe`

---

## 실행

```bat
x64\Debug\DataMonitor.exe
```

콘솔 창이 자동으로 100×45 크기로 조정됩니다.

---

## 키보드 단축키

| 키 | 동작 |
|---|---|
| `Q` | 종료 |
| `R` | 즉시 강제 새로고침 |
| `P` | 자동 새로고침 일시정지 / 재개 |
| `F` | 필터 순환: ALL → OK → WARN → ERR → ALL |
| `S` | 정렬 순환: KEY → STATUS → TYPE → TIME → KEY |
| `+` / `=` | 새로고침 간격 1초 증가 (최대 60초) |
| `-` | 새로고침 간격 1초 감소 (최소 1초) |
| `↑` / `↓` | 항목 선택 이동 |
| `Page Up` / `Page Down` | 페이지 단위 이동 |
| `Home` | 첫 번째 항목으로 이동 |
| `End` | 마지막 항목으로 이동 |

---

## 프로젝트 구조

```
DataMonitor/
├── DataMonitor.slnx          # Visual Studio 솔루션
├── DataMonitor.vcxproj       # MSBuild 프로젝트
├── README.md
└── src/
    ├── main.cpp              # 진입점, 콘솔 초기화
    ├── DataStore.h / .cpp    # 데이터 모델 및 시뮬레이션 엔진
    ├── ConsoleUI.h / .cpp    # Windows Console API 기반 렌더러
    └── Monitor.h / .cpp      # 메인 루프, 입력 처리, 갱신 주기 관리
```

---

## 모니터링 항목 (PoC 시뮬레이션)

| 키 | 타입 | 경고 임계값 | 에러 임계값 | 설명 |
|---|---|---|---|---|
| `db.connections.active` | GAUGE | 70 | 90 | 활성 DB 연결 수 |
| `db.query.avg_ms` | METRIC | 500ms | 1000ms | 평균 쿼리 응답시간 |
| `cache.hit_rate` | METRIC | 70% ↓ | 50% ↓ | 캐시 히트율 (낮을수록 나쁨) |
| `queue.msg.depth` | GAUGE | 1000 | 3000 | 메시지 큐 적재량 |
| `sys.cpu.usage` | METRIC | 75% | 90% | CPU 사용률 |
| `sys.mem.usage` | METRIC | 80% | 95% | 메모리 사용률 |
| `api.response_time_ms` | METRIC | 500ms | 1000ms | API 평균 응답시간 |
| `api.error_rate` | METRIC | 1% | 5% | API 에러율 |
| `replication.lag_ms` | METRIC | 50ms | 200ms | DB 복제 지연 |
| `app.version` 등 | CONFIG | - | - | 정적 설정값 |

---

## 실제 데이터 소스 연동 (확장 방법)

`DataStore::Update()` 및 `DataStore::InitEntries()` 를 수정하여 실제 데이터 소스와 연동합니다.

```cpp
// DataStore.cpp - Update() 내부에서 실제 데이터 조회
void DataStore::Update() {
    // 예: 데이터베이스 조회
    // entries_[0].numericVal = QueryDB("SELECT COUNT(*) FROM connections");
    
    // 예: 파일 읽기 (공유 메모리, named pipe 등)
    // entries_[1].numericVal = ReadSharedMemory("cache_hit_rate");
    
    // 예: HTTP API 호출
    // entries_[2].numericVal = FetchMetric("http://localhost:9090/metrics");
    
    for (auto& e : entries_)
        if (e.isNumeric) { RefreshValue(e); UpdateStatus(e); }
}
```
