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

#### 2.2.1.2 IRP 완료

드라이버는 자신이 받은 IRP는 자신의 것으로 간주하는데, 이것은 중요한 의미를 담고 있다.
일단 전달된 IRP를 받은 드라이버는 어떻게 해서라도 해당하는 IRP를 처리해야 한다는 것이다.

IRP를 처리하는 방법에는 크게 세 가지가 있다.
* IRP를 다른 드라이버에 전달하는 방법
* IRP를 성공 혹은 에러 상태로 완료하는 방법
* IRP를 취소하는 방법

IRP에 포함된 명령어 IRP_MJ_XXX를 해석하는 과정 중에 드라이버가 일정한 시간을 소요해야 하는 상황도 있다.

일단 전달될 IRP는 드라이버의 것이다.
다만 일정한 시간을 기다려야 결과가 도출되는 상황이라면, 드라이버는 현재 스레드를 블로킹시키지 않고 호출자에게 STATUS_PENDING 리턴값을 가지고 돌아가야 한다.
이렇게 하지 않으면 효과적인 윈도우 플그램 환경을 사용자가 경험하지 못하게 된다.

유념할 부분은 STATUS_PENDING 값을 사용해서 리턴한다고 해도 여전히 IRP는 드라이버의 소유라는 점이다.
이것은 결국 드라이버에 의해 언젠가는 해당하는 IRP를 완료해야 한다는 의미다.

**성공 혹은 에러의 값을 사용해 IRP 완료하기**

``` C
NTSTATUS xxxfunction`s IRP 처리 함수(..)
{
  ...
  Irp->IoStatus.Status = STATUS_SUCCESS; // 혹은 에러값
  Irp->IoStatus.Information = ReadResultSize;
  // 실제로 수행한 크기값(바이트 단위)
  IoCompleteRequest(Irp, IO_NO_INCREMENT); // IO_NO_INCREAMENT(0)
  return STATUS_SUCCESS; // 함수가 리턴값을 요구하므로..
}
```

**취소의 값을 사용해 IRP 완료하기**

드라이버에서 IRP를 취소하는 이유는 해당하는 IRP를 더 이상 처리하고 싶지 않기 때문이다.
이와 같은 상황에서 IRP를 가지고 있을 필요가 없는 드라이버는 취소의 값을 사용해서 IRP를 완료한다.

``` C
// 드라이버가 작성하는 CancleRoutine 등록 함수(WDK)
PDRIVER_CANCEL
  IoSetCancelRoutine(
    IN PIRP Irp,
    IN PDRIVER_CANCEL CancelRoutine
  ); 
``` C
// 드라이버가 작성하는 CancelRoutine의 프로토타입
VOID
(*PDRIVER_CANCEL)(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp
);
```

``` C
CancelRoutine 혹은 특정루틴()
{
  ...
  Irp->IoStatus.Status = STATUS_CANCEL; // 취소를 의미한다.
  IoCompleteRequest(Irp, IO_NO_INCREMENT); // IO_NO_INCREAMENT(0)
}
```

이와 같이 상태값으로 STATUS_CANCEL 값을 사용해서 IRP를 완료하는 것을 취소 완료라고 한다.

**STATUS_PENDING 값을 사용해서 호출자에게 돌아가기**

``` C
NTSTATUS xxxfunction`s IRP 처리함수(...)
{
  ...
  IoMarkIrpPending(Irp);  // 1
  return STATUS_PENDING;
}
```

`1`은 IoMarkIrpPending() 함수를 사용해서 현재 IRP를 비동기적으로 처리할 것이라는 사실을 알린다.
비동기적으로 처리한다는 것은 지금 IRP를 처리할 것이 아니라, 다른 문맥에서 처리할 것이라는 의미다.

**받은 IRP를 다른 드라이버에게 넘겨주기**
받은 IRP를 다른 드라이버에게 넘겨주는 작업은 xxxfunction.sys 드라이버 입장에서는 귀찮은 일이다.
보통 응용프로그램 클라이언트로부터 요청되는 CreateFile(), ReadFile(), WriteFile(), DeviceIoControl(), CloseHandle()과 같은 함수에 의해 발생되는 IRP는 xxxfunction 드라이버가 직접 처리해야 한다.

뒤에서 살펴볼 IRP_MJ_PNP, IRP_MJ_POWER와 같은 명령어를 담고 있는 IRP가 대표적이다.
드라이버는 디바이스 스택을 구성하는 하나의 구성 요소일 뿐이다.
따라서 디바이스 스택을 구성하고 있는 다른 드라이버들과 함께 공유해야 하는 IRP_MJ_PNP, IRP_MJ_POWER와 같은 명령어는 반드시 디바이스 스택을 구성하고 있는 모든 드라이버들이 골고루 처리해야 한다.
이런 경우 드라이버는 자신이 받은 IRP에 대한 소유권을 다른 드라이버에 넘겨주는 행위의 표현으로 IoCallDriver() 함수를 사용한다.

```C
// WDK가 제공하는 IRP 스택 위치 관련 함수 중 일부분
PIO_STACK_LOCATION
  IoGetCurrentIrpStackLocation(
    IN PIRP Irp
  ); // 1
