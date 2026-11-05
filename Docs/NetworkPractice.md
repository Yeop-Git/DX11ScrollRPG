# TCP·UDP 네트워크 실습 기록

작성일: 2026.11.05. 오늘 작업 종료 시점의 코드와 대화상 실행 근거 기준.

## 문제

면접 전 Winsock의 초기화·소켓 수명·TCP/UDP 차이를 직접 이해하고 최소 Echo 왕복을 확인하려는 실습. 게임 클라이언트와 별도 서버를 같은 솔루션에서 관리하고, 서버는 향후 확장 가능한 `Server` 이름으로 구성.

## AI 활용

- thread의 join·조건 변수 wait, Job Queue 잠금 범위, promise/future 준비 확인, 데이터 레이스와 실행 순서 문제 설명.
- Winsock API·소켓 소유권·책임 분리 비교와 채팅 참고 코드 제공. 실제 소스·프로젝트 설정은 사용자 작성.
- 서버 초기화 실패 조건 안에 정상 경로가 들어가 출력 없이 종료되던 코드 확인. 사용자가 조건문 구조 수정 후 실행 성공 보고.
- AI가 게임 두 개 실행·콘솔 테스트·전용 스레드 등으로 범위를 넓혔고, 사용자가 게임 클라이언트와 별도 서버 구조로 방향을 바로잡음.

## 내 판단

- 기존 게임을 클라이언트, 루트 `Server/` 프로젝트를 별도 콘솔 서버로 선택.
- IOCP는 후속 선택이며 현재 구현 목표에 포함하지 않음.
- 다음 작업은 전용 스레드 추가 대신 기존 Job Queue의 일회성 Echo Job 사용. 장기 수신 루프를 Pool에 넣는 설계와 구분.
- 오늘은 현재 작성 상태를 보존하고 종료. 덤프 수집·분석 경험은 미진행으로 유지.

## 구현

| 파일 | 현재 역할 |
| --- | --- |
| `Engine/Network/NetworkSystem.h/.cpp` | WSAStartup·WSACleanup, 중복 초기화·정리 방지, 복사 금지. 현재 스레드·소켓 소유 없음 |
| `Engine/Network/TcpSocket.h/.cpp` | SOCKET 하나 소유, Create·Close·Bind·Listen·Accept·Connect·SendAll·Receive. 소멸 시 닫기, 복사 금지 |
| `Engine/Network/UdpSocket.h/.cpp` | 파일만 준비. 두 파일 모두 NetworkSystem 선언 복사 상태이며 UDP 구현 아님 |
| `Server/Server.cpp` | Winsock 초기화 → 127.0.0.1:50000 Bind·Listen → 연결 하나 Accept → 받은 바이트 Echo → 상대 종료 시 반환 |
| `Server/Server.vcxproj/.filters` | 서버 콘솔 프로젝트와 기존 네트워크 파일 참조 |
| `DX11ScrollRPG.slnx`, 게임 `.vcxproj/.filters` | 서버 프로젝트 등록, 네트워크 파일·필터 등록 및 XML 서식 변경 |

Accept는 새 통신 소켓의 핸들을 빈 TcpSocket 객체에 넘김. 서버의 지역 객체는 통신 소켓 → 대기 소켓 → Winsock 순서로 정리. SendAll은 부분 송신을 반복 처리하고 Receive는 바이트 수·정상 종료 0·오류를 구분. 서버 출력은 널 종료를 가정하지 않고 수신 길이를 사용. 현재 Bind·Connect는 localhost 전용.

Visual Studio에서는 Server.cpp의 초기화 실패 분기와 Echo 루프, TcpSocket의 소유권 이전·부분 송신, NetworkSystem의 정리 순서를 확인. 기존 네트워크 소스는 복사 없이 두 프로젝트에서 각각 컴파일.

## 검증과 결과

- 사용자 확인: 초기화 실패 분기 오류 수정 후 서버 실행 성공. 대화상 Bind·Listen 단계까지 확인.
- 코드 확인: Accept·Connect·SendAll·Receive 및 서버 Echo 루프 작성 상태 확인.
- 미확인: 실제 Accept 성공, 최신 코드 전체 빌드, 게임에서 TCP 문자열 왕복, UDP 왕복, 통신 중 종료·실패 검증.
- Codex는 이번 마감 작업에서 빌드·통신 테스트를 실행하지 않음. 과거 단계 실행 확인을 최신 Echo 성공으로 확대하지 않음.

## 다음 구현 순서

1. 게임에서 NetworkSystem 초기화. 기존 JobSystem 시작 이후 JSON Job 다음에 TCP Echo Job 제출.
2. Job 내부에서 지역 TcpSocket 생성, Connect → 문자열 전체 전송 → 보낸 길이만큼 응답 누적 → 내용 비교·로그 → 소켓 정리.
3. 현재 Start(1)은 JSON과 Echo를 순차 실행. 여러 Worker 사용은 별도 선택이며 자동 변경하지 않음.
4. 연결·송수신 대기 제한을 먼저 마련. SO_RCVTIMEO·SO_SNDTIMEO만으로 connect 시간까지 제한되지 않음. 무한 대기로 Stop·join이 막히지 않도록 설계·검증.
5. JobSystem을 먼저 종료·join하고 NetworkSystem을 정리. JobSystem 마지막 멤버 선언 유지, 명시적 Shutdown으로 순서를 뒤집지 않음. Job의 참조 캡처 수명 확인.
6. UdpSocket 복사 상태를 사용자 직접 정리하고 데이터그램 송수신 구현. 고정 좌표로 왕복 확인 후 메인 스레드가 플레이어 순서 번호·X/Y를 복사해 Job에 값으로 전달.
7. UDP 데이터 형식·크기·좌표 유효성·응답 없음·순서 번호 검사. Worker의 Player 직접 접근 및 응답에 따른 플레이어 위치 변경은 이번 범위에 포함하지 않음.

결과를 메인에서 사용할 필요가 생기면 promise/future로 전달. 이번 최소 검증은 로그 비교로 시작 가능. 게임 상태 공유·반복 작업 Queue·IOCP·다중 클라이언트 관리는 후속 범위.

## 한계

- 현재 블로킹 통신의 대기 제한·취소 없음. 서버의 accept·recv는 상대를 계속 기다릴 수 있음.
- TCP 서버 바이트 Echo는 작성됐지만 전체 문자열 왕복 및 일반 메시지 프레이밍은 미검증.
- SendAll은 유효 버퍼와 양수 길이 계약을 전제로 하며 잘못된 인자 방어는 추가 검토 대상.
- UDP 파일은 미완성으로 보존. NetworkSystem.h와 UdpSocket.h를 함께 포함하면 클래스 재정의 위험이 있으므로 구현 전에 수정 필요.
- 게임 Application과 네트워크의 연결 및 일회성 Job 제출은 아직 없음. 별도 testThread와 join을 추가하는 앞선 채팅 예시는 채택하지 않음.
- 덤프 실습 미진행. 내일 면접 전 완료를 전제하지 않음.
