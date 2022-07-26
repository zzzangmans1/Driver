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

위 코드는 IRP 구조체의 내용 중 일부분만 발췌한 내용이다.
여기에서 보여주고 있는 모든 필드들을 전부 이해할 필요는 없지만, 이들 중 50% 정도를 알고 있는 개발자라면 중급 개발자이고, 70% 이상을 알고 있는 개발자라면 고급 개발자라고 생각한다.
이들 구조체의 개념을 유지하고 있는 IO 매니저 입자에서는 사실 가능하면 드라이버 개발자들이 이 필드들을 모두 직접 볼 필요가 없도록 최대한 가리고 있다.
그렇지만 현실에서는 필드를 직접 봐야 한다.
과감하게 이야기하자면 이들 필드를 직접 보지 않으려면 WDM 방식으로 드라이버를 개발하지 말아야 한다.

IRP 구조체 내부의 각 필드 중에서 반드시 알아야 하는 것들을 명시하고 있다.
`1`은 모두 IRP 명령어 패킷과 관련 있는 클라이언트 측의 버퍼와 관련 있는 필드다.
간단히 설명하자면, 응용프로그램 측에서 제공하는 버퍼 주소 혹은 이런 주소와 관련 있는 정보가 보관되는 곳이다.
드라이버는 이곳에 접근해서 클라이언트 측의 버퍼와 연결한다.

`2`는 드라이버가 IRP 구조체를 처리하고 원래 호출자인 클라이언트 측으로 돌아갈 때 반드시 기록하는 처리 결과를 보관하는 용도로 사용된다.
`3` 필드는 IRP 구조체가 가리키는 스택구조의 메모리 중 현재 위치를 보관한다.
운영체제는 스택 형태의 자료구조를 만든 뒤 해당하는 스택의 현재 위치 주소를 이곳에 보관한다.
드라이버는 이곳을 참조해 구체적인 IRP 명령어 구조체의 파라미터를 확인하게 된다.
예를 들어, 현재 IRP가 IRP_MJ_CREATE인지 아니면 IRP_MJ_READ인지 등의 구체적인 MajorFunction 값과 이에 따른 파라미터를 취한다.


#### 2.2.1.1 IO_STACK_LOCATION

IRP 구조체는 스택 형태의 자료구조인 IRP 스택의 현재 위치 혹은 Top 위치를 가리키는 포인터 변수를 가지고 있다.
IRP -> Tail.Overlay.CurrentStackLocation 포인터 변수가 존재한다.
이곳이 스택을 가리키는 포인터 변수다.

중요한 부분이 하나 더 있다.
PUSH와 POP의 역할을 하는 함수다.
이들은 IoCallDriver 함수와 IoCompleteRequest 함수다.
이 함수는 WDK에서 제공하는 함수 이름이며, 실제 메모리에서는 IofCallDriver 함수와 IofCompleteRequest 함수로 연결된다.
이들은 IRP 스택의 포인터를 PUSH, POP 하는 결과를 가져온다.

``` C
NTSTATUS
  IoCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PRIP Irp
    );
```

``` C
VOID
  IoCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );
```

이등 함수는 WDM 드라이버들 간에 IRP 명령어를 전달하고 종료하는 데 사용하는 중요한 함수다.
거의 대부분의 WDM 드라이버들이 이 함수를 사용한다고 봐도 무방하다.

IoCallDriver 함수를 호출하면 IRP 구조체의 소유권이 다음 드라이버에 넘어간다.
더 이상 호출자의 것이 아니라 피호출자의 것으로 전환된다는 의미다.

xxxbus.sys 드라이버는 자신이 받은 IRP를 종료하기 위해 IoCompleteRequest 함수를 호출한다.
이 함수는 IRP의 스택 위치를 POP과 동일한 효과를 주면서 점점 위로 올리게 된다.
이 과정 중에 xxxlower.sys, xxxfunction.sys, xxxupper.sys 드라이버가 준비한 CompleteRoution(완료 콜백 루틴)이 호출될 수 있다.