PIO_STACK_LOCATION
  IoGetNextIrpStackLocation(
    IN PIRP Irp
  ); // 1
PIO_STACK_LOCATION
  IoSkipCurrentIrpStackLocation(
    IN PIRP Irp
  ); // 1
PIO_STACK_LOCATION
  IoCopyCurrentIrpStackLocationToNext(
    IN PIRP Irp
  ); // 1
```

`1`, `2`의 함수는 IRP 구조체의 그 어떤 필드의 내용도 변경하지 않는다. 단지 현재 스택 위치와 다음 스택 위치 주소만을 가져온다.
`3`의 함수는 IRP의 현재 스택 위치를 이전 스택 위치로 옮기는 작업을 수행한다.
`4`의 함수는 IRP의 현재 스택 위치에 보관된 내용들 중 일부분을 다음 스택 위치로 복사하는 작업을 수행한다.

``` C
// WDK가 제공하는 IoCopyCurrentIrpStackLocationToNext 함수 내용
VOID
IoCopyCurrentIrpStackLocationToNext(
  PRIP Irp
)
{
  PIO_STACK_LOCATION irpSp;
  PIO_STACK_LOCATION nextIrpSp;
  irpSp = IoGetCurrentIrpStackLocation(Irp);
  nextIrpSp = IoGetNextIrpStackLocation(Irp);
  RtlCopyMemory(nextIrpSp, irpSp,
              FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); // 1
  nextIrpSp->Control = 0; // 2
}
```

위 코드는 IoCopyCurrentIrpStackLocationToNext() 함수가 IRP의 현재 스택위치의 내용을 다음 스택 위치로 복사하는 과정을 보여주고 있다.
이때 `1`에서 보면, IO_STACK_LOCATION 구조체의 CompletionRoutine 필드 이전까지만 복사하는 것을 알 수 있다.

중급개발자가 알아야 할 사항이지만, `2`와 같이 Control 필드의 값이 0으로 지워지는 것도 기억해야 한다.
Control 필드는 IoSetCompletionRoutine() 함수와 IoMarkIrpPending() 함수에 의해 변경되는 필드다.

### 2.2.2 필수 루틴
WDM 드라이버를 작성하는 데 있어서 IRP_MJ_CREATE, READ, WRITE, DEVICE_CONTROL, CLOSE 명령어를 처리해야 할 의무는 없다.
이것들을 처리할 루틴은 응용프로그램과의 통신을 위해 필요하므로 드라이버가 가져야 할 필수 루틴은 아니다.
이와 달리, 다음 절에서 소개하는 5개의 루틴들은 반드시 가져야 한다.
WDM 드라이버인 경우에 이들 중에서 어느 하나를 빼면 정상적인 동작을 보장받기가 어렵다.

#### 2.2.2.1 DrvierEntry

디바이스 드라이버가 메모리에 상주하는 의미로 호출될 콜백 함수다.
따라서 드라이버가 메모리에 상주하면서 필요한 전역변수를 준비해야 한다.
같은 종류의 드라이버는 절대로 두 번 이상 메모리에 함께 상주하지 않는다.

```C
NTSTATUS
DriverEntry(
  IN PDRIVER_OBJECT DriverObject, // 1
  IN PUNICODE_STRING RegistryPath // 2
);
```

`1`에서는 WDM 드라이버를 추상화하는 자료구조 DriverObject의 주소를 담고 있다.
하나의 드라이버는 하나의 DriverObject를 사용한다.
IO 매니저에 의해 준비된다.
`2`에서 WDM 드라이버는 Win32 서비스와 같은 형태로 관리된다.
서비스 프로그램들은 시스템 레지스트리에 지정된 위치에 자신을 기록하게 돼 있으며, RegistryPath는 이곳의 키 이름을 담고 있다.
WDM 드라이버가 선택적으로 이곳을 사용해서 필요한 파라미터들을 보관하거나 읽는 용도로 작성될 수 있다.

``` C
// WDM 드라이버 DriverEntry() 에제 코드
NTSTATUS
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath
)
{
  NTSTATUS returnStatus = STATUS_SUCCESS;
  DriverObject->DriverUnload = SAMPLE_Unload; // 1
  DriverObject->DriverExtension->AddDevice = SAMPLE_AddDevice; // 2
  DriverObject->MajorFunction[IRP_MJ_PNP] = SAMPLE_PnpDispatch; // 3
  DriverObject->MajorFunction[IRP_MJ_POWER] = SAMPLE_PowerDispatch; // 4
  return returnStatus;
}
```

위 코드를 보면 일반적인 WDM 디바이스 드라이버의 DriverEntry() 콜백 함수가 가지는 코드를 알 수 있다.
`1`,`2`,`3`,`4` 필드는 모두 이어지는 설명에서 소개할 콜백 함수들의 주소를 기록하는 곳이다.

이와 같이 드라이버는 DriverEntry() 에서 DriverObject 구조체가 가지는 콜백 함수 관련 필드들을 적당한 콜백 함수 주소로 기록하는 작업을 수행한다.

#### 2.2.2.2 AddDevice

메모리에 상주하고 있는 드라이버는 이후 메모리에서 제거될 때까지 그대로 메모리에 머문다.

IO 매니저는 디바이스 스택을 구성하는 과정 중에 사용하는 디바이스 오브젝트를 생성하기 위해, 시스템 레지스트리에 설치된 정보를 사용해 적절한 드라이버를 호출한다.
디바이스 스택을 구성하는 순서에 따라서, 가장 아래에 PDO를 두고 그 위에 올라갈 DeviceObject를 생성하기 위해 적당한 드라이버의 AddDevice() 콜백 함수를 호출한다.
AddDevice() 콜백 함수는 디바이스 스택을 구성하는 책임을 가진 콜백 함수다.
AddDevice() 콜백 함수는 디바이스 스택에 포함될 디바이스 오브젝트를 생성한 뒤, 이를 직접 디바이스 스택에 올려놓는 작업을 수행한 뒤 돌아온다.

``` C
// WDM 드라이버 AddDevice 프로토타입
typedef
NTSTATUS
DRIVER_ADD_DEVICE(
  IN struct _DRIVER_OBJECT *DriverObject, // 1
  IN struct _DEVICE_OBJECT *PhysicalDeviceObject // 2
);
```

위 코드는 드라이버가 가져야 하는 AddDevice 함수의 프로토타입이며 2개의 파라미터를 넘겨받는다.
`1`의 파라미터는 드라이버의 DriverObject 주소이고, `2`의 파라미터는 디바이스 스택을 구성하는 가장 아래 계층의 버스 드라이버가 생성한 DeviceObject다.

``` C
NTSTATUS SAMPLE_AddDevice(
  IN PDRVIER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject )
{
  // 생략
  returnStatus = IoCreateDevice(
                  DriverObject,
                  sizeof(DEVICE_EXTENSION),
                  NULL,
                  FILE_DEVICE_UNKNOWN,
                  FILE_AUTOGENERATED_DEVICE_NAME,
                  FALSE,
                  &DeviceObject
                  ); // 1
  deviceExtension = DeviceObject->DeviceExtension;
  RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));
  deviceExtension->PhysicalDevcieObject = PhysicalDeviceObject;
  deviceExtension->DeviceObject = DeviceObject;
  deviceExtension->NextLayerDeviceObject =
  IoAttachDeviceToDeviceStack(
                  DeviceObject,
                  PhysicalDeviceObject
                  ); // 2
  // 생략
  IoRegisterDeviceInterface(PhysicalDeviceObject, &GUID_SAMPLE, NULL,
          &devcieExtension->UnicodeString); // 3
  return returnStatus;
}
```

위 코드들은 일반적인 AddDevice 콜백 함수에서 수행하는 작업을 보여준다.
이 작업은 크게 세 가지로 나뉜다.

코드의 `1` 부분에서 드라이버의 소유물인 DeviceObject를 생성한다.
`2`는 생성한 DeviceObject를 현존하는 PDO에서 시작하는 디바이스 스택 위에 올려놓는 작업을 수행한다.
`3`에서는 응용프로그램 혹은 외부의 클라이언트로 드라이버가 올라가 있는 것을 알린다.

``` C
// WDK가 제공하는 IoCreateDevice 함수 프로토타입
NTSTATUS
  IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize, // 1
    IN PUNICODE_STRING DeviceName OPTIONAL, // 2
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    );
```

위 코드는 DeviceObject를 생성하는 목적으로 사용하는 함수다.

`1`에서는 파라미터에서 지정한 크기만큼의 메모리가 할당돼, 생성될 DeviceObject와 연결된다.
개발자는 이와 같은 방법을 사용해서, DeviceObject마다 가지는 새로운 메모리를 정의할 수 있다.
이와 같이 메모리에 대한 접근은 생성될 DeviceObject->DeviceExtension 포인터 변수에 의해 이뤄진다.

`2`의 파라미터는 생성될 DeviceObject의 이름을 나타내는 필드다.
항상 이름을 가져야 하는 것은 아니지만, 특별한 목적으로 이름을 지정할 수 있으며, 이렇게 지정된 이름은 반드시 시스템에서 고유한 이름이어야 한다.

``` C
// WDK가 제공하는 IoAttachDeviceToDeviceStack 함수 프로토타입
PDEVICE_OBJECT
  IoAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
  );
