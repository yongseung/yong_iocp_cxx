*기본적으로 IOWORKER thread 8 Logic thread 2개로 되어 있다

*채널 큐는 총 4개이다

* 소켓풀을 이용하여 재활용 한다 

* 각 소켓은 처리되는 logic thread 하나이다 (채널 변경시 경우에 따라 logic thread 가 달라진다)

* rmi type 에 따라서 응답을 한다

* db, 예외처리 등 되어 있지 않다

* 직렬화를 통해 user class 를 확장할 수 있다 (string, list 까지만 현재 지원 가능)