이곳에서 유의할 점은 IoCallDriver 함수는 IRP를 다음 계층의 드라이버에 전달한다.
IoCompleteRequest 함수는 IRP를 호출자에게 되돌려준다.
호출자에게 IRP가 반납되는 도중에 선택으로 상위 드라이버의 CompleteRoutine이 호출된다.
xxxfunction.sys 드라이버는 자신이 호출된 뒤 CompleteRoutine을 설정하고 있다.
CompleteRoutine은 xxxfunction.sys 드라이버가 가진 콜백 함수다.

위와 같이 설정하면 xxxbus.sys에서 IoCompleteRequest 함수를 사용하면 xxxfunction.sys 드라이버에게 CompleteRoutine이 된다.
xxxfunction.sys 드라이버의 콜백 함수가 호출된 다음에는 더 이상 CompleteRoutine이 설정돼 있지 않으므로 IRP 스택은 메모리에서 해제된다.
그리고 클라이언트에 종료 사실이 통보된다.

``` C
typedef struct _IO_STACK_LOCATION {
  UCHAR MajorFunction; // 1
  UCHAR MinorFunction; // 2
  ...
  union {
    struct {
      ULONG Length;
      ULONG POINTER_ALIGNMENT key;
      LARGE_INTEGER ByteOffset;
    } Read; // 3
    struct {
      ULONG Length;
      ULONG POINTER_ALIGNMENT Key;
      LARGE_INTEGER ByteOffset;
    } Write; // 3
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID Type3InputBuffer;
    } DeviceIoControl; // 3
  ...
  } Parametersl
  PDEVICE_OBJECT DeviceObject;
  PIO_COMPLETION_ROUTINE CompletionRoutine; // 4
  ...
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
```

`1`은 IRP_MJ_XXXX로 정의돼 있는 명령어 상수다.
`2`는 MajorFunction에 따라서 정해지는 부수적인 명령어 상수다.
`1` 필드에 따라서 구체적으로 어떤 union 공용체의 구조체 필드 `3`을 볼 것인지를 결정한다.

MajorFunction으로 사용되는 명령어 상수는 드라이버 개발자가 알아야 하는 가장 중요한 필드다.
이 필드의 값이 어떤 내용을 담고 있는가에 따라 대응되는 작업을 수행해야 하는 것이 드라이버의 역할이라고 볼 수 있다.

클라이언트 혹은 상위 계층의 드라이버는 피호출 드라이버에게 IRP 구조체를 전달할 때, MajorFunction 명령어 상수와 파라미터값을 함께 넣어서 전달한다.
이것은 피호출 드라이버가 해당하는 MajorFunction 명령어 상수를 읽어서 해석해야 하는 의무를 부과한다.

이어지는 구체적인 MajorFunction 명령어 상수는 클라이언트 혹은 상위 계층의 드라이버가 펑션 드라이버에게 전달하는 과정을 예로 들어서 설명한다.

**IRP_MJ_CREATE(0x00)**
IRP_MJ_CREATE는 대표적인 MajorFunction 명령어 상수로 사용된다.
우리말로는 '열기' 작업에 해당한다.
클라이언트 혹은 상위 계층의 드라이버는 펑션 드라이버에게 이 명령을 전달하게 되고, 펑션 드라이버는 이 명령어에 대해 성공 혹은 실패 결과를 담고 완료시킴으로써 '열기' 작업에 대한 허용 여부를 결정할 수 있게 된다.
이 명령어에 대해 성공 결과를 담고 완료시키지 않으면 더 이상 클라이언트 혹은 상위 계층의 드라이버는 이어지는 다른 MajorFunction 명령어를 펑션 드라이버에 보내지 못한다.

IRP_MJ_CREATE IRP 명령어가 xxxfunction.sys 드라이버에 전달된다.
이 명령어를 완료시킬 때 반드시 IRP 구조체의 IoStatus.Status 필드의 값을 적당한 완료 상태값으로 기록한다.

**IRP_MJ_CLOSE(0x02)**
IRP_MJ_CLOSE 명령어가 전달되는 상황은 조금 복잡하다.
윈도우 커널은 오브젝트 자료구조 형태의 메모리를 사용해서 시스템이 사용하려는 많은 유형의 데이터를 보관한다.
이것들은 다음과 같은 유형들이 존재한다.