```

위 코드는 SourceDevice의 DeviceObject를 TargetDevice의 DeviceObject 위로 올리는 역할을 수행한다.
이때 유의할 점은 TargetDevice 위에 다른 DeviceObject가 이미 올라가 있는 경우 SourceDevice는 TargetDevice 위에 올라가지 않고 가장 꼭대기에 올라간다는 것이다.

``` C
// WDK가 제공하는 IoRegisterDeviceInterface 함수 프로토타입
NTSTATUS
  IoRegistrerDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,  // 1
    IN CONST GUID *InterfaceClassGuid, // 2
    IN PUNICODE_STRING ReferenceString OPTIONAL,
    OUT PUNICODE_STRING SymbolickLinkName // 3
    );
```

위 코드에 나타난 함수는 응용프로그램에서 API CreateFile() 함수에 의해 접근하는 목적으로 사용될 '심볼릭 이름'을 생성하는 역할을 한다.
심볼릭 이름을 생성할 때 사용되는 `2`의 파라미터 InterfaceClassGuid는 개발자가 지정하는 다양한 GUID 중 하나가 사용될 수 있는데, 보통 이것을 인터페이스 GUID라고 부른다.

`1`의 파라미터는 심볼릭 이름을 연결시킬 디바이스 스택의 가장 아래에 존재하는 PDO를 의미한다.
`3`에서 파라미터는 생성된 심볼릭 이름이 리턴된다.
이렇게 리턴된 이름은 외부 클라이언트에서 접근을 시도할 때 사용되며, 이름에 대한 접근을 허용하거나 금지하는 목적으로 드라이버는 WDK가 제공하는 IoRegisterDeviceInterfaceState 함수를 사용하는데 이때 `3`의 파라미터가 사용된다.

#### 2.2.2.3 PlugNPlayDispatch

플러그 앤 플레이 IRP를 처리하는 드라이버의 콜백 함수를 PlugNPlayDispatch 루틴이라고 부른다.
IO 매니저는 내부적으로 PNP 매니저, Power 매니저의 역할을 함께 수행한다.
PNP 매니저는 디바이스 스택이 형성되는 과정부터 제거되는 과정까지 발생하는 모든 플러그 앤 플레이 이벤트를 IRP에 담아서 드라이버를 호출한다.

PNP 매니저가 다루는 PNP 이벤트(주요 사건만 발췌)
* IRP_MN_START_DEVICE : 디바이스 스택이 구동된다는 사건
* IRP_MN_REMOVE_DEVICE : 디바이스 스택이 해제된다는 사건
* IRP_MN_SURPRISE_REMOVAL : 갑작스런 디바이스 스택 해제 사건이 발생했다는 의미
* IRP_MN_QUERY_CAPABILITIES : 디바이스 스택의 특정 정보를 얻겠다는 의미

PNP 명령어는 IRP_MJ_PNP 값을 사용하는 IRP 형식으로 존재한다.
이때 구체적인 PNP 이벤트를 명시하기 위해 IRP의 IO_STACK_LOCATION 구조체가 가지는 또 다른 필드인 MinorFunction 필드를 사용한다.
이곳에는 IRP_MN_XXX 값이 담겨 있다.
MinorFunction 필드의 의미는 IRP_MJ_XXX 명령어에 따라서 다양해질 수 있다.
PNP 명령어에서는 PNP 이벤트를 담는 용도로 사용한다.

PNP 이벤트를 특정 드라이버가 받게 되면, 그 이벤트를 처리하는 일종의 약속이 있다.
이것은 다음 두 가지의 경우로 나뉠 수 있다.

* 받은 IRP를 다른 작업을 하기 전에 디바이스 스택에 존재하는 자신 다음 계층의 드라이버에 보낸 뒤, 그 결과를 확인하고 결과에 따라서 하려던 본 작업을 수행하는 경우(후처리)
* 받은 IRP를 먼저 확인해서 파라미터에 대한 해석 및 처리 작업을 한 뒤, 더 이상 할 일이 없을 때 디바이스 스택에 존재하는 자신 다음 계층의 드라이버에 보내는 경우(전처리)

물론 이와 같은 순서는 PhysicalDeviceObject의 소유자인 버스 드라이버에는 적용되지 않는다.
버스 드라이버가 소유하는 PDO는 디바이스 스택의 가장 아래에 존재하기 때문에, 전처리 또는 후처리와 같은 PNP 이벤트를 처리하는 순서에 상관없이 받은 IRP를 처리하고 반드시 IRP를 완료시켜야 한다.

``` C
// WDM 드라이버 PnPDispatch 프로토타입
NTSTATUS
SAMPLE_PnpDispatch(
  IN PDEVICE_OBJECT DeviceObject, // 드라이버의 소유 DeviceObject
  IN PIRP Irp                     // PNP 이벤트 IRP
);
```

예를 들어 IRP_MN_START_DEVICE 명령어는 디바이스 스택이 구동된다는 이벤트다.
이 명령어는 반드시 디바이스 스택의 가장 아래 계층에 존재하는 DeviceObject의 소유자, 즉 버스 드라이버가 가장 먼저 처리해야 한다.
버스 드라이버가 먼저 구동준비를 하지 않으면 그 위에 있는 다른 드라이버들의 구동 준비는 아무 의미가 없기 때문이다.

예를 들어, IRP_MJ_REMOVE_DEVICE 명령어는 디바이스 스택이 해제된다는 이벤트다.
이 명령어는 반드시 디바이스 스택의 가장 상위 계층에 존재하는 DeviceObject의 소유자 드라이버부터 아래 계층의 드라이버로 처리 순서가 이어져야 한다.

#### 2.2.2.4 PowerDispatch

IO 매니저가 가진 또 다른 기능을 나타내는 이름이 Power 매니저다.
PNP 매니저는 디바이스 스택을 PNP 이벤트를 알리는 역할을 수행하는 반면, Power 매니저는 디바이스 스택으로 Power 이벤트를 알리는 역할을 수행한다.

디바이스 드라이버 입장에서 Power 이벤트는 사건 그 자체보다는 사건이 발생하는 시기를 알고자 하는 목적으로 사용된다.

Power 매니저가 다루는 Power 이벤트
* IRP_MN_POWER_SEQUENCE : 관리 용도로 사용되며, 몇 번에 걸쳐서 특정 Power 이벤트가 발생했는지를 확인한다.
* IRP_MN_QUERY_POWER : 특정 Power 이벤트에 대한 허용 여부를 확인한다.
* IRP_MN_SET_POWER : 특정 Power 이벤트의 발생을 알린다.
* IRP_MN_WAIT_WAKE : 디바이스 스택의 전원 공급이 다시 재개돼야 하는 시기를 알기 위해 사용하는 IRP다.

디바이스 드라이버가 Power 이벤트에 관심을 가져야 하는 이유는 다음과 같다.
* 디바이스로의 전원 공급이 어떤 이유에서 끊겨야 하는 사건이 발생된다면 그 시기를 알아야 한다.

예를 들어, 사용자가 절전 모드로 사용자 PC의 상태를 변경하려는 경우 드라이버는 자신이 다루는 하드웨어에 대한 문맥 보관 및 전원 해제와 관련된 처리가 필요할 수 있다.

* 디바이스로의 전원 공급이 다시 재개돼야 하는 시기를 운영체제로 알린다.

전원 공급이 끊긴 상태에서, 다시 전원 공급을 재개해야 하는 상황이 발생될 수 있다.
예를 들어 절전 모드의 PC를 사용하는 상태에서, 사용자가 마우스를 움직인다든지 하는 행위를 통해 전원 공급을 운영체제에 알릴 수 있다.

* 디바이스로의 전원 공급이 다시 재개되는 시기를 알아야 한다.

전원이 끊긴 상태에서, 디바이스에 전원이 다시 공급되면 드라이버는 최근에 사용하던 디바이스의 상태로 복원해야 한다.
예를 들어, 외부에서 파일을 다운로드 하던 중이었다면 이 작업을 재개해야 한다.
마찬가지로, 영화를 보고 있던 상태에서 전원 공급이 끊겨 있었다면 다시 영화가 재개돼야 한다.

결국 이와 같은 내용들은 모두 문맥 보관과 복원이라는 두 가지 중요한 작업을 하기 위한 시기를 알아야 한다는 점에서 드라이버가 Power 이벤트를 다루는 목적이라고 볼 수 있다.

``` C
// WDM 드라이버 PowerDispatch 프로토타입
NTSTATUS
SAMPLE_PowerDispatch(
  IN PDEVICE_OBJECT DeviceObject, // 드라이버 소유 DeviceObject
  IN PIRP Irp                     // Power 이벤트 IRP
);
```

일반적인 IRP(IRP_MJ_PNP 포함)를 다른 드라이버에 전달하는 데는 IoCallDriver 함수를 이용한다.
반면에 Power IRP를 다른 드라이버에 전달하는 데는 PoCallDriver 함수를 이용한다.

IoCallDriver 함수는 동기화 작업을 하지 않는 반면, PoCallDriver 함수는 동일한 드라이버가 2개 이상의 Power IRP를 처리하는 것을 허용하지 않도록 해준다.
이것은 윈도우에서 요구하는 사항이므로 반드시 따라야 한다.

PoCallDriver 함수가 동기화 작업을 수행한다는 점에서 별도의 함수 호출 한 가지를 드라이버가 알아야 하는데, 그것은 바로 PoStartNextPowerIrp 함수다.
이 함수는 드라이버가 또 다른 종류의 Power IRP를 처리할 수 있다는 것을 Power 매니저에게 알리는 용도로 사용된다. 
여기서 PoStartNextPowerIrp 함수를 호출할 당시에 IRP의 현재 스택은 드라이버의 것이어야 한다는 점에 주의해야 한다.

WDK에서 정의하는 Power 상태의 종류(시스템 Power 상태)
* S0 : System Working State
* S1, S2, S3 : Sleep State
* S4 : Hibernation State(Typical Sleep)
* S5 : System Power Off State

운영체제는 위와 같이 S0에서부터 S5까지 모두 여섯 가지 Power 상태를 정의한다.
이들은 S0에서부터 차례로 점점 전원 공급이 약해지는 상태로 정의되고 있다.
평상시 PC를 운용하고 있는 상태가 S0 상태이며 PC 전원을 오프한 상태가 S5 상태다.

S4는 특별한 Sleep 상태를 지원하는데 외관상으로는 S5와 동일한 것처럼 보이지만, 전원을 다시 공급했을 때 롬 바이오스의 도움을 받아서 부팅 과정 중에 S5의 부팅 과정과 다르게 웜 부팅 과정을 진행한다.
다시 말하자면, 하드디스크에 보관된 최근 사용 중이었던 메모리 이미지를 그대로 복원시켜서 S0 상태에서 수행하던 작업을 재개하게 된다.

절전 모드에 들어가 있는 PC 상태에서도 동작을 계속해야 하는 하드웨어들이 여기에 해당한다.
이와 같이 디바이스 Power 상태는 시스템 Power 상태와 시스템 Power 상태와 별도로 관리돼야 한다.
디바이스 Power 상태를 관리하는 관리자는 Power 정책 매니저라고도 부르는데, 보통 xxxfunction.sys 드라이버가 이 작업을 수행한다.

WDK에서 정의하는 Power 상태의 종류(디바이스 Power 상태)
* D0 : Device Working State
* D1, D2 : Sleep State
* D3 : Device Power Off State

위 보기는 디바이스가 가질 수 있는 Power 상태를 정의하고 있다.
시스템 Power 상태와 유사한 의미로 사용된다.
조금 더 자세하게 이들의 차이를 설명하려면 디바이스 Power 상태의 다섯 가지 정의자를 알아야 한다.

디바이스 Power 상태의 상태를 구분하는 다섯 가지 정의자
* Power 소비 상태 : 디바이스가 얼마나 많은 전력을 소비하는가?
* 디바이스 문맥 : 현재 Power 상태에서 얼마나 많은 문맥 정보가 유지되는가?
* 드라이버 행동 : Sleep 상태에서 D0 상태로 전환하는 데 어떤 작업이 필요한가?
* 복원시간 : Sleep 상태에서 D0 상태로 전환하는 데 어느 정도의 시간이 필요한가?
* 별도의 깨어나는 특성 보유 여부 : 예를 들어 D1, D2 상태에 있는 디바이스가 D0 상태로 전환하도록 요청할 수 있는가?

간단히 정리하면, D3 상태에서 D0 상태로 전환하는 것이 D2 상태에서 D0 상태로 전환하는 것보다 시간이 더 필요할 수 있다는 의미를 포함한다.

예를 들어, PC의 상태가 S0 상태를 유지하는 동안에 장시간 동안 사용되지 않는 디바이스의 전원 공급을 끊어버리는 것이 가능하다.
이것은 해당하는 디바이스의 xxxfunction.sys 드라이버에 의해 자발적으로 진행되는데, 보통 응용프로그램이 IRP_MJ_CREATE IRP 명령어를 xxxfucntion.sys 드라이버에 보내지 않은 상태가 오랫동안 지속되면 xxxfunction.sys 드라이버는 자신의 디바이스 스택의 Power 상태를 D1, D2 또는 D3 상태로 전환하는 것은 선택적 중지라고도 부른다.
PCMCIA 카드 형태의 장치들이나 USB 버스를 사용하는 장치들이 여기에 해당하는 경우가 많다.

#### 2.2.2.5 DriverUnload 

디바이스 드라이버가 메모리에 제거될 때 호출되는 콜백 함수다.
드라이버가 사용하던 시스템 리소스를 운영체제에 반납하는 목적의 코드를 작성한다.
대부분의 경우 이 콜백 함수를 사용하는 빈도가 적다.

### 2.2.3 WDM 드라이버 보충 기술

WDM 디바이스 드라이버를 작성하기 위해 필수적으로 알아야 하는 부분을 설명했다.
하지만 실전에서 드라이버를 작성하려면 여전히 몇 가지 중요한 실전적인 기술이 필요하다.
이번에는 이 중에서 세 가지 개념을 설명하려고 한다.
이것들은 드라이버 코딩을 하는 과정에서 자주 언급되는 내용이므로 꼭 숙지하길 바란다.

#### 2.2.3.1 IoSkipCurrentIrpStackLocation

IoSkipCurrentIrpStackLocation 함수는 WDK가 제공하며 실전에서 자주 사용된다.
이름 자체를 해석하면, IRP의 현재 스택을 버린다는 뜻이다.
이 말은 이 함수를 호출할 당시의 드라이버가 소유하던 IRP의 소유권을 다음 드라이버에 넘긴다는 뜻으로 사용된다.

``` C
// IoSkipCurrentIrpStackLocation() 함수 사용 예제 코드
NTSTATUS SAMPLE_PnpDispatch(
  IN PDEVICE_OBJECT DeviceObject,
  IN PRIP Irp)
{
  // 생략
  pStack = IoGetCurrentIrpStackLocation(Irp);
  deviceExtension = DeviceObject->DeviceExtension;
  NextLayerDeviceObject = deviceExtension->NextLayerDevcieObject;
  
  switch(pStack->MinorFunction){
    case IRP_MN_REMOVE_DEVICE: // 1
      IoDetachDevice(NextLayerDeviceObject);
      IoDeleteDevice(DeviceObject);
      break;
  }
  IoSkipCurrentIrpStackLocation(Irp); // 2
  return IoCallDriver(NextLayerDeviceObject, Irp); // 3
}
```

위 코드는 IRP_MJ_PNP 명령어를 처리하는 샘플 드라이버의 PnpDispatch 코드의 일부분을 보여준다.
`1`의 IRP_REMOVE_DEVICE는 앞 절에서 배웠던 대로 선 처리해야 하는 대표적인 명령어다.
위 코드처럼 이 명령어의 경우 드라이버는 수행할 작업을 우선 수행한 뒤, `2`의 IoSkipCurrentIrpStackLocation() 함수를 호출하고 있다.
이제 IRP의 소유권은 다음 계층의 드라이버에게 넘어간다.
이후 `3`의 IoCallDriver() 함수를 호출해 다음 계층의 드라이버에 IRP를 전달하고 있다.

이와 같이 드라이버가 IRP를 처리하는 방법 중에 선처리를 하는 경우 자주 사용될 수 있는 코드 흐름이므로 이 내용을 잘 기억해두길 바란다.

#### 2.2.3.2 완료 루틴

여기서는 완료 루틴을 설정하는 방법과 완료 루틴을 작성하는 방법에 대해서만 다루도록 하겠다.
설명을 위해 IRP를 전달하는 드라이버를 '전달자', IRP를 수신하는 드라이버를 '수신자'라고 정의하자.

완료 루틴은 전달자가 설정한다.
전달자는 IRP를 수신자에게 전달하면서, IRP가 완료 요청되는 시기를 알고자 완료 루틴을 설정한다.
완료 루틴은 IoCompleteRequest() 함수에 의해 호출된다.

``` C
// WDK IoCompleteRequest 함수 프로토타입
VOID
  IoSetIoCompleteRequest(
    IN PIRP Irp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine, // 1
    IN PVOID Context,
    IN BOOLEAN InvokeOnSuccess, // 2
    IN BOOLEAN InvokeOnError,
    IN BOOLEAN InvokeOnCancel
  );
