#ifndef _CPSTREAM_H_
#define _CPSTREAM_H_

#include <string>
#include <list>
#include <vector>

namespace Ryan{


	class CPStreamReader
	{
	public:
		CPStreamReader(char* pBuf, int bufSize) : _pBegin(pBuf), _pEnd(_pBegin + bufSize)
		{
			_pCurrent = pBuf;
		}

		const void* GetBuffer() { return _pBegin; }

		CPStreamReader& operator>>(bool& value) { ReadData(value); return *this; }
		CPStreamReader& operator>>(char& value)    { ReadData(value); return *this; }
		CPStreamReader& operator>>(short& value)    { ReadData(value); return *this; }
		CPStreamReader& operator>>(int& value)    { ReadData(value); return *this; }
		CPStreamReader& operator>>(float& value)   { ReadData(value); return *this; }
		CPStreamReader& operator>>(double& value)   { ReadData(value); return *this; }


		CPStreamReader& operator>>(std::string& value)
		{
			short size;

			*this >> size;

			if (_pCurrent + size - _pBegin > MAXRECVPACKETSIZE){
				cout << "CPStreamReader::string overflow (this string) will make exception" << endl;
				return *this;
			}


			if (size > 32760)
			{
				cout << "underflow occour";
			}
			value.resize((size_t)size);
			std::copy(_pCurrent, _pCurrent + size, value.begin());

			_pCurrent += size;
			return *this;
		}


		template<class T>
		CPStreamReader& operator>>(std::list<T>& value)
		{
			short size;
			*this >> size;

			for (int i = 0; i < size; ++i)
			{
				T temp;
				*this >> temp;
				value.push_back(temp);
			}
			return *this;
		}

		//만약 만족하는 멤버함수가 없다면 죽으니 그 멤버 변수를 대입할수 있는 >>연산자 추가해야된다(패킷 클래스에 직렬화 위해서는)
		template<class T>
		void operator>>(T& obj){
			obj.serialR(*this);
			return;
		}



		// stl 아닌것만
		template<class T>
		void ReadData(T& value)
		{
			int size = sizeof(value);
			memcpy(&value, _pCurrent, size);
			_pCurrent += size;
		}

	private:
		const char* _pBegin;
		const char *_pEnd;
		char*		 _pCurrent;

	};

	class CPStreamReaderBuffer : public CPStreamReader
	{
	public:
		CPStreamReaderBuffer();


		//읽기 버퍼가 필요하다면 사용하면 된다 
		CPStreamReaderBuffer(char* pBuf, int bufSize) : CPStreamReader(_buf, bufSize)
		{
			
		}

	private:
		char _buf[1024];
	};

	class CPStreamWriter{
	public:
		CPStreamWriter(char* pBuf, int bufSize) : _pBegin(pBuf), _pEnd(_pBegin + bufSize)
		{
			//사이즈를 마지막에 정해주기 위해서
			_pCurrent = pBuf + 2;
		}

		CPStreamWriter& operator<<(char value)   { WriteData(value);	return *this; }
		CPStreamWriter& operator<<(short value)   { WriteData(value);	return *this; }
		CPStreamWriter& operator<<(int value)   { WriteData(value);	return *this; }
		CPStreamWriter& operator<<(float value)   { WriteData(value);	return *this; }
		CPStreamWriter& operator<<(double value)   { WriteData(value);	return *this; }



		//string을 보낼때에는 스트링크기 + 2byte(스트링의 크기를 위한 short 변수)만큼 보낸다
		CPStreamWriter& operator<<(std::string& value)
		{
			short size = (short)value.size();

			if (_pCurrent + size - _pBegin > MAXSENDPACKETSIZE){
				cout << "CPStreamWriter::string overflow (this string will be deleted)" << endl;
				return *this;
			}

			*this << size;

			CopyMemory(_pCurrent, value.c_str(), size);
			//strcpy_s(_pCurrent, size+1, value.c_str());
			_pCurrent += size;

			return *this;
		}




		template<class T>
		CPStreamWriter& operator<<(std::list<T>& value)
		{
			short size = (short)value.size(); //리스트의 갯수이다  4개를 넣고 그후에 값 넣기
			*this << size;
			for (auto it = value.begin(); it != value.end(); ++it){
				*this << *it;
			}
			return *this;
		}



		// stl 아닌 타입만 사용
		template<class T>
		void WriteData(T& value)
		{
			int size = sizeof(value);
			memcpy(_pCurrent, &value, size);
			_pCurrent += size;
			//cout << "WRITEDATA::MOVED SIZE IS" << size << endl;
		}

		template<class T>
		void operator<<(T& obj){
			obj.serialW(*this);
		}

		void setBodySize(){
			short bodySize;
			bodySize = getCurrent() - getBegin() - 4;
			//cout << "setbodysize::::" << bodySize << endl;
			memcpy((void*)getBegin(), &bodySize, 2);
		}
		short getBodySize(){
			return  (getCurrent() - getBegin() - 4);
		}




		char* getCurrent(){ return _pCurrent; }	
		const char* getBegin() { return _pBegin; }
	
		//이건 우선은 실제 buffer에서만 불린다
	protected:
		void initCurrent(char* val){
			_pCurrent = val + 2; //type 먼저 넣어준다
			ZeroMemory((void*)_pBegin, 4);
		}

	private:

		
		const char* _pBegin;
		const char* _pEnd;
		char*		_pCurrent;

	};

	class CPStreamWriterBuffer : public CPStreamWriter
	{
	public:
			CPStreamWriterBuffer() : CPStreamWriter(_buf, sizeof(_buf)) {}

			short getBodySize(){
				short bodySize;
				CopyMemory(&bodySize,_buf,2);
				return bodySize;
			}
			char* getBuf(){ return _buf; }
			void init(){ initCurrent(_buf);}	

	private:
		char _buf[1024];
	};

}
#endif