|ObjectType parameter|Object pointer type
|--|--
|*ExEventObjectType|PKEVENT
|*ExSemaphoreObjectType|PKSEMAPHORE
|*IoFileObjectType|PFILE_OBJECT
|*PsProcessType|PEPROCESS or PKPROCESS
|*PsThreadType|PETHREAD or PKTHREAD
|*SeTokenObjectType|PACCESS_TOKEN
|*TmEnlistmentObjectType|PKENLISTMENT
|*TmResourceManagerObjectType|PKRESOURCEMANAGER
|*TmTransactionManagerObjectType|PKTM
|*TmTransactionObjectType|PKTRANSACTION

이들 중 PFILE_OBJECT의 참조횟수 입장에서 볼 때, 클라이언트 측에서 IRP_MJ_CLOSE IRP 명령어를 전송하고자 하더라도 조건에 맞지 않으면 이 명령어는 바로 전송되지 못한다.
더 이상 PFILE_OBJECT의 참조가 없는 것이 보장돼야 IRP_MJ_CLOSE IRP 명령어가 전달된다.

응용프로그램 클라이언트 측에서 API CloseHandle 함수를 호출할 때 xxxfunction.sys 드라이버에 IRP_MJ_CLOSE IRP가 전달되고 있다.
물론, 해당하는 IRP와 관련 있는 PFILE_OBJECT의 참조가 더 이상 없다고 보장될 때 해당하는 내용이다.

IRP_MJ_CLOSE IRP 명령어도 IRP_MJ_CREATE IRP 명령어와 마찬가지로 파라미터 전달 방식을 사용한다.
입출력 파라미터는 특별히 없으며, 완료 요청 시 적당한 완료 상태값을 사용해야 한다.

**IRP_MJ_READ(0x03)**
IRP_MJ_READ 명령어는 그 의미에서 유추할 수 있듯이 무엇인가 데이터를 읽는 목적으로 사용된다.
그렇기 때문에 드라이버 개발자는 처리할 내용이 있을 것이라고 추측할 수 있다.

IRP_MJ_CREATE 명령어가 성공적인 완료를 보고하고 나면, 클라이언트는 IRP_MJ_READ 명령어를 담은 IRP를 전송할 수 있다.

IRP.AssociatedIrp.SystemBuffer와 IRP.MdlAddress 필드는 둘 다 클라이언트가 제공하는 버퍼와 관련 있다.
전자는 IO 매니저가 새롭게 생성해주는 임시 버퍼 주소가 보관돼 있지만, 클라이언트가 제공하는 버퍼와 전혀 무관하게 동작하지 않는다.

**IRP_MJ_WRITE(0x04)**
IRP_MJ_WRITE 명령어는 IRP_MJ_READ 명령어와 동일한 파라미터를 사용한다.
단지, 응용프로그램이 제공하는 버퍼의 내용이 드라이버에 전달되는 점에서 방향만 반대다.

**IRP_MJ_DEVICE_CONTROL(0x0E)**
IRP_MJ_DEVICE_CONTROL 명령어가 IRP_MJ_READ, IRP_MJ_WRITE 명령어와 다른 특징으로 두 가지를 꼽을 수 있다.
하나는 버퍼가 최대 2개까지 동시에 사용될 수 있다는 점이고 다른 하나는 별도의 IO 컨트롤 코드가 사용된다는 점이다.

IRP_MJ_READ, IRP_MJ_WRITE 명령어에서는 하나의 버퍼가 사용되면서 그 사용되는 방향만 서로 반대였다.
IRP_MJ_DEVICE_CONTROL 명령어는 필요에 따라서 2개의 입출력 버퍼를 각각 사용할 수 있다.

|애플리케이션 클라이언트</br>(사용자 레벨)
|--
|CreateFile|ReadFile|WriteFile|DeviceIoControlFile|WriteFile
|IRP_MJ_CREATE|IRP_MJ_READ|IRP_MJ_WRITE|IRP_MJ_DEVICE_CONTROL|IRP_MJ_CLOSE
|ZwCreateFile|ZwReadFile|ZxWriteFile|ZwDeviceIoControlFile|ZwClose
|드라이버 클라이언트</br>(커널 레벨)