```

전달자는 IoCompleteRequest 함수를 사요애서 `1`의 완료 루틴을 설정한다.
이때 코드의 `2` 부분에서 성공, 에러, 취소의 상태 중 어떤 상태로 완료될 때 완료 루틴이 호출되기를 원하는지를 지정하고 있다.
`2`의 파라미터에서 사용되는 성공, 에러, 취소는 함께 지정될 수 있다.
이와 같이 설정된 완료 루틴의 프로토타입은 밑의 코드와 같다.

``` C
// WDK에서 정의한 CompletionRoutine 프로토타입
typedef
NTSTATUS // 특별한 리턴값이 사용될 수 있다.
IO_COMPLETION_ROUTINE(
  PDEVICE_OBJECT DeviceObject, // 1
  PIRP Irp,
  PVOID Context 
);
```

위 코드에서 볼 수 있는 CompletionRoutine의 `1` 파라미터는 완료 루틴을 설정한 드라이버의 소유 DeviceObject가 담겨진다.
이 필드의 값이 NULL일 수도 있는데, 이것은 해당하는 IRP를 전달자가 직접 생성해 수신자에게 전달하는 경우에 해당한다.
보통 전달자가 자신이 다른 전달자로부터 전달한 IRP를 수신한 뒤, 이를 사용해서 또 다른 수신자에게 전달하는 경우에는 이 필드의 값으로는 정상적인 값이 사용된다.

위 코드를 보면 CompletionRoutine은 특별한 리턴값이 사용될 수 있음을 알 수 있다.
이 값은 'STATUS_MORE_PROCESSING_REQUIRED'다.
이 의미를 그대로 해석해보면, '해당하는 IRP를 완료하려면 조금 더 다른 처리가 필요하다!'는 뜻이 된다.

이 값이 아닌 다른 값을 리턴하는 경우에만, IoCompleteRequest(0 함수는 해당하는 IRP를 완료시키고 메모리에서 IRP를 해제한다.
그래서 드라이버가 자신의 CompletionRoutine에서 STATUS_MORE_PROCESSING_REQUIRED 값을 리턴하면 해당하는 IRP의 완료 작업은 중지되므로, 현재 IRP의 소유권이 드라이버 본인에게 돌아온다는 것을 기억해야 한다.

CompletionRoutine을 작성할 떄 마지막으로 유의할 점이 하나 더 있다.
Completion Routine에서 STATUS_MORE_PROCESSING_REQUIRED 값이 아닌 다른 값을 리턴하는 경우 반드시 호출해야 하는 함수가 있는데, 그것이 바로 IoMarkIrpPending() 함수다.
IoMarkIrpPending() 함수는 해당하는 IRP가 비동기적으로 완료될 수 있다는 것을 명시한다.

``` C
// IoMarkIrpPending() 함수와 CompletionRoutine
NTSTATUS
SAMPLE_CompletionRoutine(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP, Irp,
  IN PVOID Context)
{
  if(Irp->PendingReturned) // 1
    IoMarkIrpPending(Irp); // 2
  return Irp->IoStatus.Status;
  // 이 값은 STATUS_MORE_PROCESSING_REQUIRED 갑싱 될 수 없다.
}
```

위 코드를 보면, 일반적인 CompletionRoutine의 모습을 보여준다.
이와 같이 해당하는 IRP가 `1`의 코드를 통해 STATUS_PENDING 리턴된 적이 있는지를 조사한 후 `2`에서 IoMarkPending() 함수를 호출하지 않으면 해당하는 IRP가 비동기적으로 완료된다는 사실이 현상이 발생해서 해당하는 IRP가 완료되기를 기다리는 클라이언트에 완료 사실이 통보되지 못하는 현상이 발생한다.

#### 2.2.3.3 IRQL

윈도우 커널은 자신이 수행하려는 일반적인 작업들을 수행하는 과정 중에 처리하지 않으면 안 되는 중요한 작업이 CPU를 선점하는 것을 허용하고 있다.
이런 선점 행위는 인터럽트라는 의미로 사용된다.
인터럽트는 CPU를 선점하는 행위면서, 이렇게 CPU를 선점하는 작업은 커널이 수행하려는 일반적인 작업들보다 우선순위가 높다는 점을 기억해야 한다.
따라서 인터럽트는 가능하면 빠른 시간 안에 끝나고 원래 작업에 CPU를 돌려줘야 한다.

윈도우 커널이 사용하는 IRQL은 모두 32가지가 존재한다.
이것은 대수적으로 IRQL 0부터 IRQL 31까지로 정의하며, 큰 값이 우선순위가 높다.

이와 같은 인터럽트가 사용하는 IRQL 값은 동일한 수준의 IRQL 값을 사용하는 작업으로부터 CPU를 선점하지 못한다.
또한 IRQL은 CPU를 선점하는 작업이기 때문에 CPU의 개수만큼 IRQL이 존재한다는 점도 기억해야 한다.

|루틴|IRQL
|--|--
|DriverEntry|PASSIVE_LEVEL(0)
|AddDevice|PASSIVE_LEVEL(0)
|PnPDispatch|PASSIVE_LEVEL(0) or APC_LEVEL(1)
|PowerDispatch|PASSIVE_LEVEL(0) or DISPATCH_LEVEL(2)
|xxxIRPDispatch|PASSIVE_LEVEL(0) or APC_LEVEL(1) or DISPATCH_LEVEL(2)
|DriverUnload|PASSIVE_LEVEL(0)
|ISR Routine|DIRQL_LEVEL(n >= 2)
|Completion Routine|PASSIVE_LEVEL(0), APC_LEVEL(1) or DISPATCH_LEVEL(2)
|DPC Routine|DISPATCH_LEVEL(2)
|Time Routine|DISPATCH_LEVEL(2)
|WorkItem Routine|PASSIVE_LEVEL(0)
|StarIO Routine|DISPATCH_LEVEL(2)
|System Thread Routine|PASSIVE_LEVEL(0)


