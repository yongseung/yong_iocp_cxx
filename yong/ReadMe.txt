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


02-10 RMI ������ send�� �������� �ٲ��

02-11 ������

������� a,b �� ���� ����� �����ϸ� ���� ���� -> �ذ� b������ a�� �ʿ��� ��ü�� ����س��� b�� cpp���� a�� ����� �����ϸ� �ȴ�


02-12 ����
broadcast �������. 
posttcpsend �� ������ safe�ϴ� ���� 1q ���� 4q�� (�Ӹ�) ���� �Ҷ����� ���� ���°��� �ƴ϶� 1q�� enqueue �̺�Ʈ�� ������ �Ѵ�



	//Message �� ��� �ϸ� �� ������???????? �� ���� ����� �غ���
	//�� ���� Ŭ�� ����� �����

	//�� ���� ����ȭ �� �������� ���� ���� �� �ڵ� �ѹ� �ٵ�� �밡���� ǥ�� 
	//rmi 2�� ���� �߰� �� ������ ���� �ø��� �� 3~4�� ���� good good good good


02-15 cpstream, packetstore, protocol �����
obj is nmust class or struct ���� function �� ã�ư��� ���� �ű⿡�� �����ΰ��� �����ߴµ�
�ű⿡ �����Ǵ� �Լ��� ��� ������ ����! �װ�� �����Ϸ��� ���� �߸�ã�ƿԱ��� �ؼ� ��Ÿ���� �ƴմϴ� ��� ������ �´����Ӥ�������\
�����ڿ�{} �߰�ȣ ������ �̻��� ��������
C1001: internal error in compiler : ������ abc():{} �̷��� ������ : ������ ���� �����ϱ�!
��Ʈ�� ���ٶ� 2byte ��Ʈ�� ����� ���� ���� �߰��ȴ�

02-16 �ؾߵɰ� 	//logout, changechannelnext changechannel chat ��Ŷ ���ʷ���Ʈ �����! �״� protocol (enum type ��)

02-17 Ŭ���̾�Ʈ ���� ���� �׽�Ʈ!  // �� ���Ŀ� ���� ���� üũ �ϴ°Ŷ�, map, vector �̷��͵� �߰� �� �ϱ� *.*//
����� �� begin, end ��û�߸��Ǿ� �־���


02-18 overflow, underflow üũ�ؼ� ���̴°Ŷ� ������ ��Ŷ �����ϱ�! 
Ŭ�󿡼� �״°� �߰��� ������ ������ ��Ŷ�� 2�� ������ �Ѱ��� �ν��ؼ� �Ѱ��� �״�� ������ �̰� ó��
������ io pending �� �����ؼ� �ٷ� ������ Ŭ���� �ؼ� ���� ������ �� �� ���� �ϳ� ������ Ŭ�ο��� �Ǹ鼭 ��� �����÷ο쳲 ����Ʈ�� ����

02-19 shutdown fin ��Ŷ �Լ��� ȣ���� ���°Ű� �����ʹ� ������ ���� ���� ���°� closesocket�̴�
1. client shutdown -> recv() -> close �ص� �ǰ�
2. cli ������Ŷ -> server -> answer -> cli shutdown,close -> server�� 0����Ʈ �޾Ƽ� ���� ó��
���鲫 Ŭ�󿡼� ��Ŷ �����~
3. ���� ���� worker ���� ��ٷ��ߵǴµ� �ȱ�ٸ��� ��Ƽ�� �ʵǼ� ���� ���� �������� send�ϴµ� ���� �Ǿ��ٰ� �����ͼ� ����


02-20 future ���� ���� ���� ������ ���� ���� ���� ���� �� �ֵ��� �ϰ� �����Լ� �Ἥ ���� �Ǵ� ������ �����ؼ�
�ø��� ^-^///

02-22 �ٲܲ� singlton ���� ����, ����ó��~ &, && std::move ���� �켱 atomic gogo



02-24 rmi �����ϰ� ~~ ����ó�� ���� 25�ϳ� ��.�� //�״� SP ����� ��.�� ^.^