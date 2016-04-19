========================================================================
    CONSOLE APPLICATION : Yong Project Overview
========================================================================

AppWizard has created this Yong application for you.

This file contains a summary of what you will find in each of the files that
make up your Yong application.


Yong.vcxproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

Yong.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

Yong.cpp
    This is the main application source file.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named Yong.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////


there is a lot of problems 
anyway


02-10 RMI 떄문에 send를 전역으로 바꿨다

02-11 문제점

헤더에서 a,b 가 서로 헤더를 포함하면 에러 난다 -> 해결 b에서는 a의 필요한 객체만 등록해놓고 b의 cpp에서 a의 헤더를 포함하면 된다


02-12 문제
broadcast 만들었다. 
posttcpsend 는 쓰레드 safe하다 만약 1q 에서 4q로 (귓말) 등을 할때에는 직접 쓰는것이 아니라 1q로 enqueue 이벤트를 보내야 한다



	//Message 를 어떻게 하면 잘 보낼까???????? 에 대한 고민을 해보자
	//그 이후 클라 제대로 만들기

	//그 이후 직렬화 및 서버에서 정보 보기 및 코드 한번 다듬기 헝가리안 표기 
	//rmi 2개 정도 추가 한 다음에 포폴 올리자 한 3~4일 정도 good good good good


02-15 cpstream, packetstore, protocol 만들기
obj is nmust class or struct 에서 function 을 찾아가서 만약 거기에서 무엇인가를 대입했는데
거기에 대응되는 함수가 없어서 에러가 났다! 그결과 컴파일러는 아하 잘못찾아왔구나 해서 이타입이 아닙니다 라고 에러를 냈던거임ㅋㅋㅋㅋ\
생성자에{} 중괄호 없으면 이상한 에러난다
C1001: internal error in compiler : 생성자 abc():{} 이런건 에러다 : 다음에 변수 없으니까!
스트링 써줄때 2byte 스트링 사이즈를 위한 값이 추가된다

02-16 해야될것 	//logout, changechannelnext changechannel chat 패킷 제너레이트 만들기! 그담 protocol (enum type 도)

02-17 클라이언트 만들어서 단위 테스트!  // 그 이후에 서버 상태 체크 하는거랑, map, vector 이런것들 추가 더 하기 *.*//
헬오브 헬 begin, end 엄청잘못되어 있었음


02-18 overflow, underflow 체크해서 죽이는거랑 나머지 패킷 조사하기! 
클라에서 죽는거 발견함 이유는 서버가 패킷을 2개 보낼때 한개만 인식해서 한개를 그대로 버렸음 이걸 처리
어제는 io pending 을 무시해서 바로 소켓이 클로즈 해서 에러 났었음 그 외 에러 하나 나느데 클로우즈 되면서 어디서 오버플로우남 리스트쪽 예상

02-19 shutdown fin 패킷 함수의 호출을 막는거고 데이터는 오갈수 있음 버퍼 막는게 closesocket이다
1. client shutdown -> recv() -> close 해도 되고
2. cli 종료패킷 -> server -> answer -> cli shutdown,close -> server가 0바이트 받아서 종료 처리
만들껀 클라에서 패킷 만들기~
3. 터진 이유 worker 에서 기다려야되는데 안기다리고 컨티뉴 않되서 소켓 종료 서버에서 send하는데 종료 되었다고 연락와서 터짐


02-20 future 패턴 만들어서 현재 접속자 수랑 유저 정보 얻을 수 있도록 하고 람다함수 써서 종료 되는 정도만 구현해서
올리자 ^-^///

02-22 바꿀꺼 singlton 패턴 적용, 예외처리~ &, && std::move 정도 우선 atomic gogo



02-24 rmi 수정하고 ~~ 예외처리 하자 25일날 ㅎ.ㅎ //그담 SP 만들기 ㅎ.ㅎ ^.^