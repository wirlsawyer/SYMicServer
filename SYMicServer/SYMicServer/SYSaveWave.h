

#ifndef _SYSAVEWAVE_H_
#define _SYSAVEWAVE_H_
#include <sndfile.hh>
#include <iostream>
#include <cmath>
#pragma comment(lib, "libsndfile-1.lib")

#define M_PI	3.14159
class SYSaveWave
{
public:
	SYSaveWave()
	{
		
		const int format=SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
		//  const int format=SF_FORMAT_WAV | SF_FORMAT_FLOAT;
		const int channels=1;
		const int sampleRate=8000;
		const char* outfilename="foo.wav";

		SF_INFO     sfinfo;

		sfinfo.samplerate=sampleRate;//44100
		sfinfo.channels=channels;//1
		sfinfo.format=format;

		m_outfile = sf_open (outfilename, SFM_WRITE, &sfinfo);

		//if (outfile == NULL) return -1;
		
		// prepare a 3 seconds buffer and write it		
				

		/*
		SndfileHandle outfile(outfilename, SFM_WRITE, format, channels, sampleRate);

		const int size = sampleRate*3;

		float sample[size];
		float current=0.0;
		for (int i=0; i<size; i++) sample[i]=sin(float(i)/size*M_PI*1500);
		outfile.write(&sample[0], size);
		*/
	}

	void Close()
	{
		sf_close (m_outfile) ;
	}

	void AddData(char* pData, int len)
	{
		
		 sf_write_raw (m_outfile, pData, len);
	}
protected:
private:
	//SndfileHandle m_outfile;
	SNDFILE     *m_outfile;

};

#endif


/*
#include <sndfile.hh>
#include <iostream>
#include <cmath>

int main()
{

	const int format=SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	//  const int format=SF_FORMAT_WAV | SF_FORMAT_FLOAT;
	const int channels=1;
	const int sampleRate=48000;
	const char* outfilename="foo.wav";
	SndfileHandle outfile(outfilename, SFM_WRITE, format, channels, sampleRate);
	if (not outfile) return -1;


	// prepare a 3 seconds buffer and write it
	const int size = sampleRate*3;

	float sample[size];
	float current=0.0;
	for (int i=0; i<size; i++) sample[i]=sin(float(i)/size*M_PI*1500);
	outfile.write(&sample[0], size);
	return 0;
}
*/
