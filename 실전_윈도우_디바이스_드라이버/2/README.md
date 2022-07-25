# 2장 드라이버의 기본

2장에서는 윈도우 디바이스 드라이버의 주요 기술인 WDM, WDF와 관련해서 비교적 중요한 포인트를 설명한다.

## 2.1 디바이스 스택

윈도우는 PNP 운영체제다.
PNP 운영체제는 하드웨어 트리상에 루트로부터 시작해 발견되는 모든 장치들을 디바이스 노드 형태로 보관하고, 이 노드들 간의 부모와 자식 관계를 가진 연결 상태를 유지한다.

장치는 내부적으로 몇 개의 자료구조들로 구성된다.
크게 네 가지의 의미를 가지는 자료구조로 나뉘는 것을 알 수 있다.
* Upper FiDO : 상위 필터 디바이스 오브젝트(Upper Filter Device Object)를 의미한다. 이 유형의 자료구조는 FDO 자료구조의 상위에 존재한다.
* FDO : Functional Device Object의 줄임말로, 해당하는 장치의 주 기능을 의미한다. 
* Lower FiDO : 하위 필터 디바이스 오브젝트(Lower Filter Device Object)를 의미한다. 이 유형의 자료구조는 FDO 자료구조의 하위에 존재한다.
* PDO : Physical Device Object의 줄임말이다. 이것은 해당하는 장치가 존재한다는 의미를 가진다.

모든 장치는 필요충분조건으로서 FDO와 PDO를 가진다.
이외에 FDO 자료구조를 기준으로 위와 아래에 필터 디바이스 오브젝트가 존재한다.
이것은 필수 요건은 아니다.
이렇게 FDO 위, 아래에 존재하는 필터 디바이스 오브젝트는 FDO를 위, 아래에서 부연하는 역할을 수행한다.

이렇게 네 가지 의미를 가지는 자료구조 중에서 PDO는 해당하는 장치의 존재를 의미하며, 이것을 소유하고 있는 소유자는 버스 드라이버다.
버스 드라이버는 해당하는 장치를 실제로 검색해 발견하는 역할을 수행한다.

하나의 장치는 FDO, PDO 그리고 선택적으로 FiDO를 가지며, 이들은 소유하고 있는 소유자들이 함께 협력해서 이 장치를 유지한다.
이렇게 하나의 장치를 구성하는 네 가지 의미의 자료구조들의 모임을 디바이스 스택이라고 부른다.

IRP(IO Request Packet)는 명령어 패킷으로 불리는 자료구조다.
이 명령어 패킷은 디바이스 스택을 동작시키는 핵심 명령을 담는다.
디바이스 스택이 외부로부터 동작하는 데 필요한 요청을 받는다면 거의 대부분 IRP 형식으로 받는다.
IRP 자료구조는 생성될 당시에 디바이스 스택을 구성하는 디바이스 오브젝트의 개수를 고려해 만들어진다.

1. 외부 클라이언트는 IO 매니저가 제공하는 IoCallDriver() 함수를 호출한다.
2. IO 매니저는 요청을 xxxupper.sys 에게 전달 xxxuper.sys는 IoCallDriver() 함수 호출
3. IO 매니저는 요청을 xxxfunction.sys 에게 전달 xxxfunction.sys는 IoCallDriver() 함수 호출
4. IO 매니저는 요청을 xxxlower.sys 에게 전달 xxxlower.sys는 IoCallDriver() 함수 호출
5. IO 매니저는 요청을 xxxbus.sys 에게전달 

하나의 IRP가 IO 매니저가 제공하는 IoCallDriver() 함수에 의해 디바이스 스택을 구성하는 각각의 디바이스 오브젝트 소유자에게 순서대로 전달되고, 전달된 IRP는 또다시 소유자가 직접 호출하는 IoCallDriver() 함수에 의해 IO 매니저를 통해 다음 계층의 디바이스 오브젝트 소유자에게 전달된다.

## WDM

WDM은 드라이버 오브젝트, 디바이스 오브젝트, 과일 오브젝트, IRP 등과 같은 개념의 자료구조를 사용하면서 디바이스 스택을 구성하고 있는 장치를 마치 파일 접근하듯이 외부로 제공하는 것을 그 목적으로 한다.
WDF는 이곳에서 설명하는 WDM의 확장 개념이며, WDM이 처리하던 어렵고 까다로운 처리를 프레임워크에서 대신 처리하도록 고안된 개발 방식이다.

드라이버 개발자 관점에서 WDM을 이해하려면, IRP 자료구조와 이를 처리하는 처리기를 이해하는 거싱 가장 우선이며 필수적이다.

### IRP

WDK에서 정의하고 있는 IRP 구조체의 내용 중 일부분을 살펴보자

``` C
typedef struct _IRP{
  PMDL MdlAddress; // 1
  ULONG Flags;
  union {
    struct _IRP *MasterIrp;
    PVOID SystemBuffer; // 2
  } AssociatedIrp;
  IO_STATUS_BLOCK IoStatus; // 2
  KRPROCESSOR_MODE RequestorMode;
  BOOLEAN PendingReturned;
  BOOLEAN Cancel;
  KIRQL CancelIrql;
  PDRIVER_CANCLE CancleRoutine;
  PVOID UserBuffer; // 1
  union {
    struct {
      PETHREAD Thread;
      struct {
        LIST_ENTRY ListEntry;
        union {
          struct _IO_STACK_LOCATION *CurrentStackLocation; // 3
        };
      };
    } Overlay;
  } Tail;
} IRP, *PRIP;
```